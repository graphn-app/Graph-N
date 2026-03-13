
#ifdef __APPLE__
#include "core/managers/comfyui_manager_apple.h"
#include "core/managers/settings_manager.h"

namespace mngr
{

ComfyUIManager_Apple::ComfyUIManager_Apple() = default;
ComfyUIManager_Apple::~ComfyUIManager_Apple() = default;

void ComfyUIManager_Apple::stop_all_threads()
{
    LogF;
}

bool ComfyUIManager_Apple::init()
{
    LogF;
    if (!ComfyUIManager::init())
        return false;

    return true;
}

void ComfyUIManager_Apple::free()
{
    LogF;
}

void ComfyUIManager_Apple::run_server()
{
}

} // namespace mngr

#endif // #ifdef __APPLE__
