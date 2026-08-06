#include "core/workflow/serializer/wf_deserializer_from_json_api.h"
#include "core/workflow/workflow.h"
#include "core/workflow/workflow_action.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/managers/nodes_manager.h"

namespace data
{

WfDeserializerFromJsonApi::WfDeserializerFromJsonApi(Workflow* wf, const o_json& j)
    : m_wf(wf)
    , m_json(j)
{}

bool WfDeserializerFromJsonApi::deserialize()
{
    LogRAssert(m_wf, "Zero ptr", false);
    try
    {
        if (!m_json.is_object())
            return false;

        std::list<ApiNode> nodes;
        int32_t counter = 0;

        for (auto& [jnode_id, jnode_data] : m_json.items())
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
                            input.raw_input = jinput_value.get<int64_t>();
                        else if (jinput_value.is_number_float())
                            input.raw_input = jinput_value.get<double>();
                        else if (jinput_value.is_boolean())
                            input.raw_input = jinput_value.get<bool>();
                        else
                        {
                            input.raw_input = std::monostate();
                            LogWarn("API node " << jnode_id << "/" << api_node.class_type << " contains unknown data value type")
                        }
                    }
                    api_node.inputs.push_back(std::move(input));
                }
            }

            nodes.push_back(std::move(api_node));
        }

        for (auto& api_node : nodes)
        {
            create_api_node(api_node, false);
            m_node_pos.Min.x += 20;
            m_node_pos.Min.y += 20;
        }

        link_api_nodes(nodes);
        arrange_nodes();
        return true;
    }
    catch (const std::exception& ex)
    {
        LogExcep("Wf json API parse ex:" << ex.what());
    }
    return false;
}

NodeInstance* WfDeserializerFromJsonApi::create_api_node(WfDeserializerFromJsonApi::ApiNode& api_node, bool dummy)
{
    const auto node_hash = base::HashID::make_hash(api_node.class_type);
    auto node = g::pNodesManager->find_node(node_hash);
    if (node != nullptr && node->is_dummy)
    {
        LogRAssert(!node->is_dummy, "Dummy node " << api_node.class_type << " in base map", nullptr);
    }
    if (nullptr == node || dummy)
    {
        node = g::pNodesManager->make_dummy_node(api_node.class_type);
    }
    auto node_instance = m_wf->add_node(node, m_node_pos);
    LogRAssert(node_instance->get_object()->get_hash() == node_hash, "Node ids don't match. w:" << m_wf->get_id() << ", n:" << node_instance->get_instance_id() << ", nh1:" << node_instance->get_object()->get_hash() << ", nh2: " << node_hash, nullptr);

    if (!api_node.title.empty())
        node_instance->set_name(api_node.title);

    for (auto& input : api_node.inputs)
    {
        const auto pin_hash = Pin::make_hash(node_instance->get_object(), input.pin_name, -1);
        //auto pin = node_instance->get_object()->find_pin(pin_hash, true);
        auto pin = g::pNodesManager->find_pin(pin_hash);

        if (node->is_dummy || nullptr == pin || pin->def_state.name != input.pin_name)
        {
            if (!node_instance->get_object()->is_dummy)
            {
                m_wf->remove_node(node_instance);
                return create_api_node(api_node, true);
            }

            pin = g::pNodesManager->make_dummy_pin(node_instance->get_object(), "*", input.pin_name, -1);
        }
        else
        {
            pin->def_state.name = input.pin_name;
            pin->finalize();
        }

        LogRAssert(pin->get_hash() == pin_hash, "Pin ids don't match. w:" << m_wf->get_id() << ", n:" << node_instance->get_instance_id() << ", ph1:" << pin->get_hash() << ", ph2: " << pin_hash, nullptr);

        PinInstance* pin_instance = nullptr;
        if (node->is_dummy)
        {
            pin_instance = m_wf->add_pin(node_instance, pin, m_wf->get_new_child_id());
            pin->def_state.copy_to(pin_instance->state());
            node_instance->add_input(pin_instance);
        }
        else
        {
            pin_instance = m_wf->get_pin_by_hash(node_instance->get_instance_id(), pin_hash);
            LogRAssert(pin_instance, "Pin not found in wf. w:" << m_wf->get_id() << ", n:" << node_instance->get_instance_id() << ", ph:" << pin_hash, nullptr);
        }

        if (!input.is_linked)
        {
            std::visit([pin_instance](const auto& v)
            {
                using T = std::decay_t<decltype(v)>;

                if constexpr (std::is_same_v<T, int64_t>)
                {
                    if (PinType::INT == pin_instance->get_object()->def_state.type ||
                        PinType::COLOR == pin_instance->get_object()->def_state.type)
                    {
                        pin_instance->state().value = int64_t(v);
                    }
                    else if (PinType::FLOAT == pin_instance->get_object()->def_state.type)
                    {
                        pin_instance->state().value = double(v);
                    }
                    else if (PinType::NUMBER == pin_instance->get_object()->def_state.type)
                    {
                        pin_instance->state().value = number(number_t(v));
                    }
                    else if (PinType::BOOLEAN == pin_instance->get_object()->def_state.type)
                    {
                        pin_instance->state().value = bool(v != 0);
                    }
                }
                if constexpr (std::is_same_v<T, double>)
                {
                    if (PinType::FLOAT == pin_instance->get_object()->def_state.type)
                    {
                        pin_instance->state().value = double(v);
                    }
                    else if (PinType::NUMBER == pin_instance->get_object()->def_state.type)
                    {
                        pin_instance->state().value = number(v);
                    }
                }
                else if constexpr (std::is_same_v<T, bool>)
                {
                    if (PinType::BOOLEAN == pin_instance->get_object()->def_state.type)
                    {
                        pin_instance->state().value = v;
                    }
                }
                else if constexpr (std::is_same_v<T, std::string>)
                {
                    if (PinType::STRING == pin_instance->get_object()->def_state.type)
                    {
                        pin_instance->state().value = v;
                    }
                    else if (PinType::COMBO == pin_instance->get_object()->def_state.type)
                    {
                        pin_instance->state().value = int64_t(0);
                        pin_instance->get_object()->def_state.value = int64_t(0);

                        if (pin_instance->get_object()->combo_items.empty() && !v.empty())
                        {
                            auto pin = pin_instance->get_object();
                            pin->combo_items.push_back(v);
                            pin->combo_items_str.push_back(v);
                            pin->combo_items_ptr.push_back(pin->combo_items_str.begin()->c_str());
                            pin->min = int64_t(0);
                            pin->max = cast::to_i64(pin->combo_items.size() - 1);
                            pin->def_state.value = int64_t(0);
                            pin_instance->state().value = pin->def_state.value;
                        }
                        else
                        {
                            int64_t combo_idx = 0;
                            for (auto combo_item : pin_instance->get_object()->combo_items_ptr)
                            {
                                if (v == combo_item)
                                {
                                    pin_instance->state().value = combo_idx;
                                    break;
                                }
                                combo_idx++;
                            }
                        }
                    }
                }
            }, input.raw_input.value());
        }

        input.pin_iid = pin_instance->get_instance_id();
    }

    api_node.node_iid = node_instance->get_instance_id();
    return node_instance;
}

