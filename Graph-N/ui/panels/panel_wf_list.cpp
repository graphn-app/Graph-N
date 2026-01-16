
#include "ui/panels/panel_wf_list.h"

namespace ui
{

PanelWfList::PanelWfList()
    : PanelBase(DlgObj::PanelWfList, ICON_LC_LIBRARY " Projects", ICON_LC_LIBRARY, "0EE80880-D72D-48A6-808A-187FE7A17A71")
{}

PanelWfList::~PanelWfList()
{
    LogF;
}

void PanelWfList::init()
{
    LogF;
    PanelBase::init();

}

void PanelWfList::stop_all_threads()
{
    LogF;
}

DlgCode PanelWfList::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused, data()->clr_bg, false) && opened)
    {
        show_empty_panel_descr(
            ICON_LC_INFO " My Projects.",
            {},
                {
                    "Your library of saved graphs.",
                    "Here you can open, rename, and delete projects,",
                    "or convert successful experiments into templates.",
                    "",
                    "Use the main menu to export to .json"
                }                
                );
    }
    return end_panel(opened);
}

} // namespace ui
