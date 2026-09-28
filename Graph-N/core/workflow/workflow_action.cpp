#include "core/workflow/workflow_action.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/history_manager.h"
#include "core/workflow/workflow.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/workflow/custom_nodes/cni_reroute.h"
#include "ui/custom_widgets/custom_widget.h"
#include "ui/node_drawer.h"
#include "ui/graph-n_flow.h"

namespace data
{

WorkflowAction::WorkflowAction(data::Workflow* wf) noexcept
    : m_wf(wf)
{}

WorkflowAction::~WorkflowAction() noexcept = default;

const blob::Blob* WorkflowAction::get_drawer_state() const noexcept
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
    ImRect visible_pos;
    if (m_drawer != nullptr)
        visible_pos = ImRect(m_drawer->get_visible_pos(ImVec2(100, 100)), ImVec2(-1, -1));

    const auto node_inst = m_wf->add_node(node, pos.value_or(visible_pos), select);
    node_inst->reset_width();
    g::pHistoryManager->push_node_action(node_inst, mngr::HistoryActionType::NodeAdd);
    return node_inst;
}

data::NodeInstance* WorkflowAction::add_group(std::optional<ImRect> size)
{
    const auto group_inst = m_wf->add_group(size.value_or(ImRect(100, 100, -1, -1)), "New group");
    g::pHistoryManager->push_node_action(group_inst, mngr::HistoryActionType::NodeAdd);
    return group_inst;
}

void WorkflowAction::split_reroute(data::NodeInstance* reroute_node)
{
    LogRAssert(reroute_node, "Zero ptr", );
    LogRAssert(reroute_node->flags().has(NodeInstFlags::Reroute), "Node is not reroute. w:" << reroute_node->get_workflow_id() << ", n:" << reroute_node->get_instance_id() << ", t:" << typeid(reroute_node).name(), );
    LogRAssert(reroute_node->flags().has_not(NodeInstFlags::RerouteSplitted), "Reroute already splitted. w:" << reroute_node->get_workflow_id() << ", n:" << reroute_node->get_instance_id(), );

    const mngr::HRerouteSplitted data = { cast::strict<data::CNI_Reroute>(reroute_node)->get_splitted_nodes_poses(), reroute_node->flags().has(NodeInstFlags::RerouteSplitted) };
    cast::strict<data::CNI_Reroute>(reroute_node)->split_to_parts();
    g::pHistoryManager->push_node_action(reroute_node, mngr::HistoryActionType::NodeRerouteSplit, mngr::HActionT(data));
}

void WorkflowAction::merge_reroute(data::NodeInstance* reroute_node, bool to_left)
{
    LogRAssert(reroute_node, "Zero ptr", );
    LogRAssert(reroute_node->flags().has_all(NodeInstFlags::Reroute, NodeInstFlags::RerouteSplitted), "Node is not splitted reroute. w:" << reroute_node->get_workflow_id() << ", n:" << reroute_node->get_instance_id() << ", t:" << typeid(reroute_node).name(), );

    const mngr::HRerouteSplitted data = { cast::strict<data::CNI_Reroute>(reroute_node)->get_splitted_nodes_poses(), reroute_node->flags().has(NodeInstFlags::RerouteSplitted) };
    cast::strict<data::CNI_Reroute>(reroute_node)->merge_parts(to_left);
    g::pHistoryManager->push_node_action(reroute_node, mngr::HistoryActionType::NodeRerouteSplit, mngr::HActionT(data));
}