void WfDeserializerFromJsonApi::link_api_nodes(const std::list<ApiNode>& api_nodes)
{
    const auto find_source_pin = [this, &api_nodes](const std::string& src_node, int32_t src_pin_idx) -> PinInstance*
    {
        for (auto& api_node : api_nodes)
        {
            if (api_node.node_id == src_node)
            {
                const auto* node = m_wf->get_node_by_instance(api_node.node_iid);
                LogRAssert(node != nullptr, "Can't find deserialized API node " << api_node.node_iid, nullptr);
                if (node->get_object()->is_dummy)
                    return nullptr;

                auto& outputs = node->get_outputs();
                LogRAssert(src_pin_idx < outputs.size(), "Source pin index " << src_pin_idx << " in node " << node->get_object()->name << " greater than max inputs", nullptr);
                return outputs[src_pin_idx];
            }
        }

        LogRErrorS("Can't find pair of node/pin -> " << src_node << "/" << src_pin_idx, nullptr);
    };

    const auto find_dest_pin = [](NodeInstance* node, const std::string& pin_name) -> PinInstance*
    {
        for (auto pin : node->get_inputs())
        {
            if (pin->get_object()->def_state.name == pin_name)
                return pin;
        }

        LogAssertS(node->get_object()->is_dummy, "Can't find pair of node/pin -> " << node->get_object()->name << "/" << pin_name);
        return nullptr;
    };

    for (auto& api_node : api_nodes)
    {
        auto node = m_wf->get_node_by_instance(api_node.node_iid);
        LogCAssert(node, "Can't find deserialized API node. w:" << m_wf->get_id() << ", n:" << api_node.node_iid);

        for (auto& api_pin : api_node.inputs)
        {
            if (!api_pin.is_linked)
                continue;

            const auto* pin = m_wf->get_pin_by_instance(api_pin.pin_iid);
            LogCAssert(pin, "Can't find deserialized API pin. w:" << m_wf->get_id() << ", n:" << api_node.node_iid << ", p:" << api_pin.pin_iid);

            auto src_pin = find_source_pin(api_pin.linked_input.first, api_pin.linked_input.second);
            auto dest_pin = find_dest_pin(node, api_pin.pin_name);
            if (src_pin != nullptr && dest_pin != nullptr && nullptr == Link::link(src_pin, dest_pin, 0))
                LogError("Can't link pins. w:" << m_wf->get_id() << ", n:" << api_node.node_iid << ", p1:" << src_pin->get_instance_id() << ", p2:" << dest_pin->get_instance_id());
        }
    }
}

