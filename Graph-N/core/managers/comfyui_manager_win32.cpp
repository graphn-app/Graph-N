#ifdef _WIN32
#include "core/managers/comfyui_manager_win32.h"
#include "core/managers/settings_manager.h"
#include "ui/graph-n_flow.h"

namespace mngr
{

ComfyUIManager_Win32::ComfyUIManager_Win32() noexcept
    : m_hjob(nullptr, ::CloseHandle)
    , m_hproc(nullptr, ::CloseHandle)
    , m_hpipe(nullptr, ::CloseHandle)
{}

ComfyUIManager_Win32::~ComfyUIManager_Win32() noexcept = default;

bool ComfyUIManager_Win32::init()
{
    if (!ComfyUIManager::init())
        return false;

    return true;
}

void ComfyUIManager_Win32::stop_and_cleanup()
{
    if (!g::pOptsManager->comfyui_kill_server_load() && !m_console_hidden) // ignore user option if console was hidden
        enable_autokill(false);
    kill(true);
}

void ComfyUIManager_Win32::run_server()
{
    kill(true);

    const bool kill = g::pOptsManager->comfyui_kill_server_load();
    bool capture_log = g::pOptsManager->comfyui_capture_output_load();
    m_console_hidden = false;
    if (kill)
        m_console_hidden = g::pOptsManager->comfyui_hide_console_load();

    Handle hpipe_wr(nullptr, ::CloseHandle);
    create_job(kill);

    PROCESS_INFORMATION pi = {};
    STARTUPINFOW si = {};
    prepare(si, capture_log, m_console_hidden, hpipe_wr);

    if (!create_process(si, pi, m_console_hidden, capture_log, m_hjob != nullptr))
    {
        m_hpipe.reset();
        m_hjob.reset();
        hpipe_wr.reset();
    }
    else
    {
        m_kill_server_flag.store(false, std::memory_order_relaxed);

        apply_job(pi.hProcess);

        m_hproc.reset(pi.hProcess);
        m_procid = pi.dwProcessId;

        if (capture_log)
            m_log_th = std::thread(&ComfyUIManager_Win32::read_logs_th, this);

        if (pi.hThread != nullptr)
        {
            ::ResumeThread(pi.hThread);
            ::CloseHandle(pi.hThread);
        }

        g::pFlow->show_logs_panel(true);
    }
}

bool ComfyUIManager_Win32::create_job(bool auto_kill)
{
    Handle hjob(::CreateJobObject(nullptr, nullptr), ::CloseHandle);
    if (!hjob)
    {
        LogWarn("Failed to create job, err: " << LogApiERR << ". " << APP_COMFYUI << " will not be killed on app exit");
        g::pFlow->handle_comfyui_runserver_error(::GetLastError(), "Run " APP_COMFYUI, APP_COMFYUI " will not be killed on app exit");
        return false;
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {};
    jeli.BasicLimitInformation.LimitFlags = (auto_kill ? JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE : 0);

    if (!::SetInformationJobObject(hjob.get(), JobObjectExtendedLimitInformation, &jeli, sizeof(jeli))) 
    {
        LogWarn("Failed to set information on job, err: " << LogApiERR << ". " << APP_COMFYUI << " will not be killed n app exit");
        g::pFlow->handle_comfyui_runserver_error(::GetLastError(), "Run " APP_COMFYUI, APP_COMFYUI " will not be killed on app exit");
        return false;
    }

    m_hjob = std::move(hjob);
    return true;
}

bool ComfyUIManager_Win32::apply_job(HANDLE hproc)
{
    if (!::AssignProcessToJobObject(m_hjob.get(), hproc))
    {
        LogWarn("Failed to assign process to job, err: " << LogApiERR << ". " << APP_COMFYUI " will not be killed on app exit");
        m_hjob.reset();
        g::pFlow->handle_comfyui_runserver_error(::GetLastError(), "Run " APP_COMFYUI, APP_COMFYUI " will not be killed on app exit");
        return false;
    }

    BOOL result;
    if (!::IsProcessInJob(hproc, m_hjob.get(), &result) || !result)
    {
        LogWarn("Process assigned to job, but verification failed, err: " << LogApiERR << ". " << APP_COMFYUI " will not be killed on app exit");
        m_hjob.reset();
        g::pFlow->handle_comfyui_runserver_error(::GetLastError(), "Run " APP_COMFYUI, APP_COMFYUI " will not be killed on app exit");
        return false;
    }
    
    return true;
}

void ComfyUIManager_Win32::prepare(STARTUPINFOW& si, bool& capture_log, bool hide_console, Handle& hpipe_wr)
{
    if (capture_log)
    {
        SECURITY_ATTRIBUTES sa_attr = {};
        sa_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa_attr.bInheritHandle = TRUE;
        sa_attr.lpSecurityDescriptor = nullptr;

        HANDLE hrd = nullptr, hwr = nullptr;
        if (::CreatePipe(&hrd, &hwr, &sa_attr, 0))
        {
            if (::SetHandleInformation(hrd, HANDLE_FLAG_INHERIT, 0))
            {
                m_hpipe.reset(hrd);
                hpipe_wr.reset(hwr);
            }
            else
            {
                LogWarn("Can't set pipe handle information, err: " << LogApiERR << ". Output capture will be disabled");
                ::CloseHandle(hrd);
                ::CloseHandle(hwr);
                capture_log = false;
                g::pFlow->handle_comfyui_runserver_error(::GetLastError(), "Run " APP_COMFYUI, "Output capture will be disabled");
            }
        }
        else
        {
            LogWarn("Can't create pipe, err: " << LogApiERR << ". Capture will be disabled");
            capture_log = false;
            g::pFlow->handle_comfyui_runserver_error(::GetLastError(), "Run " APP_COMFYUI, "Output capture will be disabled");
        }
    }

    si.cb = sizeof(si);
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdError = (capture_log ? hpipe_wr.get() : GetStdHandle(STD_ERROR_HANDLE));
    si.hStdOutput = (capture_log ? hpipe_wr.get() : GetStdHandle(STD_OUTPUT_HANDLE));
    si.dwFlags |= (capture_log ? STARTF_USESTDHANDLES : 0) | (hide_console ? STARTF_USESHOWWINDOW : 0);
    si.wShowWindow = (hide_console ? SW_HIDE : SW_SHOWNORMAL);
}

bool ComfyUIManager_Win32::create_process(STARTUPINFOW& si, PROCESS_INFORMATION& pi, bool hide_console, bool capture_logs, bool suspended)
{
    const auto& python = g::pOptsManager->comfyui_python_load();
    const auto python_args = tools::str::utf8_to_wide(g::pOptsManager->comfyui_python_args_load());
    const auto& comfyui = g::pOptsManager->comfyui_main_py_load();
    const auto comfyui_as_arg = tools::path::build_commandline_arg(comfyui.wstring());
    const auto comfyui_args = tools::str::utf8_to_wide(g::pOptsManager->comfyui_comfyui_args_load());
    const auto& workdir = g::pOptsManager->comfyui_workdir_load();
    
    const auto command = fmt::format(FMT_STRING(L"{} {} {} {} {}"), python.filename().wstring(), (hide_console ? L"-u" : L""), python_args, comfyui_as_arg, comfyui_args);
    std::vector<wchar_t> cmd_vec(command.begin(), command.end());
    cmd_vec.push_back(0);

    Log("Starting " << python);
    Log("Process options: hide_console=" << hide_console << ", capture_logs=" << capture_logs << ", suspended=" << suspended);

    if (!::CreateProcessW(python.wstring().c_str(), cmd_vec.data(), nullptr, nullptr,
        (capture_logs ? TRUE : FALSE),
        (hide_console ? CREATE_NO_WINDOW : 0) | (suspended ? CREATE_SUSPENDED : 0) | CREATE_UNICODE_ENVIRONMENT,
        nullptr, workdir.wstring().c_str(), &si, &pi)) 
    {
        LogWarn("Failed to create " APP_COMFYUI " process, err: " << LogApiERR);
        g::pFlow->handle_comfyui_runserver_error(::GetLastError(), "Run " APP_COMFYUI, "Failed to create process");
        return false;
    }

    return true;
}

void ComfyUIManager_Win32::setup_auto_kill(bool enable)
{
    enable_autokill(enable);
}

void ComfyUIManager_Win32::enable_autokill(bool enable)
{
    if (!m_hjob)
        return;

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {};
    if (!::QueryInformationJobObject(m_hjob.get(), JobObjectExtendedLimitInformation, &jeli, sizeof(jeli), nullptr))
    {
        LogWarn("Failed to query job info, err: " << LogApiERR);
        return;
    }

    if (enable)
        jeli.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    else
        jeli.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

    if (!::SetInformationJobObject(m_hjob.get(), JobObjectExtendedLimitInformation, &jeli, sizeof(jeli)))
    {
        LogWarn("Failed to change job flags, err: " << LogApiERR);
    }
    else
    {
        Log("Autokill set to " << enable);
    }
}

void ComfyUIManager_Win32::kill(bool force_kill)
{
    g::pFlow->show_logs_panel(true);

    if (m_hproc)
    {
        if (force_kill || m_console_hidden || g::pOptsManager->comfyui_kill_server_load())
        {
            const auto res = ::TerminateProcess(m_hproc.get(), 0);
            Log(APP_COMFYUI " termination result: " << res << ", err: " << LogApiERR);
            m_hproc.reset();
        }
    }

    m_kill_server_flag.store(true, std::memory_order_relaxed);
    if (m_hpipe && m_log_th.joinable())
        m_log_th.join();

    m_procid = 0;
    m_hpipe.reset();
    m_hjob.reset();
    m_hproc.reset();
}

uint32_t ComfyUIManager_Win32::check_existing_process() const
{
    CComPtr<IWbemLocator> locator;
    auto hres = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&locator);
    if (FAILED(hres))
    {
        LogWarn("Failed to create IWbemLocator, err: " << LogHRES(hres));
        return 0;
    }

    CComPtr<IWbemServices> service;
    hres = locator->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, nullptr, 0, nullptr, nullptr, &service);
    if (FAILED(hres))
    {
        LogWarn("Failed to connect ROOT\\CIMV2, err: " << LogHRES(hres));
        return 0;
    }

