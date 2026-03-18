
#include "core/workflow/workflow_action.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/history_manager.h"
#include "core/workflow/workflow.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "ui/node_drawer.h"
#include "ui/graph-n_flow.h"

namespace data
{

WorkflowAction::WorkflowAction(data::Workflow* wf)
    : m_wf(wf)
{
    LogF;
}

WorkflowAction::~WorkflowAction()
{
    LogF;
}

const blob::Blob* WorkflowAction::get_drawer_state() const
{
    if (nullptr == m_drawer)
        return nullptr;
    return &m_drawer->get_state();
}

void WorkflowAction::multi_action_on(const char* action_name) const
{
    g::pHistoryManager->multi_action_begin(m_wf->get_id(), action_name);
}

void WorkflowAction::multi_action_off() const
{
    g::pHistoryManager->multi_action_end(m_wf->get_id());
}

data::NodeInstance* WorkflowAction::add_node(data::Node* node, const std::string& select, std::optional<ImRect> pos)
{
    const auto node_inst = m_wf->add_node(node, (pos.has_value() ? pos.value() : ImRect(m_drawer->get_visible_pos(ImVec2(100, 100)), ImVec2(-1, -1))), select);
    node_inst->calc_node_width(false, false);
    g::pHistoryManager->push_node_action(node_inst, mngr::HistoryActionType::NodeAdd);
    return node_inst;
}

data::NodeInstance* WorkflowAction::add_group(std::optional<ImRect> size)
{
    const auto group_inst = m_wf->add_group(size.has_value() ? size.value() : ImRect(100, 100, -1, -1), "New group");
    g::pHistoryManager->push_node_action(group_inst, mngr::HistoryActionType::NodeAdd);
    return group_inst;
}

void WorkflowAction::delete_node(data::NodeInstance* node)
{
    m_moving = false;
    m_moving_nodes.clear();

    g::pWfsManager->copy_del(node);
    g::pFlow->txt_panel_delete(m_wf->get_id(), node);

    m_drawer->delete_node(node);
    node->ed().reset();

    multi_action_on(node->m_group ? "Remove group" : "Remove node");

    std::list<Link*> links;
    const auto get_links = [&links, this](const std::vector<PinInstance*>& pins)
    {
        for (auto pin : pins)
        {
            auto add_links = m_wf->get_links_by_pin(pin);
            links.splice(links.end(), add_links);
        }
    };
    get_links(node->get_inputs());
    get_links(node->get_outputs());

    for (auto link : links)
    {
        unlink(link);
    }

    g::pHistoryManager->push_node_action(node, mngr::HistoryActionType::NodeDel);
    m_wf->remove_node(node);
    multi_action_off();
}

void WorkflowAction::delete_group(data::NodeInstance* group)
{
    return delete_node(group);
}

void WorkflowAction::hard_reset()
{
    g::pFlow->txt_panel_clear(m_wf->get_id());
    if (m_drawer != nullptr)
        m_drawer->free();
}

data::Link* WorkflowAction::link(data::PinInstance* p1, data::PinInstance* p2, iid instance_id /*= 0*/)
{
    const auto link = data::Link::link(p1, p2, instance_id);
    g::pHistoryManager->push_link_action(link, mngr::HistoryActionType::LinkAdd);
    return link;
}

void WorkflowAction::unlink(data::PinInstance* pin)
{
    multi_action_on("Unlink");

    const auto& links = m_wf->get_links();
    for (auto it = links.begin(); it != links.end(); )
    {
        auto& link = *it;
        if (link->is_linked_to(pin))
        {
            unlink(link.get());
            it = links.begin();
        }
        else
            ++it;
    }

    multi_action_off();
}

void WorkflowAction::unlink(data::PinInstance* p1, data::PinInstance* p2)
{
    multi_action_on("Unlink");

    const auto& links = m_wf->get_links();
    for (auto it = links.begin(); it != links.end(); )
    {
        auto& link = *it;
        if (link->is_linked_to(p1) && link->is_linked_to(p2))
        {
            unlink(link.get());
            it = links.begin();
        }
        else
            ++it;
    }

    multi_action_off();
}

void WorkflowAction::unlink(data::Link* link)
{
    m_drawer->delete_link(link);
    link->ed().reset();
    g::pHistoryManager->push_link_action(link, mngr::HistoryActionType::LinkDel);
    m_wf->remove_link(link);
}

void WorkflowAction::set_node_color(data::NodeInstance* node, ImU32 color)
{
    ImU32 old_clr = 0, new_clr = 0;
    if (node->m_group)
    {
        old_clr = node->m_clr_group;
        node->set_group_color(ImGui::ColorConvertU32ToFloat4(color));
        new_clr = node->m_clr_group;
    }
    else
    {
        old_clr = ImGui::ColorConvertFloat4ToU32(node->m_clr_node);
        node->set_node_color(ImGui::ColorConvertU32ToFloat4(color));
        new_clr = ImGui::ColorConvertFloat4ToU32(node->m_clr_node);
    }

    if (old_clr == new_clr)
        return;

    g::pHistoryManager->push_node_action(node, mngr::HistoryActionType::NodeClr, mngr::HActionT(old_clr));
}

void WorkflowAction::reset_node_color(data::NodeInstance* node)
{
    ImU32 old_clr = 0, new_clr = 0;
    if (node->m_group)
    {
        old_clr = node->m_clr_group;
        node->reset_to_default_colors();
        new_clr = node->m_clr_group;
    }
    else
    {
        old_clr = ImGui::ColorConvertFloat4ToU32(node->m_clr_node);
        node->reset_to_default_colors();
        new_clr = ImGui::ColorConvertFloat4ToU32(node->m_clr_node);
    }

    if (old_clr == new_clr)
        return;

    g::pHistoryManager->push_node_action(node, mngr::HistoryActionType::NodeClr, old_clr);
}

void WorkflowAction::set_node_name(data::NodeInstance* node, const std::string& name)
{
    const auto old_name = node->get_name();
    const auto new_name = tools::str::trim_their(name);
    if (old_name == new_name)
        return;

    node->set_name(new_name);
    if (!node->m_group)
    {
        node->m_ignore_size_on_frame_end = true;
        node->reset_width();
    }

    g::pHistoryManager->push_node_action(node, mngr::HistoryActionType::NodeRen, mngr::HActionT(std::move(old_name)));
}

void WorkflowAction::reset_node_name(data::NodeInstance* node)
{
    set_node_name(node, node->get_name_original());
}

bool WorkflowAction::set_group_rect(data::NodeInstance* group, const ImRect& rect)
{
    const auto old_rc = group->m_rc;
    if (old_rc == rect)
        return false;

    group->m_rc = rect;
    group->m_pos_set = false;
    group->m_size_set = false;

    g::pHistoryManager->push_node_action(group, mngr::HistoryActionType::NodeRect, mngr::HActionT(old_rc));
    return true;
}

void WorkflowAction::reset_node_size(data::NodeInstance* node)
{
    const auto old_rc = node->m_rc;
    node->reset_width();

    if (old_rc == node->m_rc)
        return;

    g::pHistoryManager->push_node_action(node, mngr::HistoryActionType::NodeRect, mngr::HActionT(old_rc));
}

void WorkflowAction::set_node_expanded(data::NodeInstance* node, bool expanded)
{
    if (node->get_expanded() == expanded)
        return;
    if (m_drawer != nullptr && !expanded)
        m_drawer->handle_node_collapsing(node);
    node->expand(expanded);
    g::pHistoryManager->push_node_action(node, mngr::HistoryActionType::NodeExpand, !expanded);
}

void WorkflowAction::set_node_pinning(data::NodeInstance* node, bool pin)
{
    if (node->pinned() == pin)
        return;
    node->pin(pin);
    m_drawer->pin_node(node->ed(), node->pinned());
    g::pHistoryManager->push_node_action(node, mngr::HistoryActionType::NodePin, !pin);
}

void WorkflowAction::set_node_mode(data::NodeInstance* node, NodeMode mode)
{
    const auto old_mode = node->get_mode();
    if (old_mode == mode)
        return;
    node->set_mode(mode);
    g::pHistoryManager->push_node_action(node, mngr::HistoryActionType::NodeMode, mngr::HActionT(old_mode));
}

void WorkflowAction::begin_nodes_move(const std::set<data::NodeInstance*>& nodes)
{
    m_moving = true;
    m_moving_nodes.clear();
    for (auto node : nodes)
    {
        m_moving_nodes[node] = node->m_rc;
    }
}

void WorkflowAction::end_nodes_move(bool cancel)
{
    if (!m_moving)
    {
        LogWarn("Move operation wasn't start");
        return;
    }
    m_moving = false;
    if (cancel)
        return;

    multi_action_on("Move");
    for (auto& [node, rc] : m_moving_nodes)
    {
        if (node->m_rc != rc)
            g::pHistoryManager->push_node_action(node, mngr::HistoryActionType::NodeRect, mngr::HActionT(rc));
    }
    multi_action_off();
}

void WorkflowAction::set_pin_name(data::PinInstance* pin, const std::string& name)
{
    auto old_name = pin->state().name;
    pin->state().name = tools::str::trim_their(name);
    if (old_name == pin->state().name)
        return;
    pin->state().w_name = ImGui::CalcTextSize(pin->state().name.c_str()).x;
    auto node = m_wf->get_node_by_instance(pin->get_parent_node());
    node->reset_width();
    node->m_ignore_size_on_frame_end = true;

    g::pHistoryManager->push_pin_action(pin, mngr::HistoryActionType::PinRen, mngr::HActionT(std::move(old_name)));
}

void WorkflowAction::reset_pin_name(data::PinInstance* pin)
{
    return set_pin_name(pin, pin->get_object()->def_state.name);
}

void WorkflowAction::set_pin_val(data::PinInstance* pin, primitive&& old_val)
{
    if (old_val == pin->state().value)
        return;
    g::pHistoryManager->push_pin_action(pin, mngr::HistoryActionType::PinVal, mngr::HActionT(std::move(old_val)));
}

void WorkflowAction::set_pin_val(data::PinInstance* pin, int64_t old_idx)
{
    const primitive old_val = old_idx;
    if (old_val == pin->state().value)
        return;
    g::pHistoryManager->push_pin_action(pin, mngr::HistoryActionType::PinVal, mngr::HActionT(old_val));
}

void WorkflowAction::set_pin_val(data::PinInstance* pin, const float* new_colors)
{
    const auto old_colors = pin->state().value;
    const primitive new_colors_v = data::Pin::colors_to_int(ImVec4(new_colors));
    if (old_colors == new_colors_v)
        return;
    pin->state().value = new_colors_v;

    g::pHistoryManager->push_pin_action(pin, mngr::HistoryActionType::PinVal, mngr::HActionT(old_colors));
}

} // namespace data
