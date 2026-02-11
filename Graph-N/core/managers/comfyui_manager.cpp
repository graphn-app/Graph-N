
#include "core/managers/comfyui_manager.h"
#include "core/managers/settings_manager.h"
#include "ui/graph-n_flow.h"

namespace mngr
{

ComfyUIManager::ComfyUIManager() = default;
ComfyUIManager::~ComfyUIManager() = default;

void ComfyUIManager::run()
{
    if (!g::pOptsManager->comfyui_start_server_load())
        return;
    if (g::pOptsManager->comfyui_python_load().empty() || g::pOptsManager->comfyui_main_py_load().empty() || g::pOptsManager->comfyui_workdir_load().empty())
    {
        g::pFlow->handle_comfyui_runserver_error(0, "Can't start ComfyUI server", "Invalid config");
        return;
    }
    if (!tools::path::exists(g::pOptsManager->comfyui_python_load()))
    {
        g::pFlow->handle_comfyui_runserver_error(0, "Can't start ComfyUI server", g::pOptsManager->comfyui_python_load() + " not found");
        return;
    }
    if (!tools::path::exists(g::pOptsManager->comfyui_main_py_load()))
    {
        g::pFlow->handle_comfyui_runserver_error(0, "Can't start ComfyUI server", g::pOptsManager->comfyui_main_py_load().u8string() + " not found");
        return;
    }
    if (!tools::path::exists(g::pOptsManager->comfyui_workdir_load()))
    {
        g::pFlow->handle_comfyui_runserver_error(0, "Can't start ComfyUI server", g::pOptsManager->comfyui_workdir_load().u8string() + " not found");
        return;
    }

    run_server();
}

} // namespace mngr
