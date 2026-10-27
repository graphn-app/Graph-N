/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#ifdef _WIN32
#include "core/managers/host_manager_win32.h"
#include "core/managers/settings_manager.h"

namespace mngr
{

HostManager_Win32::HostManager_Win32() noexcept = default;
HostManager_Win32::~HostManager_Win32() noexcept = default;

std::thread HostManager_Win32::run_host(const blob::HostData& host)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_processes[host.host_id] = std::make_shared<HostProcess>();
    return std::thread(&HostManager_Win32::host_thread, this, host);
}

void HostManager_Win32::kill_host(uint64_t id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto it = m_processes.find(id);
    LogRAssert(it != m_processes.end(), "Host #" << id << ": can't kill - host not found", );
    blob::HostData new_command;
    new_command.host_id = id;
    new_command.cmd = blob::HostData::Cmd_Terminate;
    it->second->new_commands.push_back(new_command);
    Log("Host #" << id << ": store 'kill' cmd");
}

void HostManager_Win32::shutdown_host(uint64_t id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto it = m_processes.find(id);
    LogRAssert(it != m_processes.end(), "Host #" << id << ": can't shutdown - host not found", );
    blob::HostData new_command;
    new_command.host_id = id;
    new_command.cmd = blob::HostData::Cmd_Shutdown;
    it->second->new_commands.push_back(new_command);
    Log("Host #" << id << ": store 'shutdown' cmd");
}

void HostManager_Win32::rename_host(uint64_t id, const std::string& new_wf_name, const std::string& new_node_name)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto it = m_processes.find(id);
    LogRAssert(it != m_processes.end(), "Host #" << id << ": can't rename - host not found", );
    blob::HostData new_command;
    new_command.host_id = id;
    new_command.cmd = blob::HostData::Cmd_Rename;
    new_command.wf_name = new_wf_name;
    new_command.node_name = new_node_name;
    it->second->new_commands.push_back(std::move(new_command));
    Log("Host #" << id << ": store 'rename' cmd");
}

void HostManager_Win32::raise_host(uint64_t id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto it = m_processes.find(id);
    LogRAssert(it != m_processes.end(), "Host #" << id << ": can't raise - host not found", );
    blob::HostData new_command;
    new_command.host_id = id;
    new_command.cmd = blob::HostData::Cmd_Raise;
    it->second->new_commands.push_back(new_command);
    Log("Host #" << id << ": store 'raise' cmd");
}

bool HostManager_Win32::is_host_alive(const HostProcess& host_proc) const
{
    if (!host_proc.hproc || !host_proc.hproc.get())
        return false;

    return (::WaitForSingleObject(host_proc.hproc.get(), 0) == WAIT_TIMEOUT);
}

enum HostStatus : uint8_t
{
    HostStatus_StartFailed      = 0x01,
    HostStatus_Terminate        = 0x02,
    HostStatus_FinishedOk       = 0x04,
    HostStatus_FinishedFail     = 0x08,
    HostStatus_TerminateDelayed = 0x10,
};

void HostManager_Win32::host_thread(blob::HostData host_data)
{
    std::shared_ptr<HostProcess> host_proc;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto it = m_processes.find(host_data.host_id);
        if (it == m_processes.end())
        {
            on_host_finished_job(host_data.host_id, false);
            LogError("Host #" << host_data.host_id << " not found");
            return;
        }
        host_proc = it->second;
    }

    uint32_t status = HostStatus_StartFailed | HostStatus_Terminate;
    try
    {
        status = host_thread_worker(host_proc, host_data);
    }
    catch (const std::exception& e)
    {
        LogExcep("Host #" << host_data.host_id << " thread ex: " << e.what());
    }
    catch (...) {}

    if (host_proc->hproc)
    {
#ifdef _DEBUG
        constexpr DWORD wait_timeout = 500000;
#else
        constexpr DWORD wait_timeout = 20000;
#endif

        if ((status & HostStatus_TerminateDelayed) != 0)
            ::WaitForSingleObject(host_proc->hproc.get(), wait_timeout);

        if ((status & (HostStatus_TerminateDelayed | HostStatus_Terminate)) != 0 &&
            is_host_alive(*host_proc))
        {
            const auto res = ::TerminateProcess(host_proc->hproc.get(), 1);
            Log("Host #" << host_data.host_id << ": terminated, res=" << res);
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto id = host_data.host_id;
        m_processes.erase(id);
    }

    if ((status & HostStatus_StartFailed) != 0)
        on_host_started(host_data.host_id, false);
    else if ((status & (HostStatus_FinishedOk | HostStatus_FinishedFail)) != 0)
        on_host_finished_job(host_data.host_id, (status & HostStatus_FinishedOk) != 0);
}

