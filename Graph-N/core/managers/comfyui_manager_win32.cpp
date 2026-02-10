
#ifdef _WIN32
#include "core/managers/comfyui_manager_win32.h"
#include "core/managers/settings_manager.h"

namespace mngr
{

ComfyUIManager_Win32::ComfyUIManager_Win32() = default;
ComfyUIManager_Win32::~ComfyUIManager_Win32() = default;

void ComfyUIManager_Win32::stop_all_threads()
{
    LogF;
}

bool ComfyUIManager_Win32::init()
{
    LogF;

    return true;
}

void ComfyUIManager_Win32::free()
{
    LogF;
}

bool ComfyUIManager_Win32::run_server()
{
    return true;
}

} // namespace mngr

#endif // #ifdef _WIN32
