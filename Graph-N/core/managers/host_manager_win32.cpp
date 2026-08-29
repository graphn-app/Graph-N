#ifdef _WIN32
#include "core/managers/host_manager_win32.h"
#include "core/managers/settings_manager.h"

namespace mngr
{

HostManager_Win32::HostManager_Win32() noexcept = default;
HostManager_Win32::~HostManager_Win32() noexcept = default;

void HostManager_Win32::run_host(const blob::HostData& host)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_hosts[host.host_id].host_data = host;
    std::thread(&HostManager_Win32::host_thread, this, std::ref(m_hosts[host.host_id])).detach();
}

void HostManager_Win32::kill_host(uint64_t id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto it = m_hosts.find(id);
    LogRAssert(it != m_hosts.end(), "Host " << id << " not found", );
    it->second.host_data.cmd = blob::HostData::Cmd_Terminate;
    it->second.has_new_command.store(true, std::memory_order_relaxed);
}

void HostManager_Win32::rename_host(uint64_t id, const std::string& new_wf_name, const std::string& new_node_name)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto it = m_hosts.find(id);
    LogRAssert(it != m_hosts.end(), "Host " << id << " not found", );
    it->second.host_data.cmd = blob::HostData::Cmd_Rename;
    it->second.host_data.wf_name = new_wf_name;
    it->second.host_data.node_name = new_node_name;
    it->second.has_new_command.store(true, std::memory_order_relaxed);
}

void HostManager_Win32::raise_host(uint64_t id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto it = m_hosts.find(id);
    LogRAssert(it != m_hosts.end(), "Host " << id << " not found", );
    it->second.host_data.cmd = blob::HostData::Cmd_Raise;
    it->second.has_new_command.store(true, std::memory_order_relaxed);
}

