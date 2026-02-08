
#include "ui/panels/panel_templates.h"

namespace ui
{

PanelTemplates::PanelTemplates()
    : PanelBase(DlgObj::PanelTemplates, ICON_LC_LAYOUT_TEMPLATE " Templates", ICON_LC_LAYOUT_TEMPLATE, DlgObjID[cast::to_szt(DlgObj::PanelTemplates)])
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
