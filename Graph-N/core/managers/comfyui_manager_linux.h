/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#ifdef __linux__
#include "core/managers/comfyui_manager.h"

namespace mngr
{

class ComfyUIManager_Linux final : public ComfyUIManager
{
public:
    ComfyUIManager_Linux() noexcept;
    ~ComfyUIManager_Linux() noexcept override;

    bool init() override;
    void stop_and_cleanup() override;

    uint32_t check_existing_process() const noexcept override;
    bool get_process_started() const noexcept override;
    uint32_t get_started_proc_id() const noexcept override;
    void kill(bool force_kill) override;
    void setup_auto_kill(bool enable) override;
    void append_log(std::deque<std::string>& logs) override;

protected:
    void run_server() override;
};

} // namespace mngr

#endif // #ifdef __linux__
