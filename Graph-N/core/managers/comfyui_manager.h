
#pragma once
#include "core/managers/manager.h"

namespace mngr
{

class ComfyUIManager : public Manager
{
public:
    ComfyUIManager() noexcept;
    ~ComfyUIManager() noexcept override;

    void run(bool force_start);
    const std::filesystem::path& get_log_filepath() const noexcept { return m_log_filepath; }

    virtual uint32_t check_existing_process() const = 0;
    virtual bool get_process_started() const noexcept = 0;
    virtual uint32_t get_started_proc_id() const noexcept = 0;
    virtual void kill(bool force_kill) = 0;
    virtual void setup_auto_kill(bool enable) = 0;
    virtual void append_log(std::deque<std::string>& logs) = 0;

protected:
    virtual void run_server() = 0;

private:
    std::filesystem::path m_log_filepath;
};

} // namespace mngr
