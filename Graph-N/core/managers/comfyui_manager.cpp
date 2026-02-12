
#include "core/managers/comfyui_manager.h"
#include "core/managers/settings_manager.h"
#include "ui/graph-n_flow.h"

namespace mngr
{

ComfyUIManager::ComfyUIManager() = default;
ComfyUIManager::~ComfyUIManager() = default;

void ComfyUIManager::run(bool force_start)
{
    if (!force_start && !g::pOptsManager->comfyui_start_server_load())
        return;
    if (!tools::path::exists(g::pOptsManager->comfyui_python_load()) || !g::pOptsManager->comfyui_python_load().is_absolute() ||
        !tools::path::exists(g::pOptsManager->comfyui_main_py_load()) || !g::pOptsManager->comfyui_main_py_load().is_absolute() ||
        !tools::path::exists(g::pOptsManager->comfyui_workdir_load()) || !g::pOptsManager->comfyui_workdir_load().is_absolute())
    {
        g::pFlow->handle_comfyui_runserver_error(0, "Can't start ComfyUI server", "Invalid config");
        return;
    }

    run_server(force_start);
}

void ComfyUIManager::kill()
{}

} // namespace mngr
