/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "core/workflow/custom_nodes/cni_fast_action_rgthree.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/workflow.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/hotkeys_manager.h"
#include "ui/graph-n_flow.h"

namespace data
{

///////////////////////////////////////////////////////////////////////////////////////////////
/////////        CNI_FastModeSwitcherRgthree                            ///////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

CNI_FastModeSwitcherRgthree::CNI_FastModeSwitcherRgthree(Node* obj, wid workflow_id, iid instance_id, NodeInstFlags action_mode) noexcept
    : CNI_FastSwitcherBaseRgthree(obj, workflow_id, instance_id)
    , m_action_mode(action_mode)
{
    flags().add(NodeInstFlags::CNI, NodeInstFlags::JsonApiDontExport, NodeInstFlags::JsonCustomSerialization, NodeInstFlags::BlobCustomSerialization);
}

void CNI_FastModeSwitcherRgthree::cni_init()
{
    auto wf = g::pWfsManager->access(get_workflow_id());

    auto& inputs = get_inputs();
    for (auto input : inputs)
    {
        cni_remove_pin(input->get_object());
        remove_pin(input);
        wf->remove_pin(input);
    }

    auto& outputs = get_outputs();
    for (auto output : outputs)
    {
        cni_remove_pin(output->get_object());
        remove_pin(output);
        wf->remove_pin(output);
    }
    reset_pins();

    iid out_id = 0;
    CNI_DeserializedFastActionRgthree* deserialized_values = nullptr;
    if (m_deserialized_values)
    {
        deserialized_values = cast::dynamic<CNI_DeserializedFastActionRgthree>(m_deserialized_values.get());
        LogAssert(deserialized_values, "Zero deserialized values");

        if (deserialized_values != nullptr)
        {
            out_id = deserialized_values->out_id;
            for (auto [pin_id, hash, mode] : deserialized_values->in_ids)
            {
                (void)mode;

                add_pin();
                if (pin_id != 0)
                    m_switches.back().pin->set_instance_id(pin_id);
                if (hash != 0)
                    m_switches.back().pin->get_object()->force_replace_hash(hash);
            }
        }
    }
    add_pin();

    auto obj = g::pNodesManager->make_pin_ptr(get_object(), "*", "OPT", 0);
    auto instance = wf->add_pin(this, obj.get(), out_id);
    obj->flags.remove(PinObjFlags::CanRename);
    add_output(instance);
    cni_add_pin(std::move(obj));    
}

void CNI_FastModeSwitcherRgthree::on_deserialization_ends()
{
    refresh_switchers();
    m_deserialized_values.reset();
}

bool CNI_FastModeSwitcherRgthree::cni_copy_from(const NodeInstance* copy_from_node, bool copy_pin_ids)
{
    auto& src_outs = copy_from_node->get_outputs();
    LogRAssert(!src_outs.empty(), "Source node pins empty out", true);
    LogRAssert(copy_from_node->obj_flags().has_any(NodeObjFlags::SpecFastBypasserRgthree, NodeObjFlags::SpecFastMuterRgthree), "Not a Switcher", true);

    auto that_node = cast::strict<CNI_FastModeSwitcherRgthree>(copy_from_node);
    LogRAssert(that_node, "Zero after conversion", true);

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedFastActionRgthree>();
    auto switcher_data = cast::strict<CNI_DeserializedFastActionRgthree>(data.get());
    if (copy_pin_ids)
        switcher_data->out_id = src_outs[0]->get_instance_id();

    for (const auto switcher : that_node->m_switches)
    {
        switcher_data->in_ids.push_back(std::make_tuple(copy_pin_ids ? switcher.pin->get_instance_id() : 0, switcher.pin->get_object()->get_hash(), 0));
    }

    cni_save_deserialized_info(std::move(data));

    return true;
}

void CNI_FastModeSwitcherRgthree::cni_cleanup_base_object()
{
    auto& ins = get_inputs();
    for (auto in : ins)
    {
        cni_remove_pin(in->get_object());
    }
    auto& outs = get_outputs();
    for (auto out : outs)
    {
        cni_remove_pin(out->get_object());
    }
    m_switches.clear();
}

void CNI_FastModeSwitcherRgthree::add_pin()
{
    // widget will be created on demand, when pin is connected

    auto wf = g::pWfsManager->access(get_workflow_id());
    auto obj = g::pNodesManager->make_pin_ptr(get_object(), "*", "", -1);
    obj->force_replace_hash(base::GlobalID::random());
    auto instance = wf->add_pin(this, obj.get());
    obj->flags.remove(PinObjFlags::CanRename);
    add_input(instance);
    cni_add_pin(std::move(obj));

    Switch sw;
    sw.pin = instance;
    sw.state = NodeInstFlags::None;
    m_switches.push_back(sw);
}

void CNI_FastModeSwitcherRgthree::refresh_switchers()
{
    auto wf = g::pWfsManager->access(get_workflow_id());
    for (auto& sw : m_switches)
    {
        if (sw.linked_node != 0)
        {
            auto node = wf->get_node_by_instance(sw.linked_node);
            sw.widget->set_name("Enable " + node->get_name());
            sw.pin->set_name(node->get_name());
            const bool enabled = node->flags().has_not(m_action_mode);
            *sw.widget->state().value.get_bool_ref() = enabled;
        }
    }
    reset_width();
}

std::list<std::pair<iid, NodeInstFlags>> CNI_FastModeSwitcherRgthree::switch_state(const PinInstance* widget)
{
    Switch* sw = nullptr;
    for (auto& s : m_switches)
    {
        if (s.widget == widget)
        {
            sw = &s;
            break;
        }
    }
    LogRAssert(sw, "Switch not found", {});
    if (0 == sw->linked_node)
        return {};

    const bool switched = !sw->widget->state().value.get_bool();
    *sw->widget->state().value.get_bool_ref() = switched;
    return { std::make_pair(sw->linked_node, switched ? m_action_mode : NodeInstFlags::None) };
}

void CNI_FastModeSwitcherRgthree::cni_pin_linked(PinInstance* p1, PinInstance* p2)
{
    LogRAssert(p1 && p2, "Zero ptr", );

    auto& out = get_outputs();
    if (!out.empty() && (out[0] == p1 || out[0] == p2))
        return;

    Switch* sw = nullptr;
    PinInstance* external = nullptr;

    size_t index = 0;
    for ( ; index < m_switches.size(); index++)
    {
        auto& s = m_switches[index];
        if (s.pin == p1 || s.pin == p2)
        {
            sw = &s;
            if (s.pin == p1) external = p2;
            if (s.pin == p2) external = p1;
            break;
        }
    }
    LogRAssert(sw, "Switch not found", );

    const bool last_switch = (index == m_switches.size() - 1);

    auto wf = g::pWfsManager->access(get_workflow_id());
    auto node = wf->get_node_by_instance(external->get_parent_node());
    LogRAssert(node, "Node not found", );

    sw->linked_node = external->get_parent_node();
    sw->pin->set_name(node->get_name());
    if (nullptr == sw->widget)
    {
        auto obj = g::pNodesManager->make_pin_ptr(get_object(), "BOOLEAN", "Enable " + node->get_name(), -1);
        auto instance = wf->add_pin(this, obj.get());
        obj->flags.remove(PinObjFlags::CanRename, PinObjFlags::CanLink, PinObjFlags::CanUnlink);
        add_input(instance);
        cni_add_pin(std::move(obj));
        sw->widget = instance;
    }
    else
    {
        const bool enabled = node->flags().has_not(m_action_mode);
        sw->state = (enabled ? NodeInstFlags::None : m_action_mode);
        sw->widget->set_name("Enable " + node->get_name());
        *sw->widget->state().value.get_bool_ref() = enabled;
    }

    if (last_switch)
        add_pin();
    refresh_switchers();
}

void CNI_FastModeSwitcherRgthree::cni_pin_unlinked(const PinInstance* p1, const PinInstance* p2)
{
    LogRAssert(p1 && p2, "Zero ptr", );

    auto& out = get_outputs();
    if (!out.empty() && (out[0] == p1 || out[0] == p2))
        return;

    Switch* sw = nullptr;

    for (auto& s : m_switches)
    {
        if (s.pin == p1 || s.pin == p2)
        {
            sw = &s;
            break;
        }
    }
    LogRAssert(sw, "Switch not found", );

    sw->linked_node = 0;
    sw->state = NodeInstFlags::None;
    sw->widget->set_name("Enable");
}

CNI_DeserializedFastActionRgthree CNI_FastModeSwitcherRgthree::dump() const noexcept
{
    CNI_DeserializedFastActionRgthree dump;
    if (!get_outputs().empty())
        dump.out_id = get_outputs()[0]->get_instance_id();
    else
        LogError("Output pin wasn't initialized");

    for (const auto sw : m_switches)
    {
        if (sw.linked_node != 0)
        {
            dump.in_ids.push_back(std::make_tuple(sw.pin->get_instance_id(), 0, 0));
        }
    }
    return dump;
}

iid CNI_FastModeSwitcherRgthree::node_from_widget(const PinInstance* widget) const noexcept
{
    for (auto sw : m_switches)
    {
        if (sw.widget == widget)
            return sw.linked_node;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////
/////////        CNI_FastGroupModeSwitcherRgthree                       ///////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

CNI_FastGroupModeSwitcherRgthree::CNI_FastGroupModeSwitcherRgthree(Node* obj, wid workflow_id, iid instance_id, NodeInstFlags action_mode) noexcept
    : CNI_FastSwitcherBaseRgthree(obj, workflow_id, instance_id)
    , m_action_mode(action_mode)
{
    flags().add(NodeInstFlags::CNI, NodeInstFlags::JsonApiDontExport, NodeInstFlags::JsonCustomSerialization, NodeInstFlags::BlobCustomSerialization);
}

void CNI_FastGroupModeSwitcherRgthree::cni_init()
{
    auto wf = g::pWfsManager->access(get_workflow_id());

    auto& inputs = get_inputs();
    for (auto input : inputs)
    {
        cni_remove_pin(input->get_object());
        remove_pin(input);
        wf->remove_pin(input);
    }

    auto& outputs = get_outputs();
    for (auto output : outputs)
    {
        cni_remove_pin(output->get_object());
        remove_pin(output);
        wf->remove_pin(output);
    }
    reset_pins();

    iid out_id = 0;
    CNI_DeserializedFastActionRgthree* deserialized_values = nullptr;
    if (m_deserialized_values)
    {
        deserialized_values = cast::dynamic<CNI_DeserializedFastActionRgthree>(m_deserialized_values.get());
        LogAssert(deserialized_values, "Zero deserialized values");

        if (deserialized_values != nullptr)
            out_id = deserialized_values->out_id;
    }
    auto obj = g::pNodesManager->make_pin_ptr(get_object(), "*", "OPT", 0);
    auto instance = wf->add_pin(this, obj.get(), out_id);
    obj->flags.remove(PinObjFlags::CanRename);
    add_output(instance);
    cni_add_pin(std::move(obj));    
}

void CNI_FastGroupModeSwitcherRgthree::on_deserialization_ends()
{
    refresh_switchers();
    m_deserialized_values.reset();
}

void CNI_FastGroupModeSwitcherRgthree::cni_cleanup_base_object()
{
    auto& ins = get_inputs();
    for (auto in : ins)
    {
        cni_remove_pin(in->get_object());
    }
    auto& outs = get_outputs();
    for (auto out : outs)
    {
        cni_remove_pin(out->get_object());
    }
    m_switches.clear();
}

void CNI_FastGroupModeSwitcherRgthree::refresh_switchers()
{
    auto wf = g::pWfsManager->access(get_workflow_id());
    std::set<iid> added_groups;
    
    for (auto it = m_switches.begin(); it != m_switches.end(); )
    {
        const auto sw = *it;
        if (0 == sw.linked_group)
        {
            delete_widget(sw);
            it = m_switches.erase(it);
            continue;
        }

        auto group = wf->get_node_by_instance(sw.linked_group);
        if (nullptr == group)
        {
            delete_widget(sw);
            it = m_switches.erase(it);
            continue;
        }

        ++it;
        added_groups.insert(group->get_instance_id());

        const bool enabled = get_group_state(group);
        sw.widget->set_name("Enable " + group->get_name());
        *sw.widget->state().value.get_bool_ref() = enabled;
    }

    auto& all_nodes = wf->get_nodes_all();
    for (auto& node : all_nodes)
    {
        if (node->flags().has_not(NodeInstFlags::Group))
            continue;
        if (added_groups.contains(node->get_instance_id()))
            continue;
        add_widget(node.get());
    }

    reset_width();
}

bool CNI_FastGroupModeSwitcherRgthree::get_group_state(NodeInstance* group) const
{
    bool enabled = true;
    auto nodes = g::pFlow->wf_get_nodes_in_group(get_workflow_id(), group);
    for (auto node : nodes)
    {
        if (node->flags().has(m_action_mode))
        {
            enabled = false;
            break;
        }
    }

    return enabled;
}

void CNI_FastGroupModeSwitcherRgthree::add_widget(NodeInstance* group)
{
    auto wf = g::pWfsManager->access(get_workflow_id());
    auto obj = g::pNodesManager->make_pin_ptr(get_object(), "BOOLEAN", "Enable " + group->get_name(), -1);
    auto instance = wf->add_pin(this, obj.get());
    obj->flags.remove(PinObjFlags::CanRename, PinObjFlags::CanLink, PinObjFlags::CanUnlink);
    add_input(instance);
    cni_add_pin(std::move(obj));

    Switch sw;
    sw.linked_group = group->get_instance_id();
    sw.widget = instance;
    const bool enabled = get_group_state(group);
    sw.state = (enabled ? NodeInstFlags::None : m_action_mode);
    sw.widget->set_name("Enable " + group->get_name());
    *sw.widget->state().value.get_bool_ref() = enabled;
    m_switches.push_back(sw);
}

void CNI_FastGroupModeSwitcherRgthree::delete_widget(Switch sw)
{
    cni_remove_pin(sw.widget->get_object());
    auto wf = g::pWfsManager->access(get_workflow_id());
    remove_pin(sw.widget);
    wf->remove_pin(sw.widget);
}

std::list<std::pair<iid, NodeInstFlags>> CNI_FastGroupModeSwitcherRgthree::switch_state(const PinInstance* widget)
{
    Switch* sw = nullptr;
    for (auto& s : m_switches)
    {
        if (s.widget == widget)
        {
            sw = &s;
            break;
        }
    }
    LogRAssert(sw, "Switch not found", {});
    if (0 == sw->linked_group)
        return {};

    auto wf = g::pWfsManager->access(get_workflow_id());
    auto group = wf->get_node_by_instance(sw->linked_group);
    if (nullptr == group)
        return {};

    const bool switched = !sw->widget->state().value.get_bool();
    *sw->widget->state().value.get_bool_ref() = switched;
    
    std::list<std::pair<iid, NodeInstFlags>> ret;
    auto nodes = g::pFlow->wf_get_nodes_in_group(get_workflow_id(), group);
    for (auto node : nodes)
    {
        ret.push_back(std::make_pair(node->get_instance_id(), switched ? m_action_mode : NodeInstFlags::None));
    }

    return ret;
}

CNI_DeserializedFastActionRgthree CNI_FastGroupModeSwitcherRgthree::dump() const noexcept
{
    CNI_DeserializedFastActionRgthree dump;
    if (!get_outputs().empty())
        dump.out_id = get_outputs()[0]->get_instance_id();
    else
        LogError("Output pin wasn't initialized");
    return dump;
}

iid CNI_FastGroupModeSwitcherRgthree::node_from_widget(const PinInstance* widget) const noexcept
{
    for (auto sw : m_switches)
    {
        if (sw.widget == widget)
            return sw.linked_group;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////
/////////        CNI_FastActionRgthree                                  ///////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

namespace
{
enum ActionType
{
    NoAction,
    DoMute,
    DoBypass,
    DoAlways
};
const char* action_name(ActionType act)
{
    switch (act)
    {
    case DoMute:
        return "Mute";
    case DoBypass:
        return "Bypass";
    case DoAlways:
        return "Enable";
    }
    return "None";
}
bool action(ActionType act, NodeInstFlags& mode)
{
    switch (act)
    {
    case NoAction:
        return false;
    case DoMute:
        mode = NodeInstFlags::ModeMute;
        break;
    case DoBypass:
        mode = NodeInstFlags::ModeBypass;
        break;
    case DoAlways:
        mode = NodeInstFlags::None;
        break;
    default:
        LogRErrorS("Unknown action", false);
    }
    return true;
}
} // namespace

uint8_t CNI_FastActionRgthree::mode_from_string(const std::string& string_mode) noexcept
{
    if (string_mode == "Enable")
        return DoAlways;
    if (string_mode == "Mute")
        return DoMute;
    if (string_mode == "Bypass")
        return DoBypass;
    return NoAction;
}

std::string CNI_FastActionRgthree::string_from_mode(const Primitive& value) noexcept
{
    LogRAssertS(value.is_number(), "Not a combo value", "");
    return action_name(cast::to<ActionType>(value.get_int()));
}

CNI_FastActionRgthree::CNI_FastActionRgthree(Node* obj, wid workflow_id, iid instance_id) noexcept
    : CNI_FastSwitcherBaseRgthree(obj, workflow_id, instance_id)
{
    flags().add(NodeInstFlags::CNI, NodeInstFlags::JsonApiDontExport, NodeInstFlags::JsonCustomSerialization, NodeInstFlags::BlobCustomSerialization);
}

void CNI_FastActionRgthree::cni_init()
{
    auto wf = g::pWfsManager->access(get_workflow_id());

    auto& inputs = get_inputs();
    for (auto input : inputs)
    {
        cni_remove_pin(input->get_object());
        remove_pin(input);
        wf->remove_pin(input);
    }

    auto& outputs = get_outputs();
    for (auto output : outputs)
    {
        cni_remove_pin(output->get_object());
        remove_pin(output);
        wf->remove_pin(output);
    }
    reset_pins();

    iid out_id = 0;
    CNI_DeserializedFastActionRgthree* deserialized_values = nullptr;
    if (m_deserialized_values)
    {
        deserialized_values = cast::dynamic<CNI_DeserializedFastActionRgthree>(m_deserialized_values.get());
        LogAssert(deserialized_values, "Zero deserialized values");

        if (deserialized_values != nullptr)
        {
            out_id = deserialized_values->out_id;
            for (auto [pin_id, hash, mode] : deserialized_values->in_ids)
            {
                (void)mode;

                add_pin();
                if (pin_id != 0)
                    m_switches.back().pin->set_instance_id(pin_id);
                if (hash != 0)
                    m_switches.back().pin->get_object()->force_replace_hash(hash);
            }
        }
    }
    add_pin();
}

void CNI_FastActionRgthree::on_deserialization_ends()
{
    refresh_switchers();
    m_deserialized_values.reset();
}

bool CNI_FastActionRgthree::cni_copy_from(const NodeInstance* copy_from_node, bool copy_pin_ids)
{
    LogRAssert(copy_from_node->obj_flags().has_any(NodeObjFlags::SpecFastActRgthree, NodeObjFlags::SpecFastMuterRgthree), "Not a Switcher", true);

    auto that_node = cast::strict<CNI_FastActionRgthree>(copy_from_node);
    LogRAssert(that_node, "Zero after conversion", true);

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedFastActionRgthree>();
    auto switcher_data = cast::strict<CNI_DeserializedFastActionRgthree>(data.get());
    for (const auto switcher : that_node->m_switches)
    {
        ActionType mode = NoAction;
        if (switcher.widget != nullptr)
            mode = cast::to<ActionType>(switcher.widget->state().value.get_int());
        switcher_data->in_ids.push_back(std::make_tuple(copy_pin_ids ? switcher.pin->get_instance_id() : 0, switcher.pin->get_object()->get_hash(), mode));
    }

    cni_save_deserialized_info(std::move(data));

    return true;
}

void CNI_FastActionRgthree::cni_cleanup_base_object()
{
    auto& ins = get_inputs();
    for (auto in : ins)
    {
        cni_remove_pin(in->get_object());
    }
    auto& outs = get_outputs();
    for (auto out : outs)
    {
        cni_remove_pin(out->get_object());
    }
    m_switches.clear();
}

void CNI_FastActionRgthree::add_pin()
{
    // widget will be created on demand, when pin is connected

    auto wf = g::pWfsManager->access(get_workflow_id());
    auto obj = g::pNodesManager->make_pin_ptr(get_object(), "*", "", -1);
    obj->force_replace_hash(base::GlobalID::random());
    auto instance = wf->add_pin(this, obj.get());
    obj->flags.remove(PinObjFlags::CanRename);
    add_input(instance);
    cni_add_pin(std::move(obj));

    Switch sw;
    sw.pin = instance;
    m_switches.push_back(sw);
}

void CNI_FastActionRgthree::refresh_switchers()
{
    auto wf = g::pWfsManager->access(get_workflow_id());
    for (auto& sw : m_switches)
    {
        if (sw.linked_node != 0)
        {
            auto node = wf->get_node_by_instance(sw.linked_node);
            sw.widget->set_name("Enable " + node->get_name());
            sw.pin->set_name(node->get_name());
        }
    }
    reset_width();
}

std::list<std::pair<iid, NodeInstFlags>> CNI_FastActionRgthree::switch_state(const PinInstance* /*widget*/)
{
    std::list<std::pair<iid, NodeInstFlags>> ret;

    for (auto& s : m_switches)
    {
        if (s.linked_node != 0 && s.widget != nullptr)
        {
            std::pair<iid, NodeInstFlags> node;
            node.first = s.linked_node;
            if (!action(cast::to<ActionType>(s.widget->state().value.get_int()), node.second))
                continue;            
            ret.push_back(node);
        }
    }

    return ret;
}

void CNI_FastActionRgthree::cni_pin_linked(PinInstance* p1, PinInstance* p2)
{
    LogRAssert(p1 && p2, "Zero ptr", );

    Switch* sw = nullptr;
    PinInstance* external = nullptr;

    size_t index = 0;
    for ( ; index < m_switches.size(); index++)
    {
        auto& s = m_switches[index];
        if (s.pin == p1 || s.pin == p2)
        {
            sw = &s;
            if (s.pin == p1) external = p2;
            if (s.pin == p2) external = p1;
            break;
        }
    }
    LogRAssert(sw, "Switch not found", );

    const bool last_switch = (index == m_switches.size() - 1);

    auto wf = g::pWfsManager->access(get_workflow_id());
    auto node = wf->get_node_by_instance(external->get_parent_node());
    LogRAssert(node, "Node not found", );

    sw->linked_node = external->get_parent_node();
    sw->pin->set_name(node->get_name());
    if (nullptr == sw->widget)
    {
        auto obj = g::pNodesManager->make_pin_ptr(get_object(), "COMBO", "" + node->get_name(), -1);
        obj->combo_items.push_back(action_name(NoAction));
        obj->combo_items.push_back(action_name(DoMute));
        obj->combo_items.push_back(action_name(DoBypass));
        obj->combo_items.push_back(action_name(DoAlways));
        obj->finalize_combo();

        auto instance = wf->add_pin(this, obj.get());
        instance->set_name("Switch " + node->get_name());
        obj->flags.remove(PinObjFlags::CanRename, PinObjFlags::CanLink, PinObjFlags::CanUnlink);
        add_input(instance);
        cni_add_pin(std::move(obj));
        sw->widget = instance;

        if (flags().has(NodeInstFlags::Deserialization) && m_deserialized_values)
        {
            CNI_DeserializedFastActionRgthree* deserialized_values = cast::dynamic<CNI_DeserializedFastActionRgthree>(m_deserialized_values.get());
            if (deserialized_values != nullptr)
            {
                for (auto [pin_id, hash, mode] : deserialized_values->in_ids)
                {
                    (void)hash;
                    if (pin_id == sw->pin->get_instance_id())
                    {
                        *sw->widget->state().value.get_int_ref() = mode;
                        break;
                    }
                }
            }
        }

    }
    else
    {
        sw->widget->set_name("Switch " + node->get_name());
        *sw->widget->state().value.get_int_ref() = 0;
    }

    if (last_switch)
        add_pin();
    refresh_switchers();
}

void CNI_FastActionRgthree::cni_pin_unlinked(const PinInstance* p1, const PinInstance* p2)
{
    LogRAssert(p1 && p2, "Zero ptr", );

    Switch* sw = nullptr;
    for (auto& s : m_switches)
    {
        if (s.pin == p1 || s.pin == p2)
        {
            sw = &s;
            break;
        }
    }
    LogRAssert(sw, "Switch not found", );

    sw->linked_node = 0;
}

CNI_DeserializedFastActionRgthree CNI_FastActionRgthree::dump() const noexcept
{
    CNI_DeserializedFastActionRgthree dump;

    for (const auto sw : m_switches)
    {
        if (sw.linked_node != 0)
        {
            ActionType mode = NoAction;
            if (sw.widget != nullptr)
                mode = cast::to<ActionType>(sw.widget->state().value.get_int());
            dump.in_ids.push_back(std::make_tuple(sw.pin->get_instance_id(), 0, mode));
        }
    }
    return dump;
}

iid CNI_FastActionRgthree::node_from_widget(const PinInstance* widget) const noexcept
{
    for (auto sw : m_switches)
    {
        if (sw.widget == widget)
            return sw.linked_node;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////
/////////        CNI_BookmarkRgthree                                    ///////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

void CNI_BookmarkRgthree::cni_init()
{
    LogRError("Not implemented", );
#if 0
    auto wf = g::pWfsManager->access(get_workflow_id());

    auto& inputs = get_inputs();
    for (auto input : inputs)
    {
        cni_remove_pin(input->get_object());
        remove_pin(input);
        wf->remove_pin(input);
    }

    auto& outputs = get_outputs();
    for (auto output : outputs)
    {
        cni_remove_pin(output->get_object());
        remove_pin(output);
        wf->remove_pin(output);
    }
    reset_pins();

    CNI_DeserializedBookmarkRgthree* deserialized_values = nullptr;
    if (m_deserialized_values)
    {
        deserialized_values = cast::dynamic<CNI_DeserializedBookmarkRgthree>(m_deserialized_values.get());
        LogAssert(deserialized_values, "Zero deserialized values");

        if (deserialized_values != nullptr)
        {
            m_hotkey = deserialized_values->hotkey;
            m_zoom = deserialized_values->zoom;
        }
    }
    m_deserialized_values.reset();

    auto obj = g::pNodesManager->make_pin_ptr(get_object(), "COMBO", "Shortcut", -1);

    const auto& key_names = g::pHotkeysManager->get_key_names_for_bookmark();
    obj->combo_items.reserve(key_names.size());
    for (auto hk : key_names)
    {
        obj->combo_items.push_back(hk.second);
    }
    obj->finalize_combo();

    auto instance = wf->add_pin(this, obj.get());
    obj->flags.remove(PinObjFlags::CanRename, PinObjFlags::CanLink, PinObjFlags::CanUnlink);
    add_input(instance);
    cni_add_pin(std::move(obj));

    int64_t bookmarks_on_canvas = 0;
    auto& all_nodes = wf->get_nodes_all();
    for (auto& node : all_nodes)
    {
        if (node->has_object() && node->obj_flags().has(NodeObjFlags::SpecBookmarkRgthree))
            bookmarks_on_canvas++;
    }
    if (size_t(bookmarks_on_canvas + 1) >= key_names.size())
        bookmarks_on_canvas = 0;
    else
    {
        *instance->state().value.get_int_ref() = bookmarks_on_canvas + 1;
        g::pHotkeysManager->register_bookmark(key_names[bookmarks_on_canvas + 1].first, this);
    }

    obj = g::pNodesManager->make_pin_ptr(get_object(), "FLOAT", "Zoom", -1);
    instance = wf->add_pin(this, obj.get());
    obj->flags.remove(PinObjFlags::CanRename, PinObjFlags::CanLink, PinObjFlags::CanUnlink);
    add_input(instance);
    cni_add_pin(std::move(obj));
#endif // #if 0
}

void CNI_BookmarkRgthree::cni_cleanup_base_object()
{
    auto& ins = get_inputs();
    for (auto in : ins)
    {
        cni_remove_pin(in->get_object());
    }
    auto& outs = get_outputs();
    for (auto out : outs)
    {
        cni_remove_pin(out->get_object());
    }
}

bool CNI_BookmarkRgthree::cni_copy_from(const NodeInstance* copy_from_node, bool /*copy_pin_ids*/)
{
    LogRAssert(copy_from_node->obj_flags().has(NodeObjFlags::SpecBookmarkRgthree), "Not a Bookmark", true);

    auto that_node = cast::strict<CNI_BookmarkRgthree>(copy_from_node);
    LogRAssert(that_node, "Zero after conversion", true);

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedBookmarkRgthree>();
    auto bm_data = cast::strict<CNI_DeserializedBookmarkRgthree>(data.get());
    bm_data->hotkey = m_hotkey;
    bm_data->zoom = m_zoom;
    cni_save_deserialized_info(std::move(data));

    return true;
}

CNI_DeserializedBookmarkRgthree CNI_BookmarkRgthree::dump() const noexcept
{
    CNI_DeserializedBookmarkRgthree dump;
    dump.hotkey = m_hotkey;
    dump.zoom = m_zoom;
    return dump;
}


} // namespace data


