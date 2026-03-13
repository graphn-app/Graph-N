
#include "ui/panels/panel_wf.h"
#include "ui/graph-n_flow.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/nodes_manager.h"
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
    m_wf = nullptr;
    m_drawer.free();
}

void PanelWorkflow::init()
{
    LogF;
    PanelBase::init();

    m_drawer.init();

    auto& nodes = m_wf->get_nodes();
    for (auto& node : nodes)
    {
        node->reset_state();

        if (!node->m_group)
            node->calc_node_width(false, false);
    }

    auto& links = m_wf->get_links();
    for (auto& link : links)
    {
        link->reset_state();
    }

    auto& pins = m_wf->get_pins();
    for (auto& pin : pins)
    {
        pin->reset_state();
    }
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

void PanelWorkflow::set_wf_view(std::optional<ImRect> wf_rc, std::optional<float> wf_zoom)
{
    if (wf_rc.has_value())
        m_drawer.set_viewport_rect(wf_rc.value());
    if (wf_zoom.has_value())
        m_drawer.set_zoom(wf_zoom.value());
}

void PanelWorkflow::draw_wf()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiConfigFlags old_flags = io.ConfigFlags;
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

    auto& nodes = m_wf->get_nodes();
    m_drawer.pre_draw_all(get_panel_id(), nodes.size());

    if (m_select_new_nodes)
        m_drawer.select_node({}, false);

    for (size_t node_idx = 0; node_idx < nodes.size(); node_idx++)
    {
        auto node = nodes[node_idx].get();

        const bool is_new = !node->ed();
        if (is_new)
        {
            node->reset_state();
            if (!node->m_group)
                node->calc_node_width(false, false);
        }

        ImGui::PushID(node);
        m_drawer.pre_draw_node(node);
        if (node->m_group)
            m_drawer.draw_group(node, node_idx);
        else
            m_drawer.draw_node(node, node->get_object()->w_fit, node_idx);
        m_drawer.post_draw_node(node);
        ImGui::PopID();

        if (is_new && m_select_new_nodes)
            m_drawer.select_node(node->ed(), true);
    }

    m_select_new_nodes = false;

    if (m_locate != nullptr)
    {
        m_drawer.locate_node(m_locate);
        m_locate = nullptr;
    }

    m_drawer.draw_links(m_wf->get_links());
    m_drawer.query_new_item(
        [this](data::PinInstance* pin) { unlink(pin); },
        [this, wf](data::PinInstance* pin, const ImVec2& pt)
            {
                auto node = wf->get_node_by_instance(pin->get_parent_node());
                m_menu_objs = {};
                m_menu_objs.click_pos = pt;
                m_menu_objs.pins.insert(pin);
                m_menu_objs.nodes.insert(node);
                m_context_menu_type = MenuType::SpawnNode;
                m_handle_context_menu = true;
            }
    );

    m_drawer.post_draw_all(
        [this]() { return context_menu_handler(); },
        [this]() { return show_context_menu(); },
        [this]() { return handle_moves(); });

    ImGui::GetIO().ConfigFlags = old_flags;

    handle_dnd();
}

void PanelWorkflow::fit(std::map<data::NodeInstance*, std::set<data::NodeInstance*>>& items)
{
    if (items.empty())
    {
        for (auto& node : m_wf->get_nodes())
        {
            if (node->m_group)
            {
                items.insert({ node.get(), m_drawer.get_nodes_in_group(node.get(), m_wf->get_nodes()) });
            }
            else
            {
                items.insert({ node.get(), {} });
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

void PanelWorkflow::handle_dnd()
{
    if (ImGui::BeginDragDropTarget())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(DND_FROM_NODES_LIBRARY);
        if (payload != nullptr)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
            const hid node_id = *(const hid*)payload->Data;
            auto node = g::pNodesManager->find_node(node_id);            
            auto inst = add_node(node, "", ImRect(m_drawer.get_mouse_pos(), ImVec2(-1, -1)));
            Log("Node " << inst->get_name() << " dropped");
            g::pFlow->wf_activate(m_wf);
        }
        ImGui::EndDragDropTarget();
    }
}

} // namespace ui
