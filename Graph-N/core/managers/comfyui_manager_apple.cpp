#ifdef __APPLE__
#include "core/managers/comfyui_manager_apple.h"
#include "core/managers/settings_manager.h"

namespace mngr
{

ComfyUIManager_Apple::ComfyUIManager_Apple() noexcept = default;
ComfyUIManager_Apple::~ComfyUIManager_Apple() noexcept = default;

void ComfyUIManager_Apple::stop_all_threads()
{
}

bool ComfyUIManager_Apple::init()
{
    if (!ComfyUIManager::init())
        return false;

    return true;
}

void ComfyUIManager_Apple::free()
{
}

void ComfyUIManager_Apple::run_server()
{
#error Not implemented
}

} // namespace mngr

#endif // #ifdef __APPLE__
