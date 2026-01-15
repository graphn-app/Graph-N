
#include "ui/panels/panel_wf_list.h"

namespace ui
{

PanelWfList::PanelWfList()
    : PanelBase(DlgObj::PanelWfList, ICON_LC_LIBRARY " Projects", ICON_LC_LIBRARY, "558090508C5B446BB53B6A072EC087E8")
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
    if (begin_panel(opened, focused, data()->clr_bg) && opened)
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
