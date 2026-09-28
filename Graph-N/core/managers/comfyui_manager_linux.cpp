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

void ComfyUIManager_Linux::run_server()
{
#error Not implemented
}

} // namespace mngr

#endif // #ifdef __linux__
