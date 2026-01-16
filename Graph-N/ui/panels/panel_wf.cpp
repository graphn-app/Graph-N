
#include "ui/panels/panel_wf.h"
#include "ui/graph-n_flow.h"

namespace ui
{

PanelWorkflow::PanelWorkflow(data::Workflow* wf, const char* name, const char* id, ImGuiID& dock)
    : PanelBase(DlgObj::PanelWf, name, ICON_LC_WORKFLOW, id, true)
    , m_wf(wf)
    , m_dock(dock)
{}

PanelWorkflow::~PanelWorkflow()
{
    LogF;
}

void PanelWorkflow::stop_all_threads()
{
    LogF;
}

void PanelWorkflow::init()
{
    LogF;
    PanelBase::init();

}

DlgCode PanelWorkflow::show(bool& focused)
{
    bool opened = true;
    ImGui::SetNextWindowDockID(m_dock, ImGuiCond_FirstUseEver);
    if (begin_panel(opened, focused, data()->clr_bg, false) && opened)
    {

        show_empty_panel_descr(
            "",
            {},
                {
                    "Empty Canvas.",
                    "Press Space, Right Click or load from " ICON_LC_BOOK_OPEN " Nodes Library to add the first node.",
                    "Drag and drop an image or .json file here to load a workflow."
                }                
                );
    }

    return end_panel(opened);
}

} // namespace ui
