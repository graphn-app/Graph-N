
#include "core/workflow/workflow_serializer_to_json.h"
#include "core/workflow/workflow.h"
#include "core/workflow/workflow_action.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/managers/nodes_manager.h"

namespace data
{

bool WorkflowSerializerToJson::deserialize_api(Workflow* wf, const o_json& j)
{
    try
    {
        if (!j.is_object())
            return false;

        std::list<ApiNode> nodes;
        int32_t counter = 0;

        for (auto& [jnode_id, jnode_data] : j.items())
        {
            if (!jnode_data.is_object())
                continue;

            ApiNode api_node;
            api_node.node_id = jnode_id;

            if (jnode_data.contains("class_type") && jnode_data["class_type"].is_string())
                api_node.class_type = jnode_data["class_type"].get<std::string>();
            else
            {
                api_node.class_type = "API_UNKNOWN_" + std::to_string(counter++);
                api_node.title = "Unknown";
            }

            if (jnode_data.contains("_meta") && jnode_data["_meta"].contains("title") && jnode_data["_meta"]["title"].is_string())
                api_node.title = jnode_data["_meta"]["title"];

            if (jnode_data.contains("inputs") && jnode_data["inputs"].is_object())
            {
                for (auto& [jinput_name, jinput_value] : jnode_data["inputs"].items())
                {
                    ApiNode::Input input;
                    input.pin_name = jinput_name;
                    if (jinput_value.is_array() && jinput_value.size() == 2 &&
                        (jinput_value[0].is_string() || jinput_value[0].is_number_integer()) && jinput_value[1].is_number_integer())  // linked
                    {
                        input.is_linked = true;
                        if (jinput_value[0].is_string())
                            input.linked_input.first = jinput_value[0].get<std::string>();
                        else if (jinput_value[0].is_number_integer())
                            input.linked_input.first = std::to_string(jinput_value[0].get<int32_t>());

                        input.linked_input.second = jinput_value[1].get<int32_t>();
                    }
                    else // raw value
                    {
                        input.is_linked = false;
                        if (jinput_value.is_string())
                            input.raw_input = jinput_value.get<std::string>();
                        else if (jinput_value.is_number_integer())
                        {
                            auto i64 = jinput_value.get<int64_t>();
                            auto u64 = jinput_value.get<uint64_t>();
                            if (i64 == int64_t(u64))
                                input.raw_input = i64;
                            else
                                input.raw_input = u64;
                        }
                        else if (jinput_value.is_number_float())
                            input.raw_input = jinput_value.get<double>();
                        else if (jinput_value.is_boolean())
                            input.raw_input = jinput_value.get<bool>();
                        else
                        {
                            input.raw_input = std::monostate();
                            LogWarnS("API node " << jnode_id << "/" << api_node.class_type << " contains unknown data value type")
                        }
                    }
                    api_node.inputs.push_back(std::move(input));
                }
            }

            nodes.push_back(std::move(api_node));
        }

        for (auto& api_node : nodes)
        {
            create_api_node(wf, api_node, false);
        }

        link_api_nodes(wf, nodes);
        return true;
    }
    catch (std::exception& ex)
    {
        LogExcepS(ExcepCategory::API, ex.what());
    }
    return false;
}

NodeInstance* WorkflowSerializerToJson::create_api_node(Workflow* wf, WorkflowSerializerToJson::ApiNode& api_node, bool dummy)
{
    auto node_hash = base::HashID::make_hash(api_node.class_type);
    auto node = g::pNodesManager->find_node(node_hash);
    if (node != nullptr && node->is_dummy)
    {
        LogAndThrowInternalErrorS(ExcepCategory::Logic, "Dummy node " << api_node.class_type << " in base map");
    }
    if (nullptr == node || dummy)
    {
        node = g::pNodesManager->make_dummy_node(api_node.class_type);
    }
    auto node_instance = wf->add_node(node, {});
    if (node_instance->m_object->get_id() != node_hash)
    {
        LogAndThrowInternalErrorS(ExcepCategory::Logic, "Node ids don't match " << node_instance->m_object->get_id() << " != " << node_hash);
    }

    if (!api_node.title.empty())
        node_instance->set_name(api_node.title);

    for (auto& input : api_node.inputs)
    {
        const auto pin_hash = Pin::make_hash(node_instance->m_object, input.pin_name, -1);
        auto pin = node_instance->m_object->find_pin(pin_hash, true);

        if (node->is_dummy || nullptr == pin || pin->def_state.name != input.pin_name)
        {
            if (!node_instance->m_object->is_dummy)
            {
                wf->remove_node(node_instance);
                return create_api_node(wf, api_node, true);
            }

            pin = g::pNodesManager->make_dummy_pin(node_instance->m_object, "*", input.pin_name, -1);
        }
        else
        {
            pin->def_state.name = input.pin_name;
            pin->finalize();
        }

        if (pin->get_id() != pin_hash)
        {
            LogAndThrowInternalErrorS(ExcepCategory::Logic, "Pin ids don't match " << pin->get_id() << " != " << pin_hash);
        }

        PinInstance* pin_instance = nullptr;
        if (node->is_dummy)
        {
            pin_instance = wf->add_pin(node_instance, pin, wf->get_new_child_id());
            pin_instance->m_obj_class = "*";
            pin_instance->m_state = pin->def_state;
            node_instance->m_input.push_back(pin_instance);
        }
        else
        {
            pin_instance = wf->get_pin_by_hash(node_instance->get_instance_id(), pin_hash);
            if (nullptr == pin_instance)
            {
                LogAndThrowInternalErrorS(ExcepCategory::Logic, "PinInstance " << pin_hash << " not found in wf");
            }
        }

        if (!input.is_linked)
        {
            std::visit([pin_instance](const auto& v)
            {
                using T = std::decay_t<decltype(v)>;

                if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>)
                {
                    if (PinType::INT == pin_instance->m_object->def_state.type ||
                        PinType::NUMBER == pin_instance->m_object->def_state.type ||
                        PinType::COLOR == pin_instance->m_object->def_state.type)
                    {
                        if (pin_instance->m_object->is_unsigned || std::is_same_v<T, uint64_t>)
                        {
                            pin_instance->m_state.value = uint64_t(v);
                            pin_instance->m_object->is_unsigned = true;
                        }
                        else
                            pin_instance->m_state.value = int64_t(v);
                    }
                    else if (PinType::FLOAT == pin_instance->m_object->def_state.type)
                    {
                        pin_instance->m_state.value = double(v);
                    }
                    else if (PinType::BOOLEAN == pin_instance->m_object->def_state.type)
                    {
                        pin_instance->m_state.value = bool(v != 0);
                    }
                }
                if constexpr (std::is_same_v<T, double>)
                {
                    if (PinType::FLOAT == pin_instance->m_object->def_state.type)
                    {
                        pin_instance->m_state.value = v;
                    }
                }
                else if constexpr (std::is_same_v<T, bool>)
                {
                    if (PinType::BOOLEAN == pin_instance->m_object->def_state.type)
                    {
                        pin_instance->m_state.value = v;
                    }
                }
                else if constexpr (std::is_same_v<T, std::string>)
                {
                    if (PinType::STRING == pin_instance->m_object->def_state.type)
                    {
                        pin_instance->m_state.value = v;
                    }
                    else if (PinType::COMBO == pin_instance->m_object->def_state.type)
                    {
                        pin_instance->m_state.value = int64_t(0);
                        pin_instance->m_object->def_state.value = int64_t(0);

                        if (pin_instance->m_object->list.empty() && !v.empty())
                        {
                            auto pin = pin_instance->m_object;
                            pin->list.push_back(v);
                            pin->combo_items_str.push_back(v);
                            pin->combo_items_ptr.push_back(pin->combo_items_str.begin()->c_str());
                            pin->def_state.value = int64_t(0);
                            pin_instance->m_state.value = pin->def_state.value;
                        }
                        else
                        {
                            int64_t combo_idx = 0;
                            for (auto combo_item : pin_instance->m_object->combo_items_ptr)
                            {
                                if (v == combo_item)
                                {
                                    pin_instance->m_state.value = combo_idx;
                                    break;
                                }
                                combo_idx++;
                            }
                        }
                    }
                }
            }, input.raw_input);
        }

        input.pin_iid = pin_instance->get_instance_id();
    }

