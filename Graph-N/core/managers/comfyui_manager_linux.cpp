
#ifdef __linux__
#include "core/managers/comfyui_manager_linux.h"
#include "core/managers/settings_manager.h"

namespace mngr
{

ComfyUIManager_Linux::ComfyUIManager_Linux() = default;
ComfyUIManager_Linux::~ComfyUIManager_Linux() = default;

void ComfyUIManager_Linux::stop_all_threads()
{
    LogF;
}

bool ComfyUIManager_Linux::init()
{
    LogF;

    return true;
}

void ComfyUIManager_Linux::free()
{
    LogF;
}

void ComfyUIManager_Linux::run_server()
{
}

} // namespace mngr

#endif // #ifdef __linux__
