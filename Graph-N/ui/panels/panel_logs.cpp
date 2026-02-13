
#include "ui/panels/panel_logs.h"
#include "ui/graph-n_flow.h"
#include "core/managers/comfyui_manager.h"

namespace ui
{

PanelServerLogs::PanelServerLogs()
    : PanelBase(DlgObj::PanelServerLogs, ICON_LC_TERMINAL " Server Logs", ICON_LC_TERMINAL, DlgObjID[cast::to_szt(DlgObj::PanelServerLogs)])
{}

PanelServerLogs::~PanelServerLogs()
{
    LogF;
}

void PanelServerLogs::init()
{
    LogF;
    PanelBase::init();

}

void PanelServerLogs::stop_all_threads()
{
    LogF;
}

DlgCode PanelServerLogs::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused) && opened)
    {
        if (!g::pComfyUIManager->get_process_started())
        {
            show_empty_panel_descr(
                ICON_LC_INFO " Server Log.",
                {},
                {
                    "Real-time stream of the ComfyUI console output.",
                    "Useful for debugging generation errors and monitoring model loading status.",
                    "Logs capturing is disabled or Server is not running.",
                    "Enable in Settings > " APP_COMFYUI
                },
                "Open settings",
                []()
                {
                    g::pFlow->open_settings_dlg(OptPane::LocalServer);
                }
            );
        }
        else
        {
            show_toolbar();
            show_logs();
        }
    }
    return end_panel(opened);
}

void PanelServerLogs::show_toolbar()
{}

void PanelServerLogs::show_logs()
{
    g::pComfyUIManager->append_log(m_logs);


}

} // namespace ui
