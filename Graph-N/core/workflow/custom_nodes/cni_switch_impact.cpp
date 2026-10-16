#include "core/workflow/custom_nodes/cni_switch_impact.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/nodes_manager.h"
#include "core/workflow/pin_instance.h"

namespace data
{

///////////////////////////////////////////////////////////////////////////////////////////////
/////////        CNI_SwitchImpact                                       ///////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

CNI_SwitchImpact::CNI_SwitchImpact(Node* obj, wid workflow_id, iid instance_id) noexcept
    : CNI_BaseObj(obj, workflow_id, instance_id)
{
    flags().add(NodeInstFlags::CNI, NodeInstFlags::JsonCustomSerialization, NodeInstFlags::BlobCustomSerialization);
}

void CNI_SwitchImpact::cni_init()
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

    CNI_DeserializedSwitchImpact* deserialized_values = nullptr;
    if (m_deserialized_values)
    {
        deserialized_values = cast::dynamic<CNI_DeserializedSwitchImpact>(m_deserialized_values.get());
        LogAssert(deserialized_values, "Zero deserialized values");
    }

    const auto assign_deserialized_value = [](PinInstance* inst, Pin* obj, std::vector<std::pair<iid, hid>>& source)
    {
        if (source.empty()) return;
        auto& pair = source.front();
        if (pair.first != 0) inst->set_instance_id(pair.first);
        if (pair.second != 0) obj->force_replace_hash(pair.second);
        source.erase(source.begin());
    };

    auto obj = g::pNodesManager->make_pin_ptr(get_object(), "INT", "select", -1);
    obj->def_state.min = int64_t(1);
    obj->def_state.max = int64_t(1);    
    if (deserialized_values != nullptr) obj->def = deserialized_values->selected;
    else obj->def = int64_t(1);
    obj->def_state.value = obj->def;
    obj->finalize_int();
    auto instance = wf->add_pin(this, obj.get());
    obj->flags.remove(PinObjFlags::CanRename);
    add_input(instance);
    if (deserialized_values != nullptr) assign_deserialized_value(instance, obj.get(), deserialized_values->in_ids);
    cni_add_pin(std::move(obj));
    m_pin_select = instance;

    obj = g::pNodesManager->make_pin_ptr(get_object(), "BOOLEAN", "sel_mode", -1);
    obj->label_on = "select_on_prompt";
    obj->label_off = "select_on_execution";
    if (deserialized_values != nullptr) obj->def = deserialized_values->mode;
    else obj->def = bool(false);
    obj->def_state.value = obj->def;
    obj->finalize_bool();
    instance = wf->add_pin(this, obj.get());    
    obj->flags.remove(PinObjFlags::CanRename);
    add_input(instance);
    if (deserialized_values != nullptr) assign_deserialized_value(instance, obj.get(), deserialized_values->in_ids);
    cni_add_pin(std::move(obj));
    m_pin_mode = instance;

    obj = g::pNodesManager->make_pin_ptr(get_object(), "*", "selected_value", 0);
    instance = wf->add_pin(this, obj.get());    
    obj->flags.remove(PinObjFlags::CanRename);
    add_output(instance);
    if (deserialized_values != nullptr) assign_deserialized_value(instance, obj.get(), deserialized_values->out_ids);
    cni_add_pin(std::move(obj));
    m_pin_out = instance;

    obj = g::pNodesManager->make_pin_ptr(get_object(), "STRING", "selected_label", 1);
    instance = wf->add_pin(this, obj.get());    
    obj->flags.remove(PinObjFlags::CanRename);
    add_output(instance);
    if (deserialized_values != nullptr) assign_deserialized_value(instance, obj.get(), deserialized_values->out_ids);
    cni_add_pin(std::move(obj));

    obj = g::pNodesManager->make_pin_ptr(get_object(), "INT", "selected_index", 2);
    instance = wf->add_pin(this, obj.get());    
    obj->flags.remove(PinObjFlags::CanRename);
    add_output(instance);
    if (deserialized_values != nullptr) assign_deserialized_value(instance, obj.get(), deserialized_values->out_ids);
    cni_add_pin(std::move(obj));