void WorkflowAction::delete_node(data::NodeInstance* node)
{
    LogRAssert(node, "Zero ptr", );
    m_moving = false;
    m_moving_nodes.clear();

    g::pWfsManager->copy_del(node);
    g::pFlow->txt_panel_delete(m_wf->get_id(), node);
    g::pFlow->host_handle_node_removal(m_wf->get_id(), node->get_instance_id());
    g::pFlow->custom_widgets().cleanup_outputs(m_wf->get_id(), node->get_instance_id());

    if (m_drawer != nullptr)
        m_drawer->delete_node(node);
    node->ed().reset();

    if (node->flags().has(NodeInstFlags::RerouteSplitted_R))
    {
        if (node->flags().has(NodeInstFlags::RerouteSplitted))
            merge_reroute(node, true);
        return;
    }

    multi_action_on(node->flags().has(NodeInstFlags::Group) ? "Remove group" : "Remove node");

    if (node->flags().has(NodeInstFlags::RerouteSplitted))
        merge_reroute(node, true);

    m_wf->bypass_node(node);

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

void WorkflowAction::hard_reset() noexcept
{
    g::pFlow->txt_panel_clear(m_wf->get_id());
    if (m_drawer != nullptr)
        m_drawer->free();
}

void WorkflowAction::reinit(blob::Blob&& state)
{
    if (m_drawer != nullptr)
        m_drawer->init(std::move(state));
}

data::Link* WorkflowAction::link(data::PinInstance* p1, data::PinInstance* p2, iid instance_id /*= 0*/)
{
    LogRAssert(p1 && p2, "Zero ptr", nullptr);
    const auto link = data::Link::link(p1, p2, instance_id);
    multi_action_on("Link");
    g::pHistoryManager->push_link_action(link, mngr::HistoryActionType::LinkAdd);
    if (p1->state().type == p2->state().type)
    {
        if (p1->obj_flags().has(PinObjFlags::Input) && data::Pin::is_primitive_type(p1->state().type))
        {
            auto old = p1->state().value;
            p1->state().value = p2->state().value;
            set_pin_val(p1, std::move(old));
        }
        else if (p2->obj_flags().has(PinObjFlags::Input) && data::Pin::is_primitive_type(p2->state().type))
        {
            auto old = p2->state().value;
            p2->state().value = p1->state().value;
            set_pin_val(p2, std::move(old));
        }
    }
    multi_action_off();
    return link;
}

void WorkflowAction::unlink(const data::PinInstance* pin)
{
    LogRAssert(pin, "Zero ptr", );
    multi_action_on("Unlink");

    const auto links = m_wf->get_links_no_reroutes();
    for (auto it = links.begin(); it != links.end(); ++it)
    {
        auto& link = *it;
        if (link->is_linked_to(pin))
        {
            unlink(link);
        }
    }

    multi_action_off();
}

void WorkflowAction::unlink(const data::PinInstance* p1, const data::PinInstance* p2)
{
    LogRAssert(p1 && p2, "Zero ptr", );
    multi_action_on("Unlink");

    const auto links = m_wf->get_links_no_reroutes();
    for (auto link : links)
    {
        if (link->is_linked_to(p1) && link->is_linked_to(p2))
        {
            unlink(link);
        }
    }

    multi_action_off();
}

void WorkflowAction::unlink(data::Link* link)
{
    LogRAssert(link, "Zero ptr", );
    if (m_drawer != nullptr)
        m_drawer->delete_link(link);
    link->ed().reset();
    g::pHistoryManager->push_link_action(link, mngr::HistoryActionType::LinkDel);
    m_wf->remove_link(link);
}

void WorkflowAction::set_node_color(data::NodeInstance* node, ImU32 color)
{
    LogRAssert(node, "Zero ptr", );
    ImU32 old_clr = 0, new_clr = 0;
    if (node->flags().has(NodeInstFlags::Group))
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
    LogRAssert(node, "Zero ptr", );
    ImU32 old_clr = 0, new_clr = 0;
    if (node->flags().has(NodeInstFlags::Group))
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

    g::pHistoryManager->push_node_action(node, mngr::HistoryActionType::NodeClr, mngr::HActionT(old_clr));
}

void WorkflowAction::set_node_name(data::NodeInstance* node, const std::string& name)
{
    LogRAssert(node, "Zero ptr", );
    auto old_name = node->get_name();
    const auto new_name = tools::str::trim_their(name);
    if (old_name == new_name)
        return;

    node->set_name(new_name);
    if (node->flags().has_not(NodeInstFlags::Group))
    {
        g::pFlow->host_handle_renaming(node->get_workflow_id(), node->get_instance_id());
        node->flags().add(NodeInstFlags::IgnoreSizeOnFrameEnd);
        node->reset_width();
    }

    g::pHistoryManager->push_node_action(node, mngr::HistoryActionType::NodeRen, mngr::HActionT(std::move(old_name)));
}

void WorkflowAction::reset_node_name(data::NodeInstance* node)
{
    LogRAssert(node, "Zero ptr", );
    set_node_name(node, node->get_name_original());
}

bool WorkflowAction::set_node_rect(data::NodeInstance* node, const ImRect& rect)
{
    LogRAssert(node, "Zero ptr", false);
    const auto old_rc = node->m_rc;
    if (old_rc == rect)
        return false;

    node->m_rc = rect;
    node->flags().remove(NodeInstFlags::PosSet);
    if (node->flags().has(NodeInstFlags::Group))
        node->flags().remove(NodeInstFlags::SizeSet);

    g::pHistoryManager->push_node_action(node, mngr::HistoryActionType::NodeRect, mngr::HActionT(old_rc));
    return true;
}

void WorkflowAction::reset_node_size(data::NodeInstance* node)
{
    LogRAssert(node, "Zero ptr", );
    const auto old_rc = node->m_rc;
    node->reset_width();

    if (old_rc == node->m_rc)
        return;

    g::pHistoryManager->push_node_action(node, mngr::HistoryActionType::NodeRect, mngr::HActionT(old_rc));
}

void WorkflowAction::set_node_collapsed(data::NodeInstance* node, bool collapsed)
{
    LogRAssert(node, "Zero ptr", );
    if (node->flags().has(NodeInstFlags::Collapsed) == collapsed)
        return;
    if (m_drawer != nullptr && collapsed)
        m_drawer->handle_node_collapsing(node);
    const uint64_t old_mode = node->flags().get_by_mask(NodeInstFlags::Collapsed);
    node->collapse_node(collapsed);
    g::pHistoryManager->push_node_action(node, mngr::HistoryActionType::NodeCollapse, mngr::HActionT(old_mode));
}

void WorkflowAction::set_node_pinning(data::NodeInstance* node, bool pin)
{
    LogRAssert(node, "Zero ptr", );
    if (node->flags().has(NodeInstFlags::Pinned) == pin)
        return;
    const uint64_t old_mode = node->flags().get_by_mask(NodeInstFlags::Pinned);
    node->pin_node(pin);
    if (m_drawer != nullptr)
        m_drawer->pin_node(node);
    g::pHistoryManager->push_node_action(node, mngr::HistoryActionType::NodePin, mngr::HActionT(old_mode));
}

void WorkflowAction::set_node_mode(data::NodeInstance* node, NodeInstFlags mode)
{
    LogRAssert(node, "Zero ptr", );
    LogRAssert(NodeInstFlags::None == mode || g_node_inst_all_modes_mask.has(mode), "Invalid mode passed", );

    if (mode != NodeInstFlags::None && node->flags().has(mode))
        return;
    if (NodeInstFlags::ModeBypass == mode && node->obj_flags().has_not(NodeObjFlags::CanBypass))
        return;
    if (ObjectFlags<NodeInstFlags>::is_one_of(mode, NodeInstFlags::ModeMute, NodeInstFlags::ModeOnEvent, NodeInstFlags::ModeOnTrigger) && node->obj_flags().has_not(NodeObjFlags::CanMute))
        return;

    const uint64_t old_mode = node->flags().get_by_mask(g_node_inst_all_modes_mask);
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
    LogRAssert(pin, "Zero ptr", );
    auto old_name = pin->state().name;
    pin->set_name(tools::str::trim_their(name));
    if (old_name == pin->state().name)
        return;
    
    auto node = m_wf->get_node_by_instance(pin->get_parent_node());
    node->reset_width();
    node->flags().add(NodeInstFlags::IgnoreSizeOnFrameEnd);

    g::pHistoryManager->push_pin_action(pin, mngr::HistoryActionType::PinRen, mngr::HActionT(std::move(old_name)));
}

void WorkflowAction::reset_pin_name(data::PinInstance* pin)
{
    LogRAssert(pin, "Zero ptr", );
    return set_pin_name(pin, pin->get_object()->def_state.name);
}

void WorkflowAction::set_pin_val(data::PinInstance* pin, Primitive&& old_val)
{
    LogRAssert(pin, "Zero ptr", );
    if (old_val == pin->state().value) [[unlikely]]
        return;

    multi_action_on("Set value");
    g::pHistoryManager->push_pin_action(pin, mngr::HistoryActionType::PinVal, mngr::HActionT(std::move(old_val)));
    mirror_pin_value(pin);
    multi_action_off();
}

void WorkflowAction::set_pin_val(data::PinInstance* pin, int64_t old_idx)
{
    LogRAssert(pin, "Zero ptr", );
    const Primitive old_val = old_idx;
    if (old_val == pin->state().value) [[unlikely]]
        return;

    multi_action_on("Set value");
    g::pHistoryManager->push_pin_action(pin, mngr::HistoryActionType::PinVal, mngr::HActionT(old_val));
    mirror_pin_value(pin);
    multi_action_off();
}

void WorkflowAction::set_pin_val(data::PinInstance* pin, const float* new_colors)
{
    LogRAssert(pin, "Zero ptr", );
    const auto old_colors = pin->state().value;
    const Primitive new_colors_v = data::Pin::colors_to_int(ImVec4(new_colors));
    if (old_colors == new_colors_v) [[unlikely]]
        return;
    pin->state().value = new_colors_v;

    multi_action_on("Set value");
    g::pHistoryManager->push_pin_action(pin, mngr::HistoryActionType::PinVal, mngr::HActionT(old_colors));
    mirror_pin_value(pin);
    multi_action_off();
}

void WorkflowAction::set_pin_cag(data::PinInstance* pin, ControlAfterGenerate old_cag, const Primitive& old_min, const Primitive& old_max, const Primitive& old_step)
{
    LogRAssert(pin, "Zero ptr", );
    if (old_cag == pin->state().cag_val && old_min == pin->state().min && old_max == pin->state().max && old_step == pin->state().step) [[unlikely]]
        return;

    mngr::HPinCag cag;
    cag.cag = old_cag;
    cag.max = old_max;
    cag.min = old_min;
    cag.step = old_step;

    multi_action_on("CAG value");
    g::pHistoryManager->push_pin_action(pin, mngr::HistoryActionType::PinCag, mngr::HActionT(std::move(cag)));
    mirror_pin_value(pin);
    multi_action_off();
}

void WorkflowAction::set_pin_multiline(data::PinInstance* pin, bool multiline)
{
    LogRAssert(pin, "Zero ptr", );
    if (pin->flags().has(PinInstFlags::Multiline) == multiline)
        return;
    const uint64_t old_mode = pin->flags().get_by_mask(PinInstFlags::Multiline);
    pin->flags().add(PinInstFlags::MultilineSet);
    pin->flags().add_remove_if(PinInstFlags::Multiline, multiline);

    if (!multiline)
        g::pFlow->txt_panel_delete(pin->get_workflow_id(), nullptr, pin);

    multi_action_on("Multiline");
    g::pHistoryManager->push_pin_action(pin, mngr::HistoryActionType::PinMultiline, mngr::HActionT(old_mode));
    mirror_pin_value(pin);
    multi_action_off();
}

void WorkflowAction::set_pin_color_config(data::PinInstance* pin, ObjectFlags<PinInstFlags> old_flags)
{
    LogRAssert(pin, "Zero ptr", );
    auto new_flags_raw = pin->flags().get_by_mask(PinInstFlags::ColorHSV, PinInstFlags::ColorHex, PinInstFlags::ColorRaw, PinInstFlags::ColorFloat);
    auto old_flags_raw = old_flags.get_by_mask(PinInstFlags::ColorHSV, PinInstFlags::ColorHex, PinInstFlags::ColorRaw, PinInstFlags::ColorFloat);
    if (new_flags_raw == old_flags_raw)
        return;

    g::pHistoryManager->push_pin_action(pin, mngr::HistoryActionType::PinColorConfig, mngr::HActionT(old_flags_raw));
}

void WorkflowAction::mirror_pin_value(data::PinInstance* pin)
{
    LogRAssert(pin, "Zero ptr", );
    const auto* wf = g::pWfsManager->access(pin->get_workflow_id());
    const auto* node = wf->get_node_by_instance(pin->get_parent_node());
    if (node->flags().has_not(NodeInstFlags::CNI)) [[likely]]
        return;

    std::vector<PinInstance*> mirrored_pins;
    node->cni_mirror_value_to(pin, mirrored_pins);
    for (size_t i = 0; i < mirrored_pins.size(); i++)
    {
        auto dest_pin = mirrored_pins[i];

        auto old = dest_pin->state().value;
        dest_pin->state().value = pin->state().value;
        set_pin_val(dest_pin, std::move(old));

        node = wf->get_node_by_instance(dest_pin->get_parent_node()); 
        if (node->flags().has(NodeInstFlags::CNI))
            node->cni_mirror_value_to(dest_pin, mirrored_pins);
    }
}

} // namespace data
