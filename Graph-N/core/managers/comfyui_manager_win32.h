
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

    uint32_t check_existing_process() const override;
    bool get_process_started() const override { return m_procid != 0; }
    uint32_t get_started_proc_id() const override { return m_procid; }
    void kill(bool force_kill) override;
    void setup_auto_kill(bool enable) override;
    void append_log(std::list<std::string>& logs) override;

protected:
    void run_server() override;
    bool create_job(bool auto_kill);
    bool apply_job(HANDLE hproc);
    void prepare(STARTUPINFOW& si, bool& capture_log, bool hide_console, Handle& hpipe_wr);
    bool create_process(STARTUPINFOW& si, PROCESS_INFORMATION& pi, bool hide_console, bool capture_logs, bool suspended);
    void enable_autokill(bool enable);
    void read_logs_th();

private:
    Handle m_hjob;
    Handle m_hproc;
    Handle m_hpipe;
    std::thread m_log_th;
    uint32_t m_procid = 0;
    bool m_console_hidden = false;
    std::atomic_bool m_stop_flag = false;
    std::list<std::string> m_logs;
    std::mutex m_mutex;
    std::atomic_bool m_new_data_ready = false;
};

} // namespace mngr

#endif // #ifdef _WIN32