    if (deserialized_values != nullptr)
    {
        for (auto [pin_id, hash] : deserialized_values->in_dynamic_ids)
        {
            auto pin = add_pin();
            if (pin_id != 0) pin->set_instance_id(pin_id);
            if (hash != 0) pin->get_object()->force_replace_hash(hash);
        }
    }

    if (get_inputs().size() == 2)
        add_pin();

    const int64_t pins_count = cast::to_i64(get_inputs().size() - 2);
    m_pin_select->state().min = m_pin_select->get_object()->def_state.min = std::min(pins_count, int64_t(1));
    m_pin_select->state().max = m_pin_select->get_object()->def_state.max = pins_count;
    if (m_pin_select->state().value.get_int() > pins_count)
        *m_pin_select->state().value.get_int_ref() = pins_count;

    m_deserialized_values.reset();
}

bool CNI_SwitchImpact::cni_copy_from(const NodeInstance* copy_from_node, bool copy_pin_ids)
{
    auto& src_outs = copy_from_node->get_outputs();
    LogRAssert(!src_outs.empty(), "Source node pins empty out", true);

    auto that_node = cast::strict<CNI_SwitchImpact>(copy_from_node);
    LogRAssert(that_node, "Zero after conversion", true);

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedSwitchImpact>();
    auto switcher_data = cast::strict<CNI_DeserializedSwitchImpact>(data.get());

    auto& ins = copy_from_node->get_inputs();
    for (auto& in : ins)
    {
        if (in == that_node->m_pin_select)
        {
            switcher_data->in_ids.push_back(std::make_pair(copy_pin_ids ? in->get_instance_id() : 0, 0));
            switcher_data->selected = in->state().value.get_int();
        }
        else if (in == that_node->m_pin_mode)
        {
            switcher_data->in_ids.push_back(std::make_pair(copy_pin_ids ? in->get_instance_id() : 0, 0));
            switcher_data->mode = in->state().value.get_bool();
        }
        else
            switcher_data->in_dynamic_ids.push_back(std::make_pair(copy_pin_ids ? in->get_instance_id() : 0, in->get_object()->get_hash()));
    }
    auto& outs = copy_from_node->get_outputs();
    for (auto& out : outs)
    {
        switcher_data->out_ids.push_back(std::make_pair(copy_pin_ids ? out->get_instance_id() : 0, 0));
    }

    cni_save_deserialized_info(std::move(data));

    return true;
}

void CNI_SwitchImpact::cni_cleanup_base_object()
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
    m_base_type = PinType::ANY;
    m_pin_select = m_pin_mode = m_pin_out = nullptr;
}

PinInstance* CNI_SwitchImpact::add_pin()
{
    auto wf = g::pWfsManager->access(get_workflow_id());
    const std::string name = fmt::format(FMT_STRING("input{}"), get_inputs().size() - 2 + 1);
    auto obj = g::pNodesManager->make_pin_ptr(get_object(), "*", name, -1);
    obj->force_replace_hash(base::GlobalID::random());
    obj->flags.remove(PinObjFlags::InputCanBeWidget);
    auto instance = wf->add_pin(this, obj.get());
    obj->flags.remove(PinObjFlags::CanRename);
    add_input(instance);
    cni_add_pin(std::move(obj));

    if (m_base_type != PinType::ANY)
    {
        instance->state().type = m_base_type;
        instance->state().type_str = m_base_type_str;
    }
    return instance;
}

