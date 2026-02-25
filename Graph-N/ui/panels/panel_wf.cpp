
#include "ui/panels/panel_wf.h"
#include "ui/graph-n_flow.h"
#include "core/managers/workflows_manager.h"
#include "core/workflow/workflow.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"

namespace ui
{

PanelWorkflow::PanelWorkflow(wid workflow_id, const std::string& name, const std::string& panel_id, ImGuiID dock)
    : PanelBase(DlgObj::PanelWf, name, panel_id, true)
    , m_wf(workflow_id)
    , m_dock(dock)
    , m_context_menuid(panel_id + "_ContextMenu")
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
    m_drawer.init(m_wf);

    auto wf = g::pWfsManager->access(m_wf);

    auto& nodes = wf->get_nodes();
    for (auto& node : nodes)
    {
        node->reset_state();

        if (!node->m_group)
            node->calc_node_width(false, false);
        m_nodes.push_back(node.get());
    }

    auto& links = wf->get_links();
    for (auto& link : links)
    {
        link->reset_state();
    }

    auto& pins = wf->get_pins();
    for (auto& pin : pins)
    {
        pin->reset_state();
    }
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
        //if (m_nodes.empty())
        //{
        //    show_empty_panel_descr(
        //        "",
        //        {},
        //            {
        //                "Empty Canvas.",
        //                "Right Click or load from " ICON_LC_BOOK_OPEN " Nodes Library to add the first node."
        //            }
        //        );
        //}
        //else
            draw_wf();
    }

    return end_panel(opened);
}

data::NodeInstance* PanelWorkflow::add_node(data::Node* node, const std::string& select, std::optional<ImRect> pos)
{
    auto wf = g::pWfsManager->access(m_wf);
    auto node_inst = wf->add_node(node, pos.has_value() ? pos.value() : ImRect(100, 100, -1, -1), select);
    node_inst->calc_node_width(false, false);
    m_nodes.push_back(node_inst);
    return node_inst;
}

data::NodeInstance* PanelWorkflow::add_group(std::optional<ImRect> size)
{
    auto wf = g::pWfsManager->access(m_wf);
    auto group_inst = wf->add_group(size.has_value() ? size.value() : ImRect(100, 100, -1, -1), "New group");
    m_nodes.push_back(group_inst);
    return group_inst;
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

    m_drawer.post_draw_all([this]() { return context_menu_handler(); }, [this]() { return show_context_menu(); });

    ImGui::GetIO().ConfigFlags = old_flags;
}

void PanelWorkflow::delete_node(data::NodeInstance* node)
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

void PanelWorkflow::delete_group(data::NodeInstance* group)
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

void PanelWorkflow::delete_link(data::Link* link)
{
    (void)link; // TODO
}

void PanelWorkflow::create_group(const ImRect& rc)
{
    add_group(rc);
}

void PanelWorkflow::fit(std::map<data::NodeInstance*, std::set<data::NodeInstance*>>& items)
{
    if (items.empty())
    {
        for (auto node : m_nodes)
        {
            if (node->m_group)
            {
                items.insert({ node, m_drawer.get_nodes_in_group(node, m_nodes) });
            }
            else
            {
                items.insert({ node, {} });
            }
        }
    }

    for (auto it = items.begin(); it != items.end(); )
    {
        if (!it->first->m_group)
        {
            it->first->reset_width();
            it = items.erase(it);
        }
        else if (it->second.empty())
        {
            it = items.erase(it);
        }
        else
            ++it;
    }

    std::set<data::NodeInstance*> add_y;
    for (auto it = items.begin(); it != items.end(); ++it)
    {
        auto& nodes = it->second;
        for (auto node : nodes)
        {
            if (node != it->first && node->m_group && items.find(node) != items.end())
            {
                if (m_drawer.fit_group_rect(node, m_drawer.get_nodes_in_group(node, m_nodes)))
                    add_y.insert(it->first);
                items.erase(node);
            }
        }
    }

    for (auto& [group, nodes] : items)
    {
        m_drawer.fit_group_rect(group, nodes);
        if (add_y.find(group) != add_y.end())
            group->m_rc.Max.y += ImGui::GetTextLineHeight();
    }
}

} // namespace ui
