#include "core/workflow/serializer/wf_deserializer_from_json.h"
#include "core/workflow/custom_nodes/all_impl.h"
#include "core/workflow/pin_instance.h"

namespace data
{

NodeInstance* WfDeserializerFromJson::deserialize_cni_node(NodeInstance* node, const o_json& j)
{
    if (CNI_Primitive::cni_is_same(node))
        return deserialize_cni_node_primitive(node, j);
    if (CNI_Reroute::cni_is_same(node))
        return deserialize_cni_node_reroute(node, j);

    LogRError("Unhandled cni node", nullptr);
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    Primitive                      //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

NodeInstance* WfDeserializerFromJson::deserialize_cni_node_primitive(NodeInstance* node, const o_json& j)
{
    if (!j.contains("outputs") || !j["outputs"].is_array() || j["outputs"].size() != 1)
        return nullptr;

    const auto pin_hash = Pin::make_hash(node->get_object(), "*", 0);
    data::Pin* pin = node->get_object()->find_pin(pin_hash, false);
    if (nullptr == pin)
    {
        m_wf->remove_node(node);
        return nullptr;
    }

    pin->finalize();

    const auto* pin_instance = m_wf->get_pin_by_hash(node->get_instance_id(), pin_hash);    
    if (nullptr == pin_instance)
    {        
        m_wf->remove_node(node);
        LogError("PinInstance " << pin_hash << " not found in m_wf");
        return nullptr;
    }

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
                ((CNI_DeserializedPrimitive*)data.get())->cag_value = jvalue1.get<std::string>();
            }
        }

        node->cni_save_deserialized_info(std::move(data));
    }

    return node;
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////    Reroute                        //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

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

    ((CNI_DeserializedReroute*)data.get())->pairs.push_back(pair);
    
    reroute->cni_save_deserialized_info(std::move(data));
    reroute->cni_init();

    m_node_in_pins[node->get_instance_id()][0] = pair.in_id;
    m_node_out_pins[node->get_instance_id()][0] = pair.out_id;

    return node;
}

} // namespace data
