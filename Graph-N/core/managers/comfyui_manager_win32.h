
#pragma once
#ifdef _WIN32
#include "core/managers/comfyui_manager.h"

namespace mngr
{

using Handle = std::unique_ptr<void, decltype(&::CloseHandle)>;

class ComfyUIManager_Win32 final : public ComfyUIManager
{
public:
    ComfyUIManager_Win32();
    ~ComfyUIManager_Win32() override;

    bool init() override;
    void free() override;
    void stop_all_threads() override;

protected:
    void run_server() override;
    bool create_job();
    bool apply_job(HANDLE hproc, HANDLE hthread);
    void prepare(STARTUPINFOA& si, bool& capture_log, bool hide_console, Handle& hpipe_wr);
    bool create_process(STARTUPINFOA& si, PROCESS_INFORMATION& pi, const std::string& python, const std::string& mainpy, const std::string& cmd, const std::string& workdir, bool hide_console);

private:
    Handle m_hjob;
    Handle m_hpipe;
};

} // namespace mngr

#endif // #ifdef _WIN32