void CNI_SwitchImpact::cni_pin_linked(PinInstance* p1, PinInstance* p2)
{
    LogRAssert(p1 && p2, "Zero ptr", );

    auto& ins = get_inputs();
    auto& outs = get_outputs();
    LogRAssert(!ins.empty() && !outs.empty(), "Invalid node - zero in/out pins", );
    LogRAssert(m_pin_out && m_pin_select && m_pin_mode, "Invalid node - zero dedicated pins", );

    if (m_pin_out == p1 || m_pin_out == p2)
    {
        auto external = (m_pin_out == p1 ? p2 : p1);
        external->state().copy_to(m_pin_out->state());
        if (external->state().type != PinType::ANY)
        {
            m_base_type = external->state().type;
            m_base_type_str = external->state().type_str;
            set_base_type();
        }
        return;
    }
    for (auto out : outs)
    {
        if (out == p1 || out == p2)
            return;
    }

    PinInstance* internal = nullptr;
    PinInstance* external = nullptr;

    for (auto pin : ins)
    {
        if (pin == p1 || pin == p2)
        {
            if (m_pin_select == pin || m_pin_mode == pin)
                return;

            internal = (pin == p1 ? p1 : p2);
            external = (pin == p1 ? p2 : p1);
            break;
        }
    }
    LogRAssert(internal && external, "Linked pins not found", );

    if (ins[ins.size() - 1] == internal)
        add_pin();

    external->state().copy_to(internal->state());
    if (external->state().type != PinType::ANY)
    {
        m_base_type = external->state().type;
        m_base_type_str = external->state().type_str;
        set_base_type();
    }
}

void CNI_SwitchImpact::cni_pin_unlinked(const PinInstance* /*p1*/, const PinInstance* /*p2*/)
{
    LogRAssert(!get_inputs().empty() && !get_outputs().empty(), "Invalid node - zero in/out pins", );
    LogRAssert(m_pin_out && m_pin_select && m_pin_mode, "Invalid node - zero dedicated pins", );

    check_base_type();
}

void CNI_SwitchImpact::set_base_type()
{
    m_pin_out->state().type = m_base_type;
    m_pin_out->state().type_str = m_base_type_str;

    size_t index = 1;
    auto& ins = get_inputs();
    for (auto in : ins)
    {
        if (m_pin_select != in && m_pin_mode != in)
        {
            in->state().type = m_base_type;
            in->state().type_str = m_base_type_str;
            in->state().name = in->get_object()->def_state.name = fmt::format(FMT_STRING("input{}"), index++);
        }
    }

    const int64_t pins_count = cast::to_i64(ins.size() - 2);
    m_pin_select->state().min = m_pin_select->get_object()->def_state.min = std::min(pins_count, int64_t(1));
    m_pin_select->state().max = m_pin_select->get_object()->def_state.max = pins_count;
    if (m_pin_select->state().value.get_int() > pins_count)
        *m_pin_select->state().value.get_int_ref() = pins_count;
    else if (m_pin_select->state().value.get_int() == 0)
        *m_pin_select->state().value.get_int_ref() = 1;
}

void CNI_SwitchImpact::check_base_type()
{
    m_base_type = PinType::ANY;
    m_base_type_str.clear();
    const auto test_type = [this](PinInstance* pin)
    {
        if (0 == pin->state().links)
            return;

        if (m_base_type != PinType::ANY && pin->state().type != PinType::ANY)
            LogAssertS(m_base_type == pin->state().type, "Wrong pin type");

        if (pin->state().type != PinType::ANY)
        {
            m_base_type = pin->state().type;
            m_base_type_str = pin->state().type_str;
        }
    };

    test_type(m_pin_out);

    auto wf = g::pWfsManager->access(get_workflow_id());
    auto& ins = get_inputs();
    for (auto i = 0; i < ins.size(); i++)
    {
        auto in = ins[i];
        if (m_pin_select != in && m_pin_mode != in)
        {
            if (0 == in->state().links && i < ins.size() - 1)
            {
                std::erase_if(get_object()->input, [obj = in->get_object()](const Pin* p) { return obj == p; });
                cni_remove_pin(in->get_object());
                remove_pin(in);
                wf->remove_pin(in);
                i--;
            }
            else
                test_type(in);
        }
    }
    set_base_type();
}

