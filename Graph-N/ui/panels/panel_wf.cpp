
#include "ui/panels/panel_wf.h"
#include "ui/graph-n_flow.h"
#include "core/managers/workflows_manager.h"
#include "core/workflow/workflow.h"
#include "core/workflow/node_instance.h"

namespace ui
{

PanelWorkflow::PanelWorkflow(wid workflow_id, const std::string& name, const std::string& panel_id, ImGuiID dock)
    : PanelBase(DlgObj::PanelWf, name, panel_id, true)
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

    for (auto& node : m_nodes)
    {
        node->m_pos_set = false;
    }
}

void PanelWorkflow::init()
{
    LogF;
    PanelBase::init();

    m_nodes.reserve(100);

    NodeDrawer::Callbacks callbacks;
    callbacks.delete_node = std::bind(&PanelWorkflow::on_delete_node, this, std::placeholders::_1);
    callbacks.copy_node = std::bind(&PanelWorkflow::on_copy_node, this, std::placeholders::_1, std::placeholders::_2);
    callbacks.delete_link = std::bind(&PanelWorkflow::on_delete_link, this, std::placeholders::_1);
    callbacks.create_group = std::bind(&PanelWorkflow::on_create_group, this, std::placeholders::_1);
    callbacks.copy_group = std::bind(&PanelWorkflow::on_copy_group, this, std::placeholders::_1);
    callbacks.delete_group = std::bind(&PanelWorkflow::on_delete_group, this, std::placeholders::_1);
    callbacks.get_all_nodes = std::bind(&PanelWorkflow::on_get_all_nodes, this);
    m_drawer.init(m_wf, callbacks);
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
                        "Right Click or load from " ICON_LC_BOOK_OPEN " Nodes Library to add the first node."
                    }
                );
        }
        else
            draw_wf();
    }

    return end_panel(opened);
}

void PanelWorkflow::add_node(data::Node* node, const std::string& select, std::optional<ImRect> pos)
{
    auto wf = g::pWfsManager->access(m_wf);
    auto node_inst = wf->add_node(node, pos.has_value() ? pos.value() : ImRect(100, 100, -1, -1), select);
    node_inst->calc_node_width(false, false);
    m_nodes.push_back(node_inst);
}

void PanelWorkflow::add_group(std::optional<ImRect> size)
{
    auto wf = g::pWfsManager->access(m_wf);
    auto node_inst = wf->add_group(size.has_value() ? size.value() : ImRect(100, 100, -1, -1), "New group");
    m_nodes.push_back(node_inst);
}

void PanelWorkflow::draw_wf()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiConfigFlags old_flags = io.ConfigFlags;
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

    m_drawer.pre_draw_all(get_panel_id(), m_nodes.size());
    for (size_t node_idx = 0; node_idx < m_nodes.size(); node_idx++)
    {
        auto node = m_nodes[node_idx];
        ImGui::PushID(node);
        m_drawer.pre_draw_node(node);
        if (node->m_group)
            m_drawer.draw_group(node, node_idx);
        else
            m_drawer.draw_node(node, node->get_object()->w_fit, node_idx);
        m_drawer.post_draw_node(node);
        ImGui::PopID();
    }

    if (m_locate != nullptr)
    {
        m_drawer.locate_node(m_locate);
        m_locate = nullptr;
    }

    auto wf = g::pWfsManager->access(m_wf);
    m_drawer.draw_links(wf->get_links());
    m_drawer.query_links();

    m_drawer.post_draw_all();

    ImGui::GetIO().ConfigFlags = old_flags;
}

void PanelWorkflow::on_delete_node(data::NodeInstance* node)
{
    auto wf = g::pWfsManager->access(m_wf);
    if (wf != nullptr)
    {
        for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it)
        {
            if (*it == node)
            {
                m_nodes.erase(it);
                break;
            }
        }
        wf->remove_node(node);        
    }
}

void PanelWorkflow::on_delete_group(data::NodeInstance* group)
{
    auto wf = g::pWfsManager->access(m_wf);
    if (wf != nullptr)
    {
        for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it)
        {
            if (*it == group)
            {
                m_nodes.erase(it);
                break;
            }
        }
        wf->remove_node(group);        
    }
}

void PanelWorkflow::on_delete_link(data::Link* link)
{
    (void)link;
}

void PanelWorkflow::PanelWorkflow::on_copy_node(data::NodeInstance* node, const ImRect& rc)
{
    add_node(node->get_object(), "", rc);
}

void PanelWorkflow::PanelWorkflow::on_copy_group(const ImRect& rc)
{
    add_group(rc);
}

void PanelWorkflow::on_create_group(const ImRect& rc)
{
    add_group(rc);
}

const std::vector<data::NodeInstance*>& PanelWorkflow::on_get_all_nodes() const
{
    return m_nodes;
}

} // namespace ui
