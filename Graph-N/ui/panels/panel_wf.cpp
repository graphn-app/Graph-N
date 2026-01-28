
#include "ui/panels/panel_wf.h"
#include "ui/graph-n_flow.h"
#include "core/managers/workflows_manager.h"
#include "core/workflow/workflow.h"
#include "core/workflow/node_instance.h"

namespace ui
{

PanelWorkflow::PanelWorkflow(wid workflow_id, const std::string& name, const std::string& panel_id, ImGuiID dock)
    : PanelBase(DlgObj::PanelWf, (ICON_LC_WORKFLOW " " + name).c_str(), ICON_LC_WORKFLOW, panel_id.c_str(), true)
    , m_wf(workflow_id)
    , m_dock(dock)
{
}

PanelWorkflow::~PanelWorkflow()
{
    LogF;
}

void PanelWorkflow::stop_all_threads()
{
    LogF;
}

void PanelWorkflow::free()
{
    LogF;
    m_wf = 0;
    m_drawer.free();
}

void PanelWorkflow::init()
{
    LogF;
    PanelBase::init();

    m_nodes.reserve(100);
    m_drawer.init();
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
        if (m_nodes.empty())
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
        else
            draw_wf();
    }

    return end_panel(opened);
}

void PanelWorkflow::add_node(data::Node* node, std::optional<ImVec2> /*pos*/)
{
    auto wf = g::pWfsManager->access(m_wf);
    auto node_inst = wf->add_node(node);
    node_inst->calc_node_width(false /*ro*/);

    // TODO
    //if (pos.has_value())
    //    node_inst->move(pos.value());
    //else
    //    node_inst->move(ImVec2(100, 100));
    m_nodes.push_back(node_inst);
}

void PanelWorkflow::draw_wf()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiConfigFlags old_flags = io.ConfigFlags;
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

    m_drawer.pre_draw_all(get_panel_id());
    for (auto node : m_nodes)
    {
        ImGui::PushID(node);
        m_drawer.pre_draw_node(node);
        m_drawer.draw_node(node, node->get_object()->w_fit);
        m_drawer.post_draw_node(node);
        ImGui::PopID();
    }

    auto wf = g::pWfsManager->access(m_wf);
    m_drawer.draw_links(wf->get_links());
    m_drawer.query_links();

    m_drawer.post_draw_all();

    ImGui::GetIO().ConfigFlags = old_flags;
}

} // namespace ui