CNI_DeserializedSwitchImpact CNI_SwitchImpact::dump() const noexcept
{
    auto& ins = get_inputs();
    auto& outs = get_outputs();
    LogRAssert(!ins.empty() && !outs.empty(), "Invalid node - zero in/out pins", {});
    LogRAssert(m_pin_out && m_pin_select && m_pin_mode, "Invalid node - zero dedicated pins", {});

    CNI_DeserializedSwitchImpact dump;
    for (auto pin : ins)
    {
        if (m_pin_select == pin)
        {
            dump.in_ids.push_back(std::make_pair(pin->get_instance_id(), 0));
            dump.selected = pin->state().value.get_int();
        }
        else if (m_pin_mode == pin)
        {
            dump.in_ids.push_back(std::make_pair(pin->get_instance_id(), 0));
            dump.mode = pin->state().value.get_bool();
        }
        else if (pin->state().links > 0)
            dump.in_dynamic_ids.push_back(std::make_pair(pin->get_instance_id(), pin->get_object()->get_hash()));
    }
    for (auto pin : outs)
    {
        dump.out_ids.push_back(std::make_pair(pin->get_instance_id(), 0));
    }
    return dump;
}

///////////////////////////////////////////////////////////////////////////////////////////////
/////////        CNI_SwitchInversedImpact                               ///////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

CNI_SwitchInversedImpact::CNI_SwitchInversedImpact(Node* obj, wid workflow_id, iid instance_id) noexcept
    : CNI_BaseObj(obj, workflow_id, instance_id)
{
    flags().add(NodeInstFlags::CNI, NodeInstFlags::JsonCustomSerialization, NodeInstFlags::BlobCustomSerialization);
}

void CNI_SwitchInversedImpact::cni_init()
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

    CNI_DeserializedSwitchImpact* deserialized_values = nullptr;
    if (m_deserialized_values)
    {
        deserialized_values = cast::dynamic<CNI_DeserializedSwitchImpact>(m_deserialized_values.get());
        LogAssert(deserialized_values, "Zero deserialized values");
    }

    const auto assign_deserialized_value = [](PinInstance* inst, Pin* obj, std::vector<std::pair<iid, hid>>& source)
    {
        if (source.empty()) return;
        auto& pair = source.front();
        if (pair.first != 0) inst->set_instance_id(pair.first);
        if (pair.second != 0) obj->force_replace_hash(pair.second);
        source.erase(source.begin());
    };

    auto obj = g::pNodesManager->make_pin_ptr(get_object(), "*", "input", -1);
    obj->flags.remove(PinObjFlags::InputCanBeWidget);
    auto instance = wf->add_pin(this, obj.get());    
    obj->flags.remove(PinObjFlags::CanRename);
    add_input(instance);
    if (deserialized_values != nullptr) assign_deserialized_value(instance, obj.get(), deserialized_values->in_ids);
    cni_add_pin(std::move(obj));
    m_pin_in = instance;

    obj = g::pNodesManager->make_pin_ptr(get_object(), "INT", "select", -1);
    obj->def_state.min = int64_t(1);
    obj->def_state.max = cast::to_i64(get_inputs().size());
    if (deserialized_values != nullptr) obj->def = deserialized_values->selected;
    else obj->def = int64_t(1);
    obj->def_state.value = obj->def;
    obj->finalize_int();
    instance = wf->add_pin(this, obj.get());    
    obj->flags.remove(PinObjFlags::CanRename);
    add_input(instance);
    if (deserialized_values != nullptr) assign_deserialized_value(instance, obj.get(), deserialized_values->in_ids);
    cni_add_pin(std::move(obj));
    m_pin_select = instance;

    obj = g::pNodesManager->make_pin_ptr(get_object(), "BOOLEAN", "sel_mode", -1);
    obj->label_on = "select_on_prompt";
    obj->label_off = "select_on_execution";
    if (deserialized_values != nullptr) obj->def = deserialized_values->mode;
    else obj->def = bool(false);
    obj->def_state.value = obj->def;
    obj->finalize_bool();
    instance = wf->add_pin(this, obj.get());    
    obj->flags.remove(PinObjFlags::CanRename);
    add_input(instance);
    if (deserialized_values != nullptr) assign_deserialized_value(instance, obj.get(), deserialized_values->in_ids);
    cni_add_pin(std::move(obj));
    m_pin_mode = instance;

    if (deserialized_values != nullptr)
    {
        for (auto [pin_id, hash] : deserialized_values->out_dynamic_ids)
        {
            auto pin = add_pin();
            if (pin_id != 0) pin->set_instance_id(pin_id);
            if (hash != 0) pin->get_object()->force_replace_hash(hash);
        }
    }
    if (get_outputs().size() == 0)
        add_pin();

    const int64_t pins_count = cast::to_i64(get_outputs().size());
    m_pin_select->state().min = m_pin_select->get_object()->def_state.min = std::min(pins_count, int64_t(1));
    m_pin_select->state().max = m_pin_select->get_object()->def_state.max = pins_count;
    if (m_pin_select->state().value.get_int() > pins_count)
        *m_pin_select->state().value.get_int_ref() = pins_count;

    m_deserialized_values.reset();
}

