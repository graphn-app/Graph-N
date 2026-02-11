
#ifdef _WIN32
#include "core/managers/comfyui_manager_win32.h"
#include "core/managers/settings_manager.h"
#include "ui/graph-n_flow.h"

namespace mngr
{

ComfyUIManager_Win32::ComfyUIManager_Win32()
    : m_hjob(nullptr, ::CloseHandle)
    , m_hpipe(nullptr, ::CloseHandle)
{}

ComfyUIManager_Win32::~ComfyUIManager_Win32() = default;

bool ComfyUIManager_Win32::init()
{
    LogF;
    if (!ComfyUIManager::init())
        return false;

    return true;
}

void ComfyUIManager_Win32::free()
{
    LogF;
    ComfyUIManager::free();
}

void ComfyUIManager_Win32::stop_all_threads()
{
    LogF;
    ComfyUIManager::stop_all_threads();
}

void ComfyUIManager_Win32::run_server()
{
    const auto& mainpy = g::pOptsManager->comfyui_main_py_load();
    const auto& python = g::pOptsManager->comfyui_python_load();
    const auto& args = g::pOptsManager->comfyui_args_load();
    const auto& workdir = g::pOptsManager->comfyui_workdir_load();

    bool capture_log = g::pOptsManager->comfyui_capture_output_load();
    bool kill_server = g::pOptsManager->comfyui_kill_server_load();
    bool hide_console = false;
    if (kill_server)
        hide_console = g::pOptsManager->comfyui_hide_console_load();

    Handle hpipe_wr(nullptr, ::CloseHandle);
    if (kill_server)
    {
        if (!create_job())
            hide_console = false;
    }

    PROCESS_INFORMATION pi = {};
    STARTUPINFOA si;
    prepare(si, capture_log, hide_console, hpipe_wr);

    if (!create_process(si, pi, python, mainpy.u8string(), args, workdir.u8string(), hide_console))
    {
        m_hpipe.reset();
        m_hjob.reset();
        hpipe_wr.reset();
    }
    else
    {
        bool job = false;
        if (kill_server)
            job = apply_job(pi.hProcess, pi.hThread);
        if (kill_server && !job)
        {
            ::TerminateProcess(pi.hProcess, 0);
            g::pOptsManager->comfyui_kill_server_save(false);
            run_server();
            g::pOptsManager->comfyui_kill_server_save(true);
        }
        ::CloseHandle(pi.hThread);
        ::CloseHandle(pi.hProcess);
    }
}

bool ComfyUIManager_Win32::create_job()
{
    Handle hjob(::CreateJobObject(nullptr, nullptr), ::CloseHandle);
    if (!hjob)
    {
        LogWarn("Failed to create job, err: " << LogERR << ". " << APP_COMFYUI << " will not be killed om app exit");
        g::pFlow->handle_comfyui_runserver_error(::GetLastError(), "Run " APP_COMFYUI, APP_COMFYUI " will not be killed om app exit");
        return false;
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {};
    jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

    if (!::SetInformationJobObject(hjob.get(), JobObjectExtendedLimitInformation, &jeli, sizeof(jeli))) 
    {
        LogWarn("Failed to set information on job, err: " << LogERR << ". " << APP_COMFYUI << " will not be killed om app exit");
        g::pFlow->handle_comfyui_runserver_error(::GetLastError(), "Run " APP_COMFYUI, APP_COMFYUI " will not be killed om app exit");
        return false;
    }

    m_hjob = std::move(hjob);
    return true;
}

bool ComfyUIManager_Win32::apply_job(HANDLE hproc, HANDLE hthread)
{
    if (!::AssignProcessToJobObject(m_hjob.get(), hproc))
    {
        LogWarn("Failed to assign process to job, err: " << LogERR << ". " << APP_COMFYUI " will not be killed om app exit");
        m_hjob.reset();
        g::pFlow->handle_comfyui_runserver_error(::GetLastError(), "Run " APP_COMFYUI, APP_COMFYUI " will not be killed om app exit");
        return false;
    }

    BOOL result;
    if (!::IsProcessInJob(hproc, m_hjob.get(), &result) || !result)
    {
        LogWarn("Process assigned to job, but verification failed, err: " << LogERR << ". " << APP_COMFYUI " will not be killed om app exit");
        m_hjob.reset();
        g::pFlow->handle_comfyui_runserver_error(::GetLastError(), "Run " APP_COMFYUI, APP_COMFYUI " will not be killed om app exit");
        return false;
    }
    
    ::ResumeThread(hthread);
    return true;
}

void ComfyUIManager_Win32::prepare(STARTUPINFOA& si, bool& capture_log, bool hide_console, Handle& hpipe_wr)
{
    if (capture_log)
    {
        SECURITY_ATTRIBUTES sa_attr = {};
        sa_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa_attr.bInheritHandle = TRUE;
        sa_attr.lpSecurityDescriptor = NULL;

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
                LogWarn("Can't set pipe handle information, err: " << LogERR << ". Capture will be disabled");
                ::CloseHandle(hrd);
                ::CloseHandle(hwr);
                capture_log = false;
                g::pFlow->handle_comfyui_runserver_error(::GetLastError(), "Run " APP_COMFYUI, "Capture will be disabled");
            }
        }
        else
        {
            LogWarn("Can't create pipe, err: " << LogERR << ". Capture will be disabled");
            capture_log = false;
            g::pFlow->handle_comfyui_runserver_error(::GetLastError(), "Run " APP_COMFYUI, "Output capture will be disabled");
        }
    }

    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.hStdError = (capture_log ? hpipe_wr.get() : nullptr);
    si.hStdOutput = (capture_log ? hpipe_wr.get() : nullptr);
    si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = (hide_console ? SW_HIDE : SW_SHOWNORMAL);
}

bool ComfyUIManager_Win32::create_process(
    STARTUPINFOA& si, PROCESS_INFORMATION& pi, const std::string& python, const std::string& mainpy, const std::string& cmd, const std::string& workdir, bool hide_console)
{
    std::filesystem::path path(mainpy);

    const auto command = fmt::format(FMT_COMPILE("\"{}\" -u -s \"{}\" {}"), python, mainpy, cmd);
    std::vector<char> cmd_vec(command.begin(), command.end());
    cmd_vec.push_back(0);

    if (!::CreateProcessA(NULL, cmd_vec.data(), NULL, NULL, TRUE, (hide_console ? CREATE_NO_WINDOW : 0), NULL, workdir.c_str(), &si, &pi))
    {
        LogWarn("Failed to create " APP_COMFYUI " process, err: " << LogERR);
        g::pFlow->handle_comfyui_runserver_error(::GetLastError(), "Run " APP_COMFYUI, "Failed to create process");
        return false;
    }

    return true;
}

} // namespace mngr

#endif // #ifdef _WIN32
