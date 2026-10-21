/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "core/workflow/serializer/wf_deserializer_from_json.h"
#include "core/workflow/custom_nodes/all_impl.h"
#include "core/workflow/pin_instance.h"
#include "core/managers/nodes_manager.h"

namespace data
{

void WfDeserializerFromJson::deserialize_cni_node(const o_json& j, ParseResult& result)
{
    switch (result.node_inst->get_object()->get_hash())
    {
    case js::VANILLA_PRIMITIVE_NODE:
        return deserialize_cni_node_primitive(j, result);
    case js::RGTHREE_POWER_PRIMITIVE:
        result.node_inst->get_object()->node_class = js::VANILLA_PRIMITIVE_NODE_CLASS;
        result.node_inst->get_object()->force_replace_hash(js::VANILLA_PRIMITIVE_NODE);
        return deserialize_cni_node_primitive(j, result);
    case js::VANILLA_REROUTE:
        return deserialize_cni_node_reroute(j, result);
    case js::PYSSSSS_REROUTE:
    case js::RGTHREE_REROUTE:
        result.node_inst->get_object()->node_class = js::VANILLA_REROUTE_CLASS;
        result.node_inst->get_object()->force_replace_hash(js::VANILLA_REROUTE);
        return deserialize_cni_node_reroute(j, result);
    case js::VANILLA_NOTE:
        return deserialize_cni_node_note(j, result);
    case js::VANILLA_MARKDOWN_NOTE:
        return deserialize_cni_node_markdownnote(j, result);
    case js::RGTHREE_FAST_BYPASSER:
    case js::RGTHREE_FAST_MUTER:
        return deserialize_cni_fast_switcher_rgthree(j, result);
    case js::RGTHREE_FAST_GROUPS_BYPASSER:
    case js::RGTHREE_FAST_GROUPS_MUTER:
        return deserialize_cni_fast_group_switcher_rgthree(j, result);
    case js::RGTHREE_FAST_ACTIONS_BUTTON:
        return deserialize_cni_fast_action_rgthree(j, result);
    case js::RGTHREE_BOOKMARK:
        return deserialize_cni_bookmark_rgthree(j, result);
    case js::IMPACT_SWITCH:
        return deserialize_cni_node_impact_switch(j, result);
    case js::IMPACT_INVERSED_SWITCH:
        return deserialize_cni_node_impact_insversed_switch(j, result);
    }

    if (result.node_inst->obj_flags().has(NodeObjFlags::CustomJsNode))
        return deserialize_js_node(j, result);

    LogError("Unhandled cni node");
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    Primitive                      //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void WfDeserializerFromJson::deserialize_cni_node_primitive(const o_json& j, ParseResult& result)
{
    const auto pin_hash = Pin::make_hash(result.node_inst->get_object(), "*", 0);
    auto pin = result.node_inst->get_object()->find_pin(pin_hash, false); // do not use pNodesManager
    if (nullptr == pin)
    {
        result.unrecognized_node_definition = true;
        return;
    }
    pin->finalize();

    const auto* pin_instance = m_wf->get_pin_by_hash(result.node_inst->get_instance_id(), pin_hash);    
    if (nullptr == pin_instance)
    {
        result.unrecognized_node_definition = true;
        return;
    }

    m_node_out_pins[result.node_inst->get_instance_id()][0] = pin_instance->get_instance_id();

    if (j.contains("widgets_values") && j["widgets_values"].is_array() && !j["widgets_values"].empty())
    {
        std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedPrimitive>();

        const auto& jvalue0 = j["widgets_values"][0];
        auto& value = ((CNI_DeserializedPrimitive*)data.get())->widget_value;

        if (jvalue0.is_boolean())
            value = jvalue0.get<bool>();
        else if (jvalue0.is_number_float())
            value = jvalue0.get<double>();
        else if (jvalue0.is_string())
            value = jvalue0.get<std::string>();
        else if (jvalue0.is_number_integer())
            value = jvalue0.get<int64_t>();

        if (j["widgets_values"].size() > 1)
        {
            const auto& jvalue1 = j["widgets_values"][1];
            if (jvalue1.is_string())
            {
                const auto cag_val = jvalue1.get<std::string>();
                if (cag_val == "fixed")             cast::strict<CNI_DeserializedPrimitive>(data.get())->cag_value = ControlAfterGenerate::Fixed;
                else if (cag_val == "increment")    cast::strict<CNI_DeserializedPrimitive>(data.get())->cag_value = ControlAfterGenerate::Increment;
                else if (cag_val == "decrement")    cast::strict<CNI_DeserializedPrimitive>(data.get())->cag_value = ControlAfterGenerate::Decrement;
                else if (cag_val == "randomize")    cast::strict<CNI_DeserializedPrimitive>(data.get())->cag_value = ControlAfterGenerate::Randomize;
            }
        }

        result.node_inst->cni_save_deserialized_info(std::move(data));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    Reroute                        //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void WfDeserializerFromJson::deserialize_cni_node_reroute(const o_json& j, ParseResult& result)
{
    auto reroute = cast::dynamic<CNI_Reroute>(result.node_inst);
    LogRAssert(reroute, "Node is not CNI. w:" << m_wf->get_id() << ", n: " << result.node_inst->get_instance_id() << ", t:" << typeid(result.node_inst).name(), );

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedReroute>();

    RoutedPinPair pair;

    if (j.contains("inputs") && j["inputs"].is_array())
    {
        for (const auto& jinput : j["inputs"])
        {
            if (jinput.is_object() && jinput.contains("link") && !jinput["link"].is_null() && jinput["link"].is_number_integer())
            {
                pair.in_id = m_wf->get_new_child_id();
                break;
            }
        }
    }
    if (j.contains("outputs") && j["outputs"].is_array())
    {
        for (const auto& joutput : j["outputs"])
        {
            if (joutput.is_object() && joutput.contains("links") && !joutput["links"].is_null() && joutput["links"].is_array() && !joutput["links"].empty())
            {
                pair.out_id = m_wf->get_new_child_id();
            }
            if (joutput.is_object() && joutput.contains("type") && joutput["type"].is_string())
            {
                pair.type = joutput["type"];
            }
        }
    }

    if (0 == pair.in_id && 0 == pair.out_id)
        return;

    if (0 == pair.in_id) pair.in_id = m_wf->get_new_child_id();
    if (0 == pair.out_id) pair.out_id = m_wf->get_new_child_id();
    
    cast::strict<CNI_DeserializedReroute>(data.get())->poses = { result.node_inst->m_rc, result.node_inst->m_rc };
    cast::strict<CNI_DeserializedReroute>(data.get())->pairs.push_back(pair);
    
    reroute->cni_save_deserialized_info(std::move(data));
    reroute->cni_init();

    m_node_in_pins[result.node_inst->get_instance_id()][0] = pair.in_id;
    m_node_out_pins[result.node_inst->get_instance_id()][0] = pair.out_id;
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    Note, MarkdownNote             //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

template<typename N>
void deserialize_cni_node_note_impl(Workflow* wf, const o_json& j, ParseResult& result)
{
    auto note = cast::dynamic<N>(result.node_inst);
    LogRAssertS(note, "Node is not CNI. w:" << wf->get_id() << ", n: " << result.node_inst->get_instance_id() << ", t:" << typeid(result.node_inst).name(), );
    LogRAssertS(note->get_inputs().size() == 1, "Invalid " << note->get_object()->node_class << " node definition. w:" << wf->get_id() << ", n: " << note->get_instance_id(), );

    const auto pin_hash = Pin::make_hash(note->get_object(), "", -1);
    auto pin = note->get_object()->find_pin(pin_hash, true); // do not use pNodesManager
    if (nullptr == pin)
    {
        result.unrecognized_node_definition = true;
        return;
    }
    pin->finalize();

    const auto pin_instance = wf->get_pin_by_hash(note->get_instance_id(), pin_hash);    
    if (nullptr == pin_instance)
    {
        result.unrecognized_node_definition = true;
        return;
    }

    if (j.contains("widgets_values") && j["widgets_values"].is_array() && j["widgets_values"].size() == 1 && j["widgets_values"][0].is_string())
    {
        std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedNote>();
        cast::strict<CNI_DeserializedNote>(data.get())->widget_value = j["widgets_values"][0].get<std::string>();
        note->cni_save_deserialized_info(std::move(data));
    }
}

void WfDeserializerFromJson::deserialize_cni_node_note(const o_json& j, ParseResult& result)
{
    deserialize_cni_node_note_impl<CNI_Note>(m_wf, j, result);
}

void WfDeserializerFromJson::deserialize_cni_node_markdownnote(const o_json& j, ParseResult& result)
{
    deserialize_cni_node_note_impl<CNI_MarkdownNote>(m_wf, j, result);
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    FastModeSwitcherRgthree                      ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void WfDeserializerFromJson::deserialize_cni_fast_switcher_rgthree(const o_json& j, ParseResult& result)
{
    auto switcher = cast::dynamic<CNI_FastModeSwitcherRgthree>(result.node_inst);
    LogRAssert(switcher, "Node is not CNI. w:" << m_wf->get_id() << ", n: " << result.node_inst->get_instance_id() << ", t:" << typeid(result.node_inst).name(), );

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedFastActionRgthree>();
    auto switcher_data = cast::strict<CNI_DeserializedFastActionRgthree>(data.get());

    int32_t index = 0;
    if (j.contains("inputs") && j["inputs"].is_array())
    {
        for (const auto& jinput : j["inputs"])
        {
            if (jinput.is_object() && jinput.contains("link") && !jinput["link"].is_null() && jinput["link"].is_number_integer())
            {
                const auto id = m_wf->get_new_child_id();
                switcher_data->in_ids.push_back(std::make_tuple(id, 0, 0));
                m_node_in_pins[switcher->get_instance_id()][index++] = id;
            }
        }
    }

    switcher_data->out_id = m_wf->get_new_child_id();
    m_node_out_pins[switcher->get_instance_id()][0] = switcher_data->out_id;

    switcher->cni_save_deserialized_info(std::move(data));
    switcher->cni_init();
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    FastModeGroupSwitcherRgthree                 ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void WfDeserializerFromJson::deserialize_cni_fast_group_switcher_rgthree(const o_json& /*j*/, ParseResult& result)
{
    auto switcher = cast::dynamic<CNI_FastGroupModeSwitcherRgthree>(result.node_inst);
    LogRAssert(switcher, "Node is not CNI. w:" << m_wf->get_id() << ", n: " << result.node_inst->get_instance_id() << ", t:" << typeid(result.node_inst).name(), );

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedFastActionRgthree>();
    auto switcher_data = cast::strict<CNI_DeserializedFastActionRgthree>(data.get());

    switcher_data->out_id = m_wf->get_new_child_id();
    m_node_out_pins[switcher->get_instance_id()][0] = switcher_data->out_id;

    switcher->cni_save_deserialized_info(std::move(data));
    switcher->cni_init();
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    FastActionRgthree                            ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void WfDeserializerFromJson::deserialize_cni_fast_action_rgthree(const o_json& j, ParseResult& result)
{
    auto switcher = cast::dynamic<CNI_FastActionRgthree>(result.node_inst);
    LogRAssert(switcher, "Node is not CNI. w:" << m_wf->get_id() << ", n: " << result.node_inst->get_instance_id() << ", t:" << typeid(result.node_inst).name(), );

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedFastActionRgthree>();
    auto switcher_data = cast::strict<CNI_DeserializedFastActionRgthree>(data.get());

    int32_t index = 0;
    if (j.contains("inputs") && j["inputs"].is_array())
    {
        for (const auto& jinput : j["inputs"])
        {
            if (jinput.is_object() && jinput.contains("link") && !jinput["link"].is_null() && jinput["link"].is_number_integer())
            {
                const auto id = m_wf->get_new_child_id();
                switcher_data->in_ids.push_back(std::make_tuple(id, 0, 0));
                m_node_in_pins[switcher->get_instance_id()][index++] = id;
            }
        }
    }
    if (j.contains("widgets_values") && j["widgets_values"].is_array())
    {
        index = 0;
        for (const auto& jwidget : j["widgets_values"])
        {
            if (jwidget.is_string())
            {
                const auto value = jwidget.get<std::string>();
                if (value.empty())
                    continue;

                if (index >= switcher_data->in_ids.size())
                    break;

                const auto mode = CNI_FastActionRgthree::mode_from_string(value);
                std::get<2>(switcher_data->in_ids[index++]) = mode;
            }
        }
    }

    switcher->cni_save_deserialized_info(std::move(data));
    switcher->cni_init();
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    BookmarkRgthree                              ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void WfDeserializerFromJson::deserialize_cni_bookmark_rgthree(const o_json& /*j*/, ParseResult& /*result*/)
{
    // TODO: CNI_BookmarkRgthree
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    CNI_SwitchImpact                                       //////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void WfDeserializerFromJson::deserialize_cni_node_impact_switch(const o_json& j, ParseResult& result)
{
    auto switcher = cast::dynamic<CNI_SwitchImpact>(result.node_inst);
    LogRAssert(switcher, "Node is not CNI. w:" << m_wf->get_id() << ", n: " << result.node_inst->get_instance_id() << ", t:" << typeid(result.node_inst).name(), );

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedSwitchImpact>();
    auto switcher_data = cast::strict<CNI_DeserializedSwitchImpact>(data.get());

    int32_t index = 0;
    if (j.contains("inputs") && j["inputs"].is_array())
    {
        for (const auto& jinput : j["inputs"])
        {
            if (jinput.is_object() && jinput.contains("link") && !jinput["link"].is_null() && jinput["link"].is_number_integer())
            {
                const auto id = m_wf->get_new_child_id();
                switcher_data->in_dynamic_ids.push_back(std::make_pair(id, 0));
                m_node_in_pins[switcher->get_instance_id()][index++] = id;
            }
        }
    }
    index = 0;
    if (j.contains("outputs") && j["outputs"].is_array())
    {
        for (const auto& joutput : j["outputs"])
        {
            if (joutput.is_object() && joutput.contains("link") && !joutput["link"].is_null() && !joutput["link"].empty())
            {
                const auto id = m_wf->get_new_child_id();
                switcher_data->out_ids.push_back(std::make_pair(id, 0));
                m_node_out_pins[switcher->get_instance_id()][index++] = id;
            }
        }
    }

    switcher->cni_save_deserialized_info(std::move(data));
    switcher->cni_init();
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    CNI_ReversedSwitchImpact                               //////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void WfDeserializerFromJson::deserialize_cni_node_impact_insversed_switch(const o_json& j, ParseResult& result)
{
    auto switcher = cast::dynamic<CNI_SwitchInversedImpact>(result.node_inst);
    LogRAssert(switcher, "Node is not CNI. w:" << m_wf->get_id() << ", n: " << result.node_inst->get_instance_id() << ", t:" << typeid(result.node_inst).name(), );

    std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedSwitchImpact>();
    auto switcher_data = cast::strict<CNI_DeserializedSwitchImpact>(data.get());

    int32_t index = 0;
    if (j.contains("inputs") && j["inputs"].is_array())
    {
        for (const auto& jinput : j["inputs"])
        {
            if (jinput.is_object() && jinput.contains("link") && !jinput["link"].is_null() && jinput["link"].is_number_integer())
            {
                const auto id = m_wf->get_new_child_id();
                switcher_data->in_ids.push_back(std::make_pair(id, 0));
                m_node_in_pins[switcher->get_instance_id()][index++] = id;
            }
        }
    }
    index = 0;
    if (j.contains("outputs") && j["outputs"].is_array())
    {
        for (const auto& joutput : j["outputs"])
        {
            if (joutput.is_object() && joutput.contains("link") && !joutput["link"].is_null() && !joutput["link"].empty())
            {
                const auto id = m_wf->get_new_child_id();
                switcher_data->out_dynamic_ids.push_back(std::make_pair(id, 0));
                m_node_out_pins[switcher->get_instance_id()][index++] = id;
            }
        }
    }

    switcher->cni_save_deserialized_info(std::move(data));
    switcher->cni_init();
}

} // namespace data