bool CNI_SwitchInversedImpact::cni_copy_from(const NodeInstance* copy_from_node, bool copy_pin_ids)
{
    auto& src_outs = copy_from_node->get_outputs();
    LogRAssert(!src_outs.empty(), "Source node pins empty out", true);

    auto that_node = cast::strict<CNI_SwitchInversedImpact>(copy_from_node);
    LogRAssert(that_node, "Zero after conversion", true);

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedSwitchImpact>();
    auto switcher_data = cast::strict<CNI_DeserializedSwitchImpact>(data.get());

    auto& ins = copy_from_node->get_inputs();
    for (auto& in : ins)
    {
        if (in == that_node->m_pin_select)
            switcher_data->selected = in->state().value.get_int();
        else if (in == that_node->m_pin_mode)
            switcher_data->mode = in->state().value.get_bool();
        switcher_data->in_ids.push_back(std::make_pair(copy_pin_ids ? in->get_instance_id() : 0, 0));
    }
    auto& outs = copy_from_node->get_outputs();
    for (auto& out : outs)
    {
        switcher_data->out_dynamic_ids.push_back(std::make_pair(copy_pin_ids ? out->get_instance_id() : 0, out->get_object()->get_hash()));
    }

    cni_save_deserialized_info(std::move(data));

    return true;
}

void CNI_SwitchInversedImpact::cni_cleanup_base_object()
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
    m_base_type = PinType::ANY;
    m_pin_in = m_pin_select = m_pin_mode = nullptr;
}

PinInstance* CNI_SwitchInversedImpact::add_pin()
{
    auto wf = g::pWfsManager->access(get_workflow_id());
    const std::string name = fmt::format(FMT_STRING("output{}"), get_outputs().size() + 1);
    auto obj = g::pNodesManager->make_pin_ptr(get_object(), "*", name, m_last_out_index++);
    obj->force_replace_hash(base::GlobalID::random());
    auto instance = wf->add_pin(this, obj.get());
    obj->flags.remove(PinObjFlags::CanRename);
    add_output(instance);
    cni_add_pin(std::move(obj));

    if (m_base_type != PinType::ANY)
    {
        instance->state().type = m_base_type;
        instance->state().type_str = m_base_type_str;
    }
    return instance;
}

void CNI_SwitchInversedImpact::cni_pin_linked(PinInstance* p1, PinInstance* p2)
{
    LogRAssert(p1 && p2, "Zero ptr", );

    auto& ins = get_inputs();
    auto& outs = get_outputs();
    LogRAssert(!ins.empty() && !outs.empty(), "Invalid node - zero in/out pins", );
    LogRAssert(m_pin_in && m_pin_select && m_pin_mode, "Invalid node - zero dedicated pins", );

    if (m_pin_in == p1 || m_pin_in == p2)
    {
        auto external = (m_pin_in == p1 ? p2 : p1);
        external->state().copy_to(m_pin_in->state());
        if (external->state().type != PinType::ANY)
        {
            m_base_type = external->state().type;
            m_base_type_str = external->state().type_str;
            set_base_type();
        }
        return;
    }
    for (auto in : ins)
    {
        if (in == p1 || in == p2)
            return;
    }

    PinInstance* internal = nullptr;
    PinInstance* external = nullptr;

    for (auto pin : outs)
    {
        if (pin == p1 || pin == p2)
        {
            internal = (pin == p1 ? p1 : p2);
            external = (pin == p1 ? p2 : p1);
            break;
        }
    }
    LogRAssert(internal && external, "Linked pins not found", );

    if (outs[outs.size() - 1] == internal)
        add_pin();

    external->state().copy_to(internal->state());
    if (external->state().type != PinType::ANY)
    {
        m_base_type = external->state().type;
        m_base_type_str = external->state().type_str;
        set_base_type();
    }
}

