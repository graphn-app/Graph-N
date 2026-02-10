
#include "core/managers/comfyui_manager.h"
#include "core/managers/settings_manager.h"

namespace mngr
{

ComfyUIManager::ComfyUIManager() = default;
ComfyUIManager::~ComfyUIManager() = default;

void ComfyUIManager::stop_all_threads()
{
    LogF;
}

bool ComfyUIManager::init()
{
    LogF;

    return true;
}

void ComfyUIManager::free()
{
    LogF;
}

void ComfyUIManager::run()
{
}

} // namespace mngr