void HostManager_Win32::terminate_and_cleanup(const Host& host)
{
    if (host.hproc)
    {
        const auto res = ::TerminateProcess(host.hproc.get(), 1);
        Log("Host terminated, id:" << host.host_data.host_id << ", res:" << res);
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto id = host.host_data.host_id;
    m_hosts.erase(id);
}

bool HostManager_Win32::is_host_alive(const Host& host) const
{
    if (!host.hproc || !host.hproc.get())
        return false;

    return (::WaitForSingleObject(host.hproc.get(), 0) == WAIT_TIMEOUT);
}

void HostManager_Win32::host_thread(Host& host)
{
    const auto notify_start_failed = [&host, this](const char* message)
    {
        const auto id = host.host_data.host_id;
        LogWarn(message << ". Host:" << id << ", err: " << LogApiERR);
        terminate_and_cleanup(host);
        on_host_started(id, false);
    };

    const auto host_path = fs::get_host_path();
    if (!tools::path::exists(host_path))
        return notify_start_failed("Host binary not found");

    const HANDLE hjob = ::CreateJobObjectW(nullptr, nullptr);
    if (nullptr == hjob)
        return notify_start_failed("Failed to create job");
    host.hjob.reset(hjob);

    ::JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {};
    jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!::SetInformationJobObject(host.hjob.get(), JobObjectExtendedLimitInformation, &jeli, sizeof(jeli))) 
        return notify_start_failed("Failed to set information on job");

    ::PROCESS_INFORMATION pi = {};
    ::STARTUPINFOW si;
    si.cb = sizeof(si);
    si.wShowWindow = SW_SHOWNORMAL;

    const auto pipe_unique = tools::str::utf8_to_wide(tools::str::generate_uuid_v4());
    const std::wstring pipe_name = L"\\\\.\\pipe\\" + pipe_unique;

#ifdef _DEBUG
    const auto command = fmt::format(FMT_STRING(L"-pipe {} -id {} -catch"), pipe_unique, host.host_data.host_id);
#else
    const auto command = fmt::format(FMT_STRING(L"-pipe {} -id {}"), tools::str::utf8_to_wide(pipe_name), host.host_data.host_id);
#endif
    std::vector<wchar_t> cmd_vec(command.begin(), command.end());
    cmd_vec.push_back(0);

    if (!::CreateProcessW(host_path.wstring().c_str(), cmd_vec.data(), nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, nullptr, &si, &pi)) 
        return notify_start_failed("Failed to create host process");

    Handle hthread(pi.hThread, ::CloseHandle);
    host.hproc.reset(pi.hProcess);
    host.proc_id = pi.dwProcessId;

    if (!::AssignProcessToJobObject(host.hjob.get(), host.hproc.get()))
        return notify_start_failed("Failed to assign process to job");

    BOOL result;
    if (!::IsProcessInJob(host.hproc.get(), host.hjob.get(), &result) || !result)
        return notify_start_failed("Process assigned to job, but verification failed");

    const auto min_size = cast::to<DWORD>(blob::HostSerializer::serialize(blob::HostData::Cmd_None, 0)->size());
    const auto max_size = min_size + MAX_PATH * 2 /* image pathes */ + 200 /* wf and node names */ + 20 /*comfyui host*/ + 200 /*settings*/;

    const HANDLE hpipe = ::CreateNamedPipeW(pipe_name.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1, std::max(max_size, DWORD(4096)), std::max(max_size, DWORD(4096)), 0, nullptr);
    if (INVALID_HANDLE_VALUE == hpipe)
        return notify_start_failed("Can't create IPC pipe");
    host.hpipe.reset(hpipe);

    if (g::app_global_stop_all_threads.stop_requested())
        return terminate_and_cleanup(host);

    OVERLAPPED ov = {};
    ov.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);

    ::ResumeThread(pi.hThread);
    hthread.reset();

    Log("Starting " << host_path << " " << command);

    bool exit = false;
    if (!::ConnectNamedPipe(host.hpipe.get(), &ov))
    {
        const auto apierr = ::GetLastError();
        if (apierr == ERROR_IO_PENDING)
        {
            if (::WaitForSingleObject(ov.hEvent, 10000) != WAIT_OBJECT_0)
            {
                notify_start_failed("Pipe wasn't connected by client");
                exit = true;
            }
        }
        else if (apierr != ERROR_PIPE_CONNECTED)
        {
            notify_start_failed("Can't connect pipe");
            exit = true;
        }
    }

    ::CloseHandle(ov.hEvent);
    if (exit)
        return;

    const auto write_pipe = [&host](blob::HostData::Command cmd)
    {
        DWORD bytes_written = 0;
        ::OVERLAPPED write_ov = {};
        write_ov.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);

        bool result = false;
        const auto blob = blob::HostSerializer::serialize(cmd, host.host_data.host_id);

        if (!::WriteFile(host.hpipe.get(), blob->data(), cast::to<DWORD>(blob->size()), &bytes_written, &write_ov))
        {
            if (::GetLastError() == ERROR_IO_PENDING)
            {
                if (::GetOverlappedResult(host.hpipe.get(), &write_ov, &bytes_written, TRUE))
                    result = true;
                else
                    LogS("Can't write to pipe - overlapped result failed. err: " << LogApiERR);
            }
            else
                LogS("Can't write to pipe. err: " << LogApiERR);
        }
        else
            result = true;

        ::CloseHandle(write_ov.hEvent);
        return result;
    };

    const auto get_data_ready = [min_size, &host]() -> DWORD
    {
        DWORD bytes_avail = 0;
        const BOOL peek_status = ::PeekNamedPipe(host.hpipe.get(), nullptr, 0, nullptr, nullptr, &bytes_avail);
        if (peek_status && bytes_avail >= min_size)
            return bytes_avail;
        return 0;
    };

    const auto read_pipe = [&host](DWORD data_size, blob::HostData& data)
    {
        byte_buff buffer(data_size);
        DWORD bytes_read = 0;
        ::OVERLAPPED read_ov = {};
        read_ov.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);

        bool success = false;
        if (::ReadFile(host.hpipe.get(), buffer.data(), data_size, &bytes_read, &read_ov))
        {
            success = true;
        }
        else if (::GetLastError() == ERROR_IO_PENDING)
        {
            if (::GetOverlappedResult(host.hpipe.get(), &read_ov, &bytes_read, TRUE))
                success = true;
        }

        ::CloseHandle(read_ov.hEvent);

        if (!success || bytes_read != data_size)
            return false;

        blob::Blob blob(std::move(buffer));
        data = blob::HostSerializer::deserialize(blob);
        return data.cmd != blob::HostData::Cmd_None;
    };

    bool client_ready = false;
    auto start_time = std::chrono::steady_clock::now();

    while (!client_ready)
    {
        if (g::app_global_stop_all_threads.stop_requested())
            return terminate_and_cleanup(host);

        if (!is_host_alive(host))
            return notify_start_failed("Client process died");

        if (std::chrono::steady_clock::now() - start_time > 5s)
            return notify_start_failed("Destroy client by timeout");

        const DWORD bytes_avail = get_data_ready();
        if (bytes_avail > 0)
        {
            blob::HostData response;
            if (!read_pipe(bytes_avail, response))
                return notify_start_failed("Failed to read client data");

            if (response.cmd != blob::HostData::Cmd_Ready)
                return notify_start_failed("Invalid client response");

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

        exit = false;
        const auto blob = blob::HostSerializer::serialize(host.host_data);
        if (!::WriteFile(host.hpipe.get(), blob->data(), cast::to<DWORD>(blob->size()), &bytes_written, &write_ov))
        {
            if (::GetLastError() == ERROR_IO_PENDING)
            {
                if (!::GetOverlappedResult(host.hpipe.get(), &write_ov, &bytes_written, TRUE))
                {
                    exit = true;
                    notify_start_failed("Failed to send initial config to client");
                }
            }
            else
            {
                exit = true;
                notify_start_failed("Can't write to pipe");
            }
        }
        ::CloseHandle(write_ov.hEvent);
        if (exit)
            return;
    }

    on_host_started(host.host_data.host_id, true);
    Log("Host initialized, id: " << host.host_data.host_id);

    while (true)
    {
        if (g::app_global_stop_all_threads.stop_requested())
            return terminate_and_cleanup(host);

        if (!is_host_alive(host))
        {
            on_host_finished_job(host.host_data.host_id, false);
            return terminate_and_cleanup(host);
        }

        if (host.has_new_command.load(std::memory_order_relaxed))
        {
            host.has_new_command.store(false, std::memory_order_relaxed);
            blob::HostData::Command new_command;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                new_command = host.host_data.cmd;
            }
            if (!write_pipe(new_command))
            {
                LogWarn("Failed to send command to host, so terminate it, id: " << host.host_data.host_id);
                on_host_finished_job(host.host_data.host_id, false);
                return terminate_and_cleanup(host);
            }
        }

        const DWORD bytes_avail = get_data_ready();
        if (bytes_avail > 0)
        {
            blob::HostData response;
            if (read_pipe(bytes_avail, response))
            {
                if (blob::HostData::Cmd_Apply == response.cmd)
                {
                    Log("Host finished job, id " << host.host_data.host_id);
                    if (!response.settings.empty())
                        g::pOptsManager->ui_host_settings_save(response.settings);
                    on_host_finished_job(host.host_data.host_id, true);
                    return terminate_and_cleanup(host);
                }
                else
                {
                    LogWarn("Unknown host command, host:" << host.host_data.host_id << ", cmd:" << cast::to_u32(response.cmd));
                }
            }
            else
            {
                LogWarn("Failed to read incoming pipe data. Host ID: " << host.host_data.host_id);
            }
        }

        std::this_thread::sleep_for(100ms);
    }

    LogError("Host thread finished in unexpected way, id:" << host.host_data.host_id);
    terminate_and_cleanup(host);
}

} // namespace mngr

#endif // #ifdef _WIN32