void CNI_SwitchInversedImpact::cni_pin_unlinked(const PinInstance* /*p1*/, const PinInstance* /*p2*/)
{
    LogRAssert(!get_inputs().empty() && !get_outputs().empty(), "Invalid node - zero in/out pins", );
    LogRAssert(m_pin_in && m_pin_select && m_pin_mode, "Invalid node - zero dedicated pins", );

    check_base_type();
}

void CNI_SwitchInversedImpact::set_base_type()
{
    m_pin_in->state().type = m_base_type;
    m_pin_in->state().type_str = m_base_type_str;

    size_t index = 1;
    auto& outs = get_outputs();
    for (auto out : outs)
    {
        out->state().type = m_base_type;
        out->state().type_str = m_base_type_str;
        out->state().name = out->get_object()->def_state.name = fmt::format(FMT_STRING("output{}"), index++);
    }

    const int64_t pins_count = cast::to_i64(outs.size());
    m_pin_select->state().min = m_pin_select->get_object()->def_state.min = std::min(pins_count, int64_t(1));
    m_pin_select->state().max = m_pin_select->get_object()->def_state.max = pins_count;
    if (m_pin_select->state().value.get_int() > pins_count)
        *m_pin_select->state().value.get_int_ref() = pins_count;
    else if (m_pin_select->state().value.get_int() == 0)
        *m_pin_select->state().value.get_int_ref() = 1;
}

void CNI_SwitchInversedImpact::check_base_type()
{
    m_base_type = PinType::ANY;
    m_base_type_str.clear();
    const auto test_type = [this](PinInstance* pin)
    {
        if (0 == pin->state().links)
            return;

        if (m_base_type != PinType::ANY && pin->state().type != PinType::ANY)
            LogAssertS(m_base_type == pin->state().type, "Wrong pin type");

        if (pin->state().type != PinType::ANY)
        {
            m_base_type = pin->state().type;
            m_base_type_str = pin->state().type_str;
        }
    };

    test_type(m_pin_in);

    auto wf = g::pWfsManager->access(get_workflow_id());
    auto& outs = get_outputs();
    for (auto i = 0; i < outs.size(); i++)
    {
        auto out = outs[i];
        if (0 == out->state().links && i < outs.size() - 1)
        {
            std::erase_if(get_object()->output, [obj = out->get_object()](const Pin* p) { return obj == p; });
            cni_remove_pin(out->get_object());
            remove_pin(out);
            wf->remove_pin(out);
            i--;
        }
        else
            test_type(out);
    }
    set_base_type();
}

CNI_DeserializedSwitchImpact CNI_SwitchInversedImpact::dump() const noexcept
{
    auto& ins = get_inputs();
    auto& outs = get_outputs();
    LogRAssert(!ins.empty() && !outs.empty(), "Invalid node - zero in/out pins", {});
    LogRAssert(m_pin_in && m_pin_select, "Invalid node - zero dedicated pins", {});

    CNI_DeserializedSwitchImpact dump;
    for (auto pin : ins)
    {
        if (m_pin_select == pin)
            dump.selected = pin->state().value.get_int();
        else if (m_pin_mode == pin)
            dump.mode = pin->state().value.get_bool();
        dump.in_ids.push_back(std::make_pair(pin->get_instance_id(), 0));
    }
    for (auto pin : outs)
    {
        if (pin->state().links > 0)
            dump.out_dynamic_ids.push_back(std::make_pair(pin->get_instance_id(), pin->get_object()->get_hash()));
    }
    return dump;
}

} // namespace data