    hres = CoSetProxyBlanket(service, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
    if (FAILED(hres))
    {
        LogWarn("Failed to configure service, err: " << LogHRES(hres));
        return 0;
    }

    CComPtr<IEnumWbemClassObject> enumerator;
    hres = service->ExecQuery(_bstr_t(L"WQL"),
        _bstr_t(L"SELECT CommandLine, ProcessId FROM Win32_Process WHERE Name LIKE '%python%'"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &enumerator);
    if (FAILED(hres))
    {
        LogWarn("Failed to query python processes, err: " << LogHRES(hres));
        return 0;
    }

    if (enumerator != nullptr)
    {
        ULONG ret;
        CComPtr<IWbemClassObject> object;
        while (S_OK == enumerator->Next(WBEM_INFINITE, 1, &object, &ret))
        {
            CComVariant var;
            hres = object->Get(L"CommandLine", 0, &var, nullptr, nullptr);
            if (FAILED(hres))
            {
                LogWarn("Failed to get python CommandLine, err: " << LogHRES(hres));
                object.Release();
                continue;
            }

            if (VT_BSTR == var.vt && wcsstr(var.bstrVal, L"main.py") != nullptr)
            {
                var.Clear();
                hres = object->Get(L"ProcessId", 0, &var, nullptr, nullptr);
                if (FAILED(hres) || (var.vt != VT_UI4 && var.vt != VT_I4))
                {
                    LogWarn("Failed to get python ProcessId, err: " << LogHRES(hres));
                    object.Release();
                    continue;
                }
                return var.uintVal;
            }
            object.Release();
        }
    }

    return false;
}

void ComfyUIManager_Win32::append_log(std::deque<std::string>& logs)
{
    if (!m_new_data_ready.load(std::memory_order_relaxed))
        return;

    std::unique_lock<std::mutex> lock(m_mutex);
    for (auto it = m_logs.begin(); it != m_logs.end(); ++it)
    {
        logs.push_back(std::move(*it));
    }
    m_logs.clear();
    m_new_data_ready.store(false, std::memory_order_relaxed);
}

void ComfyUIManager_Win32::read_logs_th()
{
    LogRAssert(m_hpipe, "Can't capture logs, pipe is not ready", );

    constexpr const uint32_t wait_interval = 100;
    DWORD read = 0;
    char buffer[4096];

    std::string line_accumulator;
    line_accumulator.reserve(10000);

    std::list<std::string> file_logs;
    constexpr const uint32_t log_flush_interval = 5000;
    constexpr const uint32_t log_flush_buffer_size = 10;
    uint32_t log_flush_time_passed = 0;

    std::ofstream logfs;
    logfs.open(get_log_filepath().string(), std::ios::out | std::ios::app);

    const auto flush_to_file = [&logfs, &file_logs]()
    {
        if (logfs.is_open() && !logfs.bad() && !file_logs.empty())
        {
            for (const auto& buff : file_logs)
            {
                logfs << buff;
            }
            logfs.flush();
            file_logs.clear();
        }
    };

    const auto flush_lines = [&line_accumulator, this](const std::string& new_data)
    {
        line_accumulator.append(new_data);

        std::list<std::string> new_lines;
        size_t pos = 0;

        while ((pos = line_accumulator.find('\n')) != std::string::npos)
        {
            std::string line = line_accumulator.substr(0, pos);
            line_accumulator.erase(0, pos + 1);

            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            if (!line.empty())
                new_lines.push_back(std::move(line));
        }

        if (!new_lines.empty())
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_logs.splice(m_logs.end(), new_lines);
            m_new_data_ready.store(true, std::memory_order_relaxed);
        }
    };

    while (!m_kill_server_flag.load(std::memory_order_relaxed))
    {
        DWORD bytes_avail = 0;
        if (!::PeekNamedPipe(m_hpipe.get(), nullptr, 0, nullptr, &bytes_avail, nullptr))
        {
            LogS("Pipe is broken");
            break; 
        }

        if (bytes_avail > 0)
        {
            if (::ReadFile(m_hpipe.get(), buffer, sizeof(buffer), &read, nullptr) && read > 0)
            {
                std::string chunk(buffer, read);
                flush_lines(chunk);

                if (logfs.is_open() && !logfs.bad())
                    file_logs.push_back(std::move(chunk));
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(wait_interval));
            log_flush_time_passed += wait_interval;
        }

        if (log_flush_time_passed > log_flush_interval && !file_logs.empty() ||
            file_logs.size() > log_flush_buffer_size)
        {
            log_flush_time_passed = 0;
            flush_to_file();
        }
    }

    if (!line_accumulator.empty())
    {
        if (!m_kill_server_flag.load(std::memory_order_relaxed))  // comfyui stopped
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_logs.push_back(line_accumulator);
            m_new_data_ready.store(true, std::memory_order_relaxed);
        }

        file_logs.push_back(std::move(line_accumulator));
    }

    flush_to_file();
    if (logfs.is_open() && !logfs.bad())
        logfs.close();

    LogS("Exit. Stop flag: " << m_kill_server_flag.load(std::memory_order_relaxed));
}

} // namespace mngr

#endif // #ifdef _WIN32