    api_node.node_iid = node_instance->get_instance_id();
    return node_instance;
}

void WorkflowSerializerToJson::link_api_nodes(Workflow* wf, const std::list<ApiNode>& api_nodes)
{
    const auto find_source_pin = [&api_nodes, wf](const std::string& src_node, int32_t src_pin_idx) -> PinInstance*
    {
        for (auto& api_node : api_nodes)
        {
            if (api_node.node_id == src_node)
            {
                auto node = wf->get_node_by_instance(api_node.node_iid);
                if (nullptr == node)
                {
                    LogAndThrowInternalErrorS(ExcepCategory::Logic, "Can't find deserialized API node " << api_node.node_iid);
                }
                if (node->m_object->is_dummy)
                    return nullptr;

                auto& outputs = node->m_output;
                if (src_pin_idx >= outputs.size())
                {
                    LogAndThrowInternalErrorS(ExcepCategory::Logic, "Source pin index " << src_pin_idx << " in node " << node->m_object->name << " greater than max inputs");
                }
                return outputs[src_pin_idx];
            }
        }

        LogAndThrowInternalErrorS(ExcepCategory::Logic, "Can't find pair of node/pin -> " << src_node << "/" << src_pin_idx);
    };

    const auto find_dest_pin = [](NodeInstance* node, const std::string& pin_name) -> PinInstance*
    {
        auto& inputs = node->m_input;
        for (auto pin : inputs)
        {
            if (pin->m_object->def_state.name == pin_name)
                return pin;
        }

        if (!node->m_object->is_dummy)
            LogAndThrowInternalErrorS(ExcepCategory::Logic, "Can't find pair of node/pin -> " << node->m_object->name << "/" << pin_name);
        return nullptr;
    };

    for (auto& api_node : api_nodes)
    {
        auto node = wf->get_node_by_instance(api_node.node_iid);
        if (nullptr == node)
        {
            LogAndThrowInternalErrorS(ExcepCategory::Logic, "Can't find deserialized API node " << api_node.node_iid);
        }

        for (auto& api_pin : api_node.inputs)
        {
            if (!api_pin.is_linked)
                continue;

            auto pin = wf->get_pin_by_instance(api_pin.pin_iid);
            if (nullptr == pin)
            {
                LogAndThrowInternalErrorS(ExcepCategory::Logic, "Can't find deserialized API pin " << api_pin.pin_iid);
            }

            auto src_pin = find_source_pin(api_pin.linked_input.first, api_pin.linked_input.second);
            auto dest_pin = find_dest_pin(node, api_pin.pin_name);
            if (src_pin != nullptr && dest_pin != nullptr && nullptr == Link::link(src_pin, dest_pin))
                LogWarnS("Can't link pins");
        }
    }
}

} // namespace data
