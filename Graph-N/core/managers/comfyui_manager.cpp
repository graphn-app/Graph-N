
#include "core/managers/comfyui_manager.h"
#include "core/managers/settings_manager.h"
#include "ui/graph-n_flow.h"

namespace mngr
{

ComfyUIManager::ComfyUIManager() noexcept = default;
ComfyUIManager::~ComfyUIManager() noexcept = default;

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

    m_log_filepath = fs::get_log_directory_path();
    tools::path::make_folder(m_log_filepath);
    m_log_filepath /= APP_COMFYUI "_" + tools::dt::get_app_start_time().as_filename() + ".log";

    run_server();
}

} // namespace mngr
