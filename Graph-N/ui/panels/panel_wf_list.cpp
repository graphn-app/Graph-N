
#include "ui/panels/panel_wf_list.h"
#include "core/workflow/workflow.h"

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
    if (begin_panel(opened, focused, data()->clr_bg) && opened)
    {
        if (m_wfs.empty())
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
        else
            show_wfs();
    }
    return end_panel(opened);
}

void PanelWfList::add_wf(data::Workflow* wf)
{
    m_wfs.push_back(wf);
}

void PanelWfList::remove_wf(data::Workflow* wf)
{
    m_wfs.remove_if([wf](data::Workflow* pwf) { return wf->get_id() == pwf->get_id(); });
}

void PanelWfList::show_wfs()
{}

} // namespace ui