uint32_t HostManager_Win32::host_thread_worker(std::shared_ptr<HostProcess> host_proc, const blob::HostData& host_data)
{
    const auto notify_start_failed = [&host_data](const char* message) -> uint32_t
    {
        LogWarnS("Host #" << host_data.host_id << ": " << message << ", err: " << LogApiERR);
        return HostStatus_StartFailed | HostStatus_Terminate;
    };

    const auto exit_on_global_stop = [&host_data]() -> uint32_t
    {
        LogS("Host #" << host_data.host_id << ": global stop");
        return HostStatus_Terminate;
    };

    const auto host_path = fs::get_host_path();
    if (!tools::path::exists(host_path))
        return notify_start_failed("Host binary not found");

    const HANDLE hjob = ::CreateJobObjectW(nullptr, nullptr);
    if (nullptr == hjob)
        return notify_start_failed("Failed to create job");
    host_proc->hjob.reset(hjob);

    ::JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {};
    jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!::SetInformationJobObject(host_proc->hjob.get(), JobObjectExtendedLimitInformation, &jeli, sizeof(jeli))) 
        return notify_start_failed("Failed to set information on job");

    ::PROCESS_INFORMATION pi = {};
    ::STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.wShowWindow = SW_SHOWNORMAL;

    const auto pipe_unique = tools::str::utf8_to_wide(tools::str::generate_uuid_v4());
    const std::wstring pipe_name = L"\\\\.\\pipe\\" + pipe_unique;

    const auto min_size = cast::to<DWORD>(blob::HostSerializer::get_min_size());
    const auto max_size = cast::to<DWORD>(blob::HostSerializer::get_max_size());

    const HANDLE hpipe = ::CreateNamedPipeW(pipe_name.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1, std::max(max_size, DWORD(4096)), std::max(max_size, DWORD(4096)), 0, nullptr);
    if (INVALID_HANDLE_VALUE == hpipe)
        return notify_start_failed("Can't create IPC pipe");
    host_proc->hpipe.reset(hpipe);

#ifdef _DEBUG
    const auto command_line = fmt::format(FMT_STRING(L" -catch -pipe {} -id {}"), pipe_unique, host_data.host_id);
#else
    const auto command_line = fmt::format(FMT_STRING(L" -pipe {} -id {}"), pipe_unique, host_data.host_id);
#endif
    std::vector<wchar_t> cmdline_vec(command_line.begin(), command_line.end());
    cmdline_vec.push_back(0);

    if (!::CreateProcessW(host_path.wstring().c_str(), cmdline_vec.data(), nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, nullptr, &si, &pi)) 
        return notify_start_failed("Failed to create host process");

    Handle hthread(pi.hThread, ::CloseHandle);
    host_proc->hproc.reset(pi.hProcess);
    host_proc->proc_id = pi.dwProcessId;

    if (!::AssignProcessToJobObject(host_proc->hjob.get(), host_proc->hproc.get()))
        return notify_start_failed("Failed to assign process to job");

    BOOL job_result;
    if (!::IsProcessInJob(host_proc->hproc.get(), host_proc->hjob.get(), &job_result) || !job_result)
        return notify_start_failed("Process assigned to job, but verification failed");

    if (g::app_global_stop_all_threads.stop_requested())
        return exit_on_global_stop();

    OVERLAPPED ov = {};
    ov.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);

    if (::ResumeThread(pi.hThread) == DWORD(-1))
        return notify_start_failed("Failed to resume process");
    hthread.reset();

    Log("Starting " << host_path << " " << command_line);

