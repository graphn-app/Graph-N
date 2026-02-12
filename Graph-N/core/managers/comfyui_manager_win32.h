
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

    bool check_existing_process() const override;
    bool get_process_started() const override { return m_procid != 0; }
    uint32_t get_started_proc_id() const override { return m_procid; }
    void kill() override;

protected:
    void run_server(bool force_start) override;
    bool create_job();
    bool apply_job(HANDLE hproc, HANDLE hthread);
    void prepare(STARTUPINFOA& si, bool& capture_log, bool hide_console, Handle& hpipe_wr);
    bool create_process(STARTUPINFOA& si, PROCESS_INFORMATION& pi, const std::string& python, const std::string& mainpy, const std::string& cmd, const std::string& workdir, bool hide_console);

private:
    Handle m_hjob;
    Handle m_hpipe;
    uint32_t m_procid = 0;
};

} // namespace mngr

#endif // #ifdef _WIN32