void WfDeserializerFromJsonApi::arrange_nodes()
{
    const auto& wf_nodes = m_wf->get_nodes();
    if (wf_nodes.empty())
        return;
    const auto& wf_links = m_wf->get_links();

    struct NodeWithParents
    {
        NodeInstance* node = nullptr;
        std::vector<NodeInstance*> parents;
    };

    std::vector<NodeWithParents> nodes_with_parents;
    nodes_with_parents.reserve(wf_nodes.size());

    std::unordered_map<NodeInstance*, int32_t> depths;
    int32_t max_depth = 0;

    std::unordered_map<NodeInstance*, std::pair<float, float>> sizes;
    const auto pin_h = ImGui::GetTextLineHeightWithSpacing();

    std::unordered_map<void*, NodeInstance*> pin_to_parent_node; 
    for (const auto& link : wf_links)
    {
        pin_to_parent_node[link->get_input()] = m_wf->get_node_by_instance(link->get_output()->get_parent_node());
    }

    for (auto& node : wf_nodes)
    {
        auto& inputs = node->get_inputs();
        auto& outputs = node->get_outputs();

        bool has_ins = false, has_outs = false;
        float max_h_pin_in = 0.0f;
        float max_h_widget_in = 0.0f;
        float max_h_out = 0.0f;

        for (auto in : inputs)
        {
            if (in->state().links > 0)
                has_ins = true;

            if (in->get_object()->is_widget(in->state()))
            {
                if (PinType::STRING == in->state().type && in->get_object()->multiline)
                    max_h_widget_in += pin_h * 3;
                else
                    max_h_widget_in += pin_h;
            }
            else
                max_h_pin_in += pin_h;
        }
        for (auto out : outputs)
        {
            if (out->state().links > 0)
                has_outs = true;

            max_h_out += pin_h;
        }

        const float max_w = node->calc_node_width(false, false) + 40.0f;
        const float max_h = std::max(max_h_pin_in, max_h_out) + max_h_widget_in + 50.0f + 20.0f;
        sizes[node.get()] = { max_w, max_h };

        if (!has_ins && !has_outs)
        {
            depths[node.get()] = 0;
            continue;
        }

        if (!has_ins)
        {
            depths[node.get()] = 1;
            max_depth = std::max(max_depth, 1);
            continue;
        }

        NodeWithParents np;
        np.node = node.get();

        for (auto in : inputs)
        {
            auto it = pin_to_parent_node.find(in);
            if (it != pin_to_parent_node.end() && it->second != nullptr)
            {
                np.parents.push_back(it->second);
            }
        }
        nodes_with_parents.push_back(std::move(np));
    }

    bool changed = true;
    int32_t iteration_guard = 0;
    const int32_t MAX_ITERATIONS = 1000; 

    while (changed && iteration_guard++ < MAX_ITERATIONS) 
    {
        changed = false;

        for (const auto& node : nodes_with_parents) 
        {
            bool all_parents_resolved = true;
            int32_t max_parent_depth = -1;

            for (auto parent : node.parents) 
            {
                auto it = depths.find(parent);
                if (it == depths.end()) 
                {
                    all_parents_resolved = false;
                    break;
                }
                max_parent_depth = std::max(max_parent_depth, it->second);
            }

            if (all_parents_resolved) 
            {
                int32_t new_depth = max_parent_depth + 1;

                if (depths.find(node.node) == depths.end() || depths[node.node] != new_depth) 
                {
                    depths[node.node] = new_depth;
                    max_depth = std::max(max_depth, new_depth);
                    changed = true;
                }
            }
        }
    }

    if (iteration_guard >= MAX_ITERATIONS)
    {
        LogWarn("Graph contains cycles or is too deep. Layout calculation aborted early.");
    }

    std::vector<float> col_widths(max_depth + 1, 0.0f);
    for (const auto& [node, depth] : depths)
    {
        const auto& size = sizes[node];
        col_widths[depth] = std::max(col_widths[depth], size.first);
    }

    std::vector<float> w_off(max_depth + 1, 0.0f); 
    w_off[0] = 20.0f;

    for (size_t i = 1; i < col_widths.size(); i++)
    {
        w_off[i] = w_off[i - 1] + col_widths[i - 1] + 100.0f; 
    }

    std::vector<float> h_off(max_depth + 1, 0.0f);
    for (const auto& [node, depth] : depths)
    {
        if (0.0f == h_off[depth])
            h_off[depth] = 20.0f;

        node->m_rc.Min = { w_off[depth], h_off[depth] };
        h_off[depth] += sizes[node].second + 50.0f;
    }
}

} // namespace data
