#include "core/workflow/serializer/wf_deserializer_from_json.h"
#include "core/workflow/custom_nodes/all_impl.h"
#include "core/workflow/pin_instance.h"
#include "core/managers/nodes_manager.h"

namespace data
{

bool WfDeserializerFromJson::check_cni_node_definition(const o_json& j, hid node_class, bool& is_valid, bool& is_cni) const
{
    is_valid = true;
    is_cni = true;

    switch (node_class)
    {
    case js::PRIMITIVE_NODE:
        return check_cni_node_definition_primitive(j, is_valid);
    case js::REROUTE:
        return check_cni_node_definition_reroute(j, is_valid);
    case js::NOTE:
        return check_cni_node_definition_note(j, is_valid);
    case js::MARKDOWN_NOTE:
        return check_cni_node_definition_markdownnote(j, is_valid);
    }

    is_cni = false;
    return true;
}

NodeInstance* WfDeserializerFromJson::deserialize_cni_node(NodeInstance* node, const o_json& j)
{
    if (node->obj_flags().has(NodeObjFlags::CustomJsNode))
        return deserialize_js_node(node, j);

    switch (node->get_object()->get_hash())
    {
    case js::PRIMITIVE_NODE:
        return deserialize_cni_node_primitive(node, j);
    case js::REROUTE:
        return deserialize_cni_node_reroute(node, j);
    case js::NOTE:
        return deserialize_cni_node_note(node, j);
    case js::MARKDOWN_NOTE:
        return deserialize_cni_node_markdownnote(node, j);
    }

    LogRError("Unhandled cni node", nullptr);
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    Primitive                      //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

bool WfDeserializerFromJson::check_cni_node_definition_primitive(const o_json& j, bool& /*is_valid*/) const
{
    if (j.contains("inputs") && !j["inputs"].empty())
        return false;
    if (!j.contains("outputs") || !j["outputs"].is_array() || j["outputs"].size() != 1)
        return false;
    return true;
}

NodeInstance* WfDeserializerFromJson::deserialize_cni_node_primitive(NodeInstance* node, const o_json& j)
{
    const auto pin_hash = Pin::make_hash(node->get_object(), "*", 0);
    auto pin = node->get_object()->find_pin(pin_hash, false); // do not use pNodesManager
    if (nullptr == pin)
        return nullptr;
    pin->finalize();

    const auto* pin_instance = m_wf->get_pin_by_hash(node->get_instance_id(), pin_hash);    
    if (nullptr == pin_instance)
        return nullptr;    

    m_node_out_pins[node->get_instance_id()][0] = pin_instance->get_instance_id();

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
                cast::strict<CNI_DeserializedPrimitive>(data.get())->cag_value = jvalue1.get<std::string>();
            }
        }

        node->cni_save_deserialized_info(std::move(data));
    }

    return node;
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    Reroute                        //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

bool WfDeserializerFromJson::check_cni_node_definition_reroute(const o_json& j, bool& /*is_valid*/) const
{
    if (j.contains("inputs") && !j["inputs"].empty() && j["inputs"].size() != 1)
        return false;
    if (j.contains("outputs") && j["outputs"].size() != 1)
        return false;
    return true;
}

NodeInstance* WfDeserializerFromJson::deserialize_cni_node_reroute(NodeInstance* node, const o_json& j)
{
    auto reroute = cast::dynamic<CNI_Reroute>(node);
    LogRAssert(reroute, "Node is not CNI. w:" << m_wf->get_id() << ", n: " << node->get_instance_id() << ", t:" << typeid(node).name(), node);

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
        return node;

    if (0 == pair.in_id) pair.in_id = m_wf->get_new_child_id();
    if (0 == pair.out_id) pair.out_id = m_wf->get_new_child_id();
    
    cast::strict<CNI_DeserializedReroute>(data.get())->poses = { node->m_rc, node->m_rc };
    cast::strict<CNI_DeserializedReroute>(data.get())->pairs.push_back(pair);
    
    reroute->cni_save_deserialized_info(std::move(data));
    reroute->cni_init();

    m_node_in_pins[node->get_instance_id()][0] = pair.in_id;
    m_node_out_pins[node->get_instance_id()][0] = pair.out_id;

    return node;
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    Note, MarkdownNote             //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

bool WfDeserializerFromJson::check_cni_node_definition_note(const o_json& j, bool& /*is_valid*/) const
{
    if (j.contains("inputs") && !j["inputs"].empty())
        return false;
    if (j.contains("outputs") && !j["outputs"].empty())
        return false;
    return true;
}

bool WfDeserializerFromJson::check_cni_node_definition_markdownnote(const o_json& j, bool& is_valid) const
{
    return check_cni_node_definition_note(j, is_valid);
}

template<typename N>
NodeInstance* deserialize_cni_node_note_impl(Workflow* wf, NodeInstance* node, const o_json& j)
{
    auto note = cast::dynamic<N>(node);
    LogRAssertS(note, "Node is not CNI. w:" << wf->get_id() << ", n: " << node->get_instance_id() << ", t:" << typeid(node).name(), node);
    LogRAssertS(note->get_inputs().size() == 1, "Invalid " << note->get_object()->node_class << " node definition. w:" << wf->get_id() << ", n: " << node->get_instance_id() << ", t:" << typeid(node).name(), node);

    const auto pin_hash = Pin::make_hash(node->get_object(), "", -1);
    auto pin = node->get_object()->find_pin(pin_hash, true); // do not use pNodesManager
    if (nullptr == pin)
        return nullptr;
    pin->finalize();

    const auto pin_instance = wf->get_pin_by_hash(node->get_instance_id(), pin_hash);    
    if (nullptr == pin_instance)
        return nullptr;

    if (j.contains("widgets_values") && j["widgets_values"].is_array() && j["widgets_values"].size() == 1 && j["widgets_values"][0].is_string())
    {
        std::unique_ptr<CNI_DeserializedData> data = std::make_unique<CNI_DeserializedNote>();
        cast::strict<CNI_DeserializedNote>(data.get())->widget_value = j["widgets_values"][0].get<std::string>();
        node->cni_save_deserialized_info(std::move(data));
    }

    return node;
}

NodeInstance* WfDeserializerFromJson::deserialize_cni_node_note(NodeInstance* node, const o_json& j)
{
    return deserialize_cni_node_note_impl<CNI_Note>(m_wf, node, j);
}

NodeInstance* WfDeserializerFromJson::deserialize_cni_node_markdownnote(NodeInstance* node, const o_json& j)
{
    return deserialize_cni_node_note_impl<CNI_MarkdownNote>(m_wf, node, j);
}

} // namespace data
