
#include "ui/panels/panel_wf.h"
#include "ui/graph-n_flow.h"
#include "core/managers/workflows_manager.h"
#include "core/workflow/workflow.h"

namespace ui
{

PanelWorkflow::PanelWorkflow(wid workflow_id, const std::string& name, const std::string& panel_id, ImGuiID dock)
    : PanelBase(DlgObj::PanelWf, (ICON_LC_WORKFLOW " " + name).c_str(), ICON_LC_WORKFLOW, panel_id.c_str(), true)
    , m_wf(workflow_id)
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

void PanelWorkflow::restore_pos()
{
    m_restore_pos = true;
}

void PanelWorkflow::rename_panel(const std::string& new_name)
{
    set_name(new_name);
}

DlgCode PanelWorkflow::show(bool& focused)
{
    bool opened = true;
    ImGui::SetNextWindowDockID(m_dock, m_restore_pos ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
    m_restore_pos = false;
    if (begin_panel(opened, focused, false, m_active) && opened)
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

void PanelWorkflow::add_node(data::Node* node)
{
    auto wf = g::pWfsManager->access(m_wf);
    wf->add_node(node);
}

} // namespace ui
