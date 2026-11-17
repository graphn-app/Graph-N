/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#ifdef __linux__
#include "core/managers/comfyui_manager_linux.h"
#include "core/managers/settings_manager.h"

namespace mngr
{

ComfyUIManager_Linux::ComfyUIManager_Linux() noexcept = default;
ComfyUIManager_Linux::~ComfyUIManager_Linux() noexcept = default;

void ComfyUIManager_Linux::stop_and_cleanup()
{
}

bool ComfyUIManager_Linux::init()
{
    if (!ComfyUIManager::init())
        return false;

    return true;
}

uint32_t ComfyUIManager_Linux::check_existing_process() const noexcept
{
    return 0;
}

bool ComfyUIManager_Linux::get_process_started() const noexcept
{
    return false;
}

uint32_t ComfyUIManager_Linux::get_started_proc_id() const noexcept
{
    return 0;
}

void ComfyUIManager_Linux::kill(bool /*force_kill*/)
{
}

void ComfyUIManager_Linux::setup_auto_kill(bool /*enable*/)
{
}

void ComfyUIManager_Linux::append_log(std::deque<std::string>& /*logs*/)
{
}

void ComfyUIManager_Linux::run_server()
{
    // ComfyUI server integration is a Windows-only feature for now.
    // On Linux this stays a no-op stub.
}

} // namespace mngr

#endif // #ifdef __linux__
