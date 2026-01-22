
#include "ui/panels/panel_output.h"
#include "ui/graph-n_flow.h"

namespace ui
{

PanelOutput::PanelOutput()
    : PanelBase(DlgObj::PanelOutput, ICON_LC_TERMINAL " Server Log", ICON_LC_TERMINAL, "10AAC62E-FEF9-4834-986C-70A5664DE9C4")
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
                    g::pFlow->open_settings_dlg(OptPane::ComfyUi);
                }             
                );
    }
    return end_panel(opened);
}

} // namespace ui