#ifdef _DEBUG
    constexpr DWORD pipe_timeout = 500000;
    constexpr auto wait_client_ready = 500s;
#else
    constexpr DWORD pipe_timeout = 5000;
    constexpr auto wait_client_ready = 5s;
#endif

    uint32_t result = 0;
    if (!::ConnectNamedPipe(host_proc->hpipe.get(), &ov))
    {
        const auto apierr = ::GetLastError();
        if (apierr == ERROR_IO_PENDING)
        {
            if (::WaitForSingleObject(ov.hEvent, pipe_timeout) != WAIT_OBJECT_0)
                result = notify_start_failed("Pipe wasn't connected by client");
        }
        else if (apierr != ERROR_PIPE_CONNECTED)
            result = notify_start_failed("Can't connect pipe");
    }

    ::CloseHandle(ov.hEvent);
    if (result != 0)
        return result;

    const auto write_pipe = [&host_proc](const blob::HostData& data)
    {
        DWORD bytes_written = 0;
        ::OVERLAPPED write_ov = {};
        write_ov.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);

        bool result = false;
        const auto blob = blob::HostSerializer::serialize(data);

        if (!::WriteFile(host_proc->hpipe.get(), blob->data(), cast::to<DWORD>(blob->size()), &bytes_written, &write_ov))
        {
            if (::GetLastError() == ERROR_IO_PENDING)
            {
                if (::GetOverlappedResult(host_proc->hpipe.get(), &write_ov, &bytes_written, TRUE))
                    result = true;
                else
                    LogWarnS("Host #" << data.host_id << ": can't write to pipe - overlapped result failed. err: " << LogApiERR);
            }
            else
                LogWarnS("Host #" << data.host_id << ": can't write to pipe. err: " << LogApiERR);
        }
        else
            result = true;

        ::CloseHandle(write_ov.hEvent);
        return result;
    };

    const auto get_data_ready = [min_size, &host_proc, id = host_data.host_id]() -> DWORD
    {
        DWORD bytes_avail = 0;
        const BOOL peek_status = ::PeekNamedPipe(host_proc->hpipe.get(), nullptr, 0, nullptr, nullptr, &bytes_avail);
        if (peek_status && bytes_avail >= min_size)
        {
            LogS("Host #" << id << ": message avail (" << bytes_avail << "b)");
            return bytes_avail;
        }
        return 0;
    };

    const auto read_pipe = [&host_proc, id = host_data.host_id](DWORD data_size, blob::HostData& data)
    {
        byte_buff buffer(data_size);
        DWORD bytes_read = 0;
        ::OVERLAPPED read_ov = {};
        read_ov.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);

        bool success = false;
        if (::ReadFile(host_proc->hpipe.get(), buffer.data(), data_size, &bytes_read, &read_ov))
        {
            success = true;
        }
        else if (::GetLastError() == ERROR_IO_PENDING)
        {
            if (::GetOverlappedResult(host_proc->hpipe.get(), &read_ov, &bytes_read, TRUE))
                success = true;
            else
                LogWarnS("Host #" << id << ": readfile overlapped result err: " << LogApiERR);
        }

        ::CloseHandle(read_ov.hEvent);

        if (!success || bytes_read != data_size)
            return false;

        blob::Blob blob(std::move(buffer));
        data = blob::HostSerializer::deserialize(blob);

        if (data.host_id != id)
            LogWarnS("Unknown host id in command: " << data.host_id);
        if (blob::HostData::Cmd_None == data.cmd)
            LogWarnS("Unknown command: " << cast::to_u32(data.cmd));
        return data.cmd != blob::HostData::Cmd_None && data.host_id == id;
    };

    bool client_ready = false;
    const auto start_time = std::chrono::steady_clock::now();

    while (!client_ready)
    {
        if (g::app_global_stop_all_threads.stop_requested())
            return exit_on_global_stop();

        if (!is_host_alive(*host_proc))
            return notify_start_failed("Client process died");

        if (std::chrono::steady_clock::now() - start_time > wait_client_ready)
            return notify_start_failed("Destroy client by timeout");

        const DWORD bytes_avail = get_data_ready();
        if (bytes_avail > 0)
        {
            blob::HostData response;
            if (!read_pipe(bytes_avail, response))
                return notify_start_failed("Failed to read client hello");

            if (response.cmd != blob::HostData::Cmd_Ready)
                return notify_start_failed("Invalid client hello response");

            client_ready = true;
        }
        else
        {
            std::this_thread::sleep_for(50ms);
        }
    }

    {
        DWORD bytes_written = 0;
        ::OVERLAPPED write_ov = {};
        write_ov.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);

        result = 0;
        const auto blob = blob::HostSerializer::serialize(host_data);
        if (!::WriteFile(host_proc->hpipe.get(), blob->data(), cast::to<DWORD>(blob->size()), &bytes_written, &write_ov))
        {
            if (::GetLastError() == ERROR_IO_PENDING)
            {
                if (!::GetOverlappedResult(host_proc->hpipe.get(), &write_ov, &bytes_written, TRUE))
                    result = notify_start_failed("Failed to send initial config to client");
            }
            else
                result = notify_start_failed("Can't write to pipe");
        }
        ::CloseHandle(write_ov.hEvent);
        if (result != 0)
            return result;
    }

    on_host_started(host_data.host_id, true);
    Log("Host #" << host_data.host_id << ": client initialized");

    std::vector<blob::HostData> new_commands;

    while (true)
    {
        if (g::app_global_stop_all_threads.stop_requested())
            return exit_on_global_stop();

        if (!is_host_alive(*host_proc))
        {
            Log("Host #" << host_data.host_id << ": process died");
            return HostStatus_Terminate | HostStatus_FinishedFail;
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!host_proc->new_commands.empty())
                std::swap(host_proc->new_commands, new_commands);
        }
        for (auto& command : new_commands)
        {
            if (!write_pipe(command))
            {
                LogWarn("Host #" << host_data.host_id << "Failed to send command");
                return HostStatus_Terminate | HostStatus_FinishedFail;
            }
            if (blob::HostData::Command::Cmd_Terminate == command.cmd)
                return HostStatus_TerminateDelayed | HostStatus_FinishedFail;
            if (blob::HostData::Command::Cmd_Shutdown == command.cmd)
                return HostStatus_TerminateDelayed | HostStatus_FinishedOk;
        }
        new_commands.clear();

        const DWORD bytes_avail = get_data_ready();
        if (bytes_avail > 0)
        {
            blob::HostData response;
            if (read_pipe(bytes_avail, response))
            {
                if (blob::HostData::Cmd_Apply == response.cmd)
                {
                    if (!response.settings.empty())
                        g::pOptsManager->ui_host_settings_save(response.settings);
                    shutdown_host(host_data.host_id);
                }
                else
                    LogWarn("Host #" << host_data.host_id << ": unknown host command: " << cast::to_u32(response.cmd));                
            }
            else
            {
                LogWarn("Host #" << host_data.host_id << ": failed to read incoming pipe data");
            }
        }

        std::this_thread::sleep_for(100ms);
    }

    LogError("Host #" << host_data.host_id << ": thread finished in unexpected way");
    return HostStatus_Terminate | HostStatus_FinishedFail;
}

} // namespace mngr

#endif // #ifdef _WIN32
