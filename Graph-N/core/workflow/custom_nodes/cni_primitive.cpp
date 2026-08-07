#include "core/workflow/custom_nodes/cni_primitive.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/workflow.h"
#include "core/workflow/link.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/workflows_manager.h"
#include "ui/graph-n_flow.h"
#include "imgui_widget_limits.h"

namespace data
{

std::unique_ptr<Node> CNI_Primitive::cni_create_object(mngr::NodesManager* manager)
{
    if (nullptr == manager)
        manager = g::pNodesManager;

    auto node = std::make_unique<data::Node>(cni_get_node_class());
    node->name = node->display_name = "Primitive";
    node->category = "utils";
    node->description = "Primitive values like numbers, strings, and booleans.";
    node->can_bypass = false;

    node->finalize(nullptr);
    return node;
}

void CNI_Primitive::cni_init()
{
    auto wf = g::pWfsManager->access(get_workflow_id());

    for (auto input : m_input)
    {
        cni_remove_pin(input->get_object());
        g::pNodesManager->remove_pin(get_object(), input->get_object());
        wf->remove_pin(input);
    }
    m_input.clear();

    for (auto output : m_output)
    {
        cni_remove_pin(output->get_object());
        g::pNodesManager->remove_pin(get_object(), output->get_object());
        wf->remove_pin(output);
    }
    m_output.clear();

    iid out_id = 0;

    CNI_DeserializedPrimitive* deserialized_values = nullptr;
    if (m_deserialized_values)
    {
        deserialized_values = cast::dynamic<CNI_DeserializedPrimitive>(m_deserialized_values.get());
        LogAssert(deserialized_values, "Zero deserialized values");

        if (deserialized_values != nullptr)
            out_id = deserialized_values->out_id;
        //{
        //    auto& outs = get_outputs();
        //    if (!outs.empty())
        //    {
        //        cni_remove_pin(outs[0]->get_object());
        //        remove_pin(outs[0]);
        //        wf->remove_pin(outs[0]);
        //    }

        //    auto obj = g::pNodesManager->make_pin_ptr(get_object(), "*", "*", 0);
        //    auto instance = wf->add_pin(this, obj.get(), deserialized_values->out_id);
        //    obj->can_rename = false;
        //    add_output(instance);
        //    cni_add_pin(std::move(obj));
        //}
    }

    auto obj = g::pNodesManager->make_pin_ptr(get_object(), "*", "connect to widget input", 0);
    auto instance = wf->add_pin(this, obj.get(), out_id);
    obj->can_rename = false;
    add_output(instance);
    cni_add_pin(std::move(obj));    

    //obj = g::pNodesManager->make_pin_ptr(get_object(), "*", "*", 0);
    //instance = wf->add_pin(this, obj.get());
    //if (out_id != 0)
    //    instance->set_instance_id(out_id);
    //obj->can_rename = false;
    //add_output(instance);
    //cni_add_pin(std::move(obj));
}

void CNI_Primitive::cni_deserialilzation_begins() noexcept
{
    m_deserialization_mode = true;
}

void CNI_Primitive::cni_deserialilzation_ends()
{
    m_deserialization_mode = false;
    update();
}

bool CNI_Primitive::cni_copy_from(const NodeInstance* copy_from_node, bool copy_pin_ids)
{
    auto& outs = copy_from_node->get_outputs();
    auto& src_outs = copy_from_node->get_outputs();
    LogRAssert(!outs.empty() && !src_outs.empty(), "Source node pins empty out", true);

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedPrimitive>();
    if (copy_pin_ids)
        ((CNI_DeserializedPrimitive*)data.get())->out_id = src_outs[0]->get_instance_id();
    ((CNI_DeserializedPrimitive*)data.get())->out_pin_name = src_outs[0]->state().name;
    ((CNI_DeserializedPrimitive*)data.get())->out_pin_type = src_outs[0]->state().type_str;

    auto& ins = copy_from_node->get_inputs();
    if (ins.size() >= 1)
        ((CNI_DeserializedPrimitive*)data.get())->widget_value = ins[0]->state().value;
    if (ins.size() >= 2)
        ((CNI_DeserializedPrimitive*)data.get())->cag_value = ins[1]->state().value;

    cni_save_deserialized_info(std::move(data));

    return true;
}

void CNI_Primitive::cni_cleanup_base_object() noexcept
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

template <typename TYPE>
bool test_ranges(TYPE src_min, TYPE src_max, TYPE dest_min, TYPE dest_max) noexcept
{
    return (src_min <= dest_max) && (dest_min <= src_max);
}

bool CNI_Primitive::cni_can_link(const PinInstance* p1, const PinInstance* p2) const noexcept
{
    LogRAssert(p1 && p2, "Zero ptr", false);
    auto& src_outputs = get_outputs();
    LogRAssert(!src_outputs.empty(), "Empty outputs", false);

    auto out_pin = src_outputs[0];
    const PinInstance* linking_pin = nullptr;
    if (out_pin == p1)
        linking_pin = p2;
    else
        linking_pin = p1;

    const auto dest_type = linking_pin->state().type;

    if (m_links.empty())
        return Pin::is_primitive_type(dest_type);

    const auto& src_inputs = get_inputs();
    LogRAssert(!src_inputs.empty(), "Empty inputs", false);

    const auto dest_obj = linking_pin->get_object();
    const auto& dest_val = linking_pin->state().value;

    const auto src_pin = src_inputs[0];
    const auto& src_state = src_pin->state();
    const auto& src_value = src_pin->state().value;
    const auto src_obj = src_pin->get_object();

    if (PinType::INT == dest_type && PinType::INT == src_state.type)
    {
        LogRAssert(src_inputs.size() == 2, "Inputs wrong size. w: " << get_workflow_id() << ", n:" << get_instance_id() << ", sz: " << src_inputs.size(), false);

        const bool dest_u = dest_val.is_int();
        const bool src_u = src_value.is_int();
        if (dest_u != src_u)
            return false;
        if (dest_u == src_u && src_u)
            return true;
        return test_ranges(src_obj->min.get_int(), src_obj->max.get_int(), dest_obj->min.get_int(), dest_obj->max.get_int());
    }

    if (PinType::FLOAT == dest_type && PinType::FLOAT == src_state.type)
    {
        LogRAssert(src_inputs.size() == 2, "Inputs wrong size. w: " << get_workflow_id() << ", n:" << get_instance_id() << ", sz: " << src_inputs.size(), false);
        return test_ranges(src_obj->min.get_float(), src_obj->max.get_float(), dest_obj->min.get_float(), dest_obj->max.get_float());
    }

    if (PinType::NUMBER == dest_type && PinType::NUMBER == src_state.type)
    {
        LogRAssert(src_inputs.size() == 2, "Inputs wrong size. w: " << get_workflow_id() << ", n:" << get_instance_id() << ", sz: " << src_inputs.size(), false);
        return test_ranges(src_obj->min.get_number(), src_obj->max.get_number(), dest_obj->min.get_number(), dest_obj->max.get_number());
    }

    if (PinType::COMBO == dest_type && PinType::COMBO == src_state.type)
    {
        LogRAssert(src_inputs.size() == 2, "Inputs wrong size. w: " << get_workflow_id() << ", n:" << get_instance_id() << ", sz: " << src_inputs.size(), false);

        if (dest_obj->combo_items_str.size() != src_obj->combo_items_str.size())
            return false;
        for (size_t i = 0; i < dest_obj->combo_items_str.size(); i++)
        {
            if (dest_obj->combo_items_str[i] != src_obj->combo_items_str[i])
                return false;
        }
        return true;
    }

    if (PinType::STRING == dest_type && PinType::STRING == src_state.type)
        return true;

    if (PinType::BOOLEAN == dest_type && PinType::BOOLEAN == src_state.type)
        return true;

    return false;
}

void CNI_Primitive::update()
{
    if (m_links.empty())
    {
        auto& outs = get_outputs();
        LogRAssert(!outs.empty(), "Empty out pins", );

        auto pin = outs[0];
        pin->get_object()->can_rename = false;
        pin->state().name = pin->get_object()->def_state.name;

        auto& ins = get_inputs();
        if (!ins.empty())
        {
            auto wf = g::pWfsManager->access(get_workflow_id());
            while (!ins.empty())
            {
                pin = ins.front();
                cni_remove_pin(pin->get_object());
                remove_pin(pin);
                wf->remove_pin(pin);
            }
        }
        return;
    }

    m_has_in_pins = true;

    auto src_pin_inst = *m_links.begin();
    const auto src_pin_obj = src_pin_inst->get_object();
    const auto src_type = src_pin_inst->state().type;
    const auto& src_val = src_pin_inst->state().value;
    const std::string& pin_name = src_pin_inst->state().name;
    auto wf = g::pWfsManager->access(get_workflow_id());

    CNI_DeserializedPrimitive* deserialized_values = nullptr;
    if (m_deserialized_values)
    {
        deserialized_values = cast::dynamic<CNI_DeserializedPrimitive>(m_deserialized_values.get());
        LogAssert(deserialized_values, "Zero deserialized values");
    }

    if (!m_linked)
    {
        auto this_node_obj = get_object();

        if (PinType::INT == src_type)
        {
            auto this_pin_obj = g::pNodesManager->make_pin_ptr(this_node_obj, "INT", pin_name, -1);
            src_pin_obj->copy_int(this_pin_obj.get());
            this_pin_obj->cag_require = true;
            this_pin_obj->can_link = false;

            auto instance = wf->add_pin(this, this_pin_obj.get());
            src_pin_inst->state().copy_to(instance->state());

            if (deserialized_values != nullptr && instance->state().value.is_int() && deserialized_values->widget_value.is_int())
            {
                instance->state().value = deserialized_values->widget_value;
            }

            add_input(instance);
            cni_add_pin(std::move(this_pin_obj));
        }

        else if (PinType::FLOAT == src_type)
        {
            auto this_pin_obj = g::pNodesManager->make_pin_ptr(this_node_obj, "FLOAT", pin_name, -1);
            src_pin_obj->copy_float(this_pin_obj.get());
            this_pin_obj->cag_require = true;
            this_pin_obj->can_link = false;

            auto instance = wf->add_pin(this, this_pin_obj.get());
            src_pin_inst->state().copy_to(instance->state());

            if (deserialized_values != nullptr && instance->state().value.is_float() && deserialized_values->widget_value.is_float())
                instance->state().value = deserialized_values->widget_value;

            add_input(instance);
            cni_add_pin(std::move(this_pin_obj));
        }

        else if (PinType::NUMBER == src_type)
        {
            auto this_pin_obj = g::pNodesManager->make_pin_ptr(this_node_obj, "NUMBER", pin_name, -1);
            src_pin_obj->copy_number(this_pin_obj.get());
            this_pin_obj->cag_require = true;
            this_pin_obj->can_link = false;

            auto instance = wf->add_pin(this, this_pin_obj.get());
            src_pin_inst->state().copy_to(instance->state());

            if (deserialized_values != nullptr && instance->state().value.is_number() && deserialized_values->widget_value.is_number())
                instance->state().value = deserialized_values->widget_value;

            add_input(instance);
            cni_add_pin(std::move(this_pin_obj));
        }

        else if (PinType::COMBO == src_type)
        {
            auto this_pin_obj = g::pNodesManager->make_pin_ptr(this_node_obj, "COMBO", pin_name, -1);
            src_pin_obj->copy_combo(this_pin_obj.get());
            this_pin_obj->cag_require = true;
            this_pin_obj->can_link = false;

            auto instance = wf->add_pin(this, this_pin_obj.get());
            src_pin_inst->state().copy_to(instance->state());

            if (deserialized_values != nullptr && instance->state().value.is_int() && deserialized_values->widget_value.is_int())
                instance->state().value = deserialized_values->widget_value;

            add_input(instance);
            cni_add_pin(std::move(this_pin_obj));
        }

        else if (PinType::STRING == src_type)
        {
            auto this_pin_obj = g::pNodesManager->make_pin_ptr(this_node_obj, "STRING", pin_name, -1);
            src_pin_obj->copy_string(this_pin_obj.get());
            this_pin_obj->cag_require = false;
            this_pin_obj->can_link = false;
            this_pin_obj->multiline = true;

            auto instance = wf->add_pin(this, this_pin_obj.get());
            src_pin_inst->state().copy_to(instance->state());

            if (deserialized_values != nullptr && instance->state().value.is_string() && deserialized_values->widget_value.is_string())
                instance->state().value = deserialized_values->widget_value;

            add_input(instance);
            cni_add_pin(std::move(this_pin_obj));
        }

        else if (PinType::BOOLEAN == src_type)
        {
            auto this_pin_obj = g::pNodesManager->make_pin_ptr(this_node_obj, "BOOLEAN", pin_name, -1);
            src_pin_obj->copy_bool(this_pin_obj.get());
            this_pin_obj->cag_require = false;
            this_pin_obj->can_link = false;

            auto instance = wf->add_pin(this, this_pin_obj.get());
            src_pin_inst->state().copy_to(instance->state());

            if (deserialized_values != nullptr && instance->state().value.is_bool() && deserialized_values->widget_value.is_bool())
                instance->state().value = deserialized_values->widget_value;

            add_input(instance);
            cni_add_pin(std::move(this_pin_obj));
        }

        if (PinType::INT == src_type || PinType::FLOAT == src_type|| PinType::NUMBER == src_type || PinType::COMBO == src_type)
        {
            auto this_pin_obj = g::pNodesManager->make_pin_ptr(this_node_obj, "COMBO", base::GlobalID::uuid(), -1);
            this_pin_obj->cag_pin = true;
            this_pin_obj->finalize_combo();
            this_pin_obj->cag_require = false;
            this_pin_obj->can_link = false;
            this_pin_obj->def = cast::to_i64(ControlAfterGenerate::CAGFixed);
            this_pin_obj->def_state.name = "control";
            this_pin_obj->def_state.value = this_pin_obj->def;

            auto instance = wf->add_pin(this, this_pin_obj.get());
            this_pin_obj->def_state.copy_to(instance->state());

            if (deserialized_values != nullptr)
            {
                if (deserialized_values->cag_value.is_int())
                    instance->state().value = deserialized_values->cag_value;
                else if (deserialized_values->cag_value.is_string())
                {
                    auto& value = deserialized_values->cag_value.get_string();
                    for (size_t i = 0; i < this_pin_obj->combo_items.size(); i++)
                    {
                        if (value == this_pin_obj->combo_items[i].get_string())
                        {
                            instance->state().value = cast::to_i64(i);
                            break;
                        }
                    }
                }
            }

            add_input(instance);
            cni_add_pin(std::move(this_pin_obj));
        }

        link_cag_pins();

        auto& ins = get_inputs();
        auto& outs = get_outputs();
        LogRAssert(!ins.empty() && !outs.empty(), "Empty pins", );

        auto out = outs[0];
        auto in = ins[0];
        //in->get_object()->def_state.copy_to(out->get_object()->def_state);
        out->state().value = in->state().value;
        out->get_object()->min = in->get_object()->min;
        out->get_object()->max = in->get_object()->max;
        out->get_object()->step = in->get_object()->step;
        out->get_object()->def = in->get_object()->def;
        out->get_object()->can_rename = true;
        if (deserialized_values != nullptr)
            out->state().name = deserialized_values->out_pin_name;

        m_deserialized_values.reset();
        m_linked = true;
    }

    const auto& inputs = get_inputs();
    auto this_pin_obj = inputs[0]->get_object();

    if (src_val.is_int())
    {
        int64_t this_min = IM_S64_MIN_WIDGET;
        int64_t this_max = IM_S64_MAX_WIDGET;

        for (auto link : m_links)
        {
            const int64_t min = link->get_object()->min.get_int();
            const int64_t max = link->get_object()->max.get_int();
            this_min = std::max(this_min, min);
            this_max = std::min(this_max, max);
        }

        this_pin_obj->min = this_min;
        this_pin_obj->max = this_max;

        auto& val = inputs[0]->state().value;
        val = std::clamp(val.get_int(), this_min, this_max);
    }

    else if (src_val.is_float())
    {
        double this_min = IM_DBL_MIN_WIDGET;
        double this_max = IM_DBL_MAX_WIDGET;

        for (auto link : m_links)
        {
            const double min = link->get_object()->min.get_float();
            const double max = link->get_object()->max.get_float();
            this_min = std::max(this_min, min);
            this_max = std::min(this_max, max);
        }

        this_pin_obj->min = this_min;
        this_pin_obj->max = this_max;

        auto& val = inputs[0]->state().value;
        val = std::clamp(val.get_float(), this_min, this_max);
    }

    else if (src_val.is_number())
    {
        static_assert(std::same_as<number_t, double>, "Check this function if number_t changes");

        number this_min = IM_NUM_MIN_WIDGET;
        number this_max = IM_NUM_MAX_WIDGET;

        for (auto link : m_links)
        {
            const number min = link->get_object()->min.get_number();
            const number max = link->get_object()->max.get_number();
            this_min = std::max(this_min, min);
            this_max = std::min(this_max, max);
        }

        this_pin_obj->min = this_min;
        this_pin_obj->max = this_max;

        auto& val = inputs[0]->state().value;
        val = std::clamp(val.get_number(), this_min, this_max);
    }

    reset_width();
}

void CNI_Primitive::cni_pin_linked(PinInstance* p1, PinInstance* p2)
{
    LogRAssert(p1 && p2, "Zero ptr", );
    LogRAssert(cni_can_link(p1, p2),
        "Check CNI_primitive can link before entablish connection. w:" << get_workflow_id() << ", n: " << get_instance_id() << ", p1:" << p1->get_instance_id() << ", p2:" << p2->get_instance_id(), );

    auto& src_outputs = get_outputs();
    LogRAssert(!src_outputs.empty(), "Empty outputs", );

    auto out_pin = src_outputs[0];
    if (out_pin == p1)
        m_links.push_back(p2);
    else
        m_links.push_back(p1);

    Log("Primitive linked. w:" << get_workflow_id() << ", n:" << get_instance_id() <<
        (p1 == out_pin ? ", int:" : ", ext:") << p1->get_instance_id() <<
        (p1 == out_pin ? ", ext:" : ", int:") << p2->get_instance_id());

    update();
}

void CNI_Primitive::cni_mirror_value_to(const PinInstance* src, std::vector<PinInstance*>& append_pins) const
{
    LogRAssert(src, "Zero ptr", );
    const auto& inputs = get_inputs();
    if (inputs.empty() || inputs[0] != src)
        return;

    const auto& outputs = get_outputs();
    if (!outputs.empty() && outputs[0]->state().type == src->state().type)
        outputs[0]->state().value = src->state().value;

    std::ranges::for_each(m_links, [&append_pins](PinInstance* pin) { append_pins.push_back(pin); });
}

std::list<PinType> CNI_Primitive::get_supported_pin_types(PinInstance* pin) const
{
    LogRAssert(pin, "Zero ptr", std::list<PinType>{});
    if (pin->get_object()->input)
        return { PinType::_none };

    if (PinType::ANY == pin->state().type)
        return { PinType::INT, PinType::FLOAT, PinType::NUMBER, PinType::BOOLEAN, PinType::STRING, PinType::COMBO };

    return { pin->state().type };
}

void CNI_Primitive::cni_pin_unlinked(const PinInstance* p1, const PinInstance* p2)
{
    LogRAssert(p1 && p2, "Zero ptr", );
    if (m_links.size() == 1)
    {
        auto wf = g::pWfsManager->access(get_workflow_id());

        auto inputs_copy = get_inputs();
        for (auto instance : inputs_copy)
        {
            auto pin = instance->get_object();

            if (PinType::STRING == instance->state().type)
                g::pFlow->txt_panel_delete(wf->get_id(), this);

            wf->remove_pin(instance);
            remove_pin(instance);
            cni_remove_pin(pin);
        }

        m_has_in_pins = false;
        m_linked = false;
        m_links.clear();
        cni_recreate_base_object();

        auto& outs = get_outputs();
        LogRAssert(!outs.empty(), "Empty out pins", );

        auto out = outs[0];
        out->get_object()->def_state.copy_to(out->state());
        return;
    }

    auto& outs = get_outputs();
    LogRAssert(!outs.empty(), "Empty out pins", );

    auto out_pin = outs[0];
    const PinInstance* external_pin = (out_pin == p1) ? p2 : p1;
    std::erase_if(m_links, [external_pin](const PinInstance* pin) { return pin == external_pin; });

    Log("Primitive unlinked. w:" << get_workflow_id() << ", n:" << get_instance_id() << ", p1:" << p1->get_instance_id() << ", p2:" << p2->get_instance_id());

    update();
}

} // namespace data
