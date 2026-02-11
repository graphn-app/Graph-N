
#include "ui/panels/panel_output.h"
#include "ui/graph-n_flow.h"

namespace ui
{

PanelOutput::PanelOutput()
    : PanelBase(DlgObj::PanelOutput, ICON_LC_TERMINAL " Server Log", ICON_LC_TERMINAL, DlgObjID[cast::to_szt(DlgObj::PanelOutput)])
{}

PanelOutput::~PanelOutput()
{
    LogF;
}

void PanelOutput::init()
{
    LogF;
    PanelBase::init();

}

void PanelOutput::stop_all_threads()
{
    LogF;
}

DlgCode PanelOutput::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused) && opened)
    {
        show_empty_panel_descr(
            ICON_LC_INFO " Server Log.",
            {},
                {
                    "Real-time stream of the ComfyUI console output.",
                    "Useful for debugging generation errors and monitoring model loading status.",
                    "Panel is disabled. Enable in Settings > " APP_COMFYUI
                },
                "Open settings",
                []()
                {
                    g::pFlow->open_settings_dlg(OptPane::LocalServer);
                }             
                );
    }
    return end_panel(opened);
}

} // namespace ui
