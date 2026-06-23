
#include "ui/panels/panel_templates.h"

namespace ui
{

PanelTemplates::PanelTemplates()
    : PanelBase(DlgObj::PanelTemplates, "Templates")
{}

PanelTemplates::~PanelTemplates()
{
    LogF;
}

void PanelTemplates::init()
{
    LogF;
    PanelBase::init();

}

void PanelTemplates::stop_all_threads()
{
    LogF;
    // handle g::app_global_stop_all_threads
}

DlgCode PanelTemplates::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused) && opened)
    {
        show_empty_panel_descr(
            ICON_LC_INFO " Templates.",
            {},
                {
                    "Speed up your workflow",
                    "using ready-made solutions.",
                    "You can save the current graph",
                    "as a template for future projects.",
                    "The list is currently empty.",
                },
                "Load default templates",
                []()
                {

                }
                );
    }
    return end_panel(opened);
}

} // namespace ui
