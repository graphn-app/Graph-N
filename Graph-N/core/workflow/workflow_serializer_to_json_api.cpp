
#include "core/workflow/workflow_serializer_to_json.h"
#include "core/workflow/workflow.h"
#include "core/workflow/workflow_action.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/managers/nodes_manager.h"

namespace data
{

o_json WorkflowSerializerToJson::serialize_to_api(const Workflow* wf)
{
    const auto& nodes = wf->m_nodes;
    o_json j;

    for (auto& node : nodes)
    {
        if (node->m_group)
            continue;

        o_json jnode;
        jnode["class_type"] = node->m_object->node_class;
        jnode["_meta"]["title"] = node->get_name();

        int32_t inputs_counter = 0;
        o_json inputs;

        for (auto pin : node->m_input)
        {
            if (pin->m_object->cag_pin)
                continue;

            if (!pin->m_object->is_widget() && 0 == pin->m_state.links)
                continue;

            if (0 == pin->m_state.links)
            {
                o_json& input = inputs[pin->m_object->def_state.name];

                std::visit([&input, pin](const auto& v)
                {
                    using T = std::decay_t<decltype(v)>;

                    if constexpr (std::is_same_v<T, uint64_t> || std::is_same_v<T, double> || std::is_same_v<T, bool>)
                    {
                        input = v;
                    }
                    else if constexpr (std::is_same_v<T, int64_t>)
                    {
                        if (PinType::COMBO == pin->m_object->def_state.type)
                        {
                            if (pin->m_object->combo_items_ptr.size() <= uint64_t(v))
                                input = "";
                            else
                                input = pin->m_object->combo_items_ptr[v];
                        }
                        else
                        {
                            input = v;
                        }
                    }
                    else if constexpr (std::is_same_v<T, std::string>)
                    {
                        input = v;
                    }
                }, pin->m_state.value);

                inputs_counter++;
            }
            else
            {
                auto jarr = o_json::array();

                for (auto& link : wf->m_links)
                {
                    if (link->m_input == pin)
                    {
                        auto src_node_iid = link->m_output->get_parent_node();
                        jarr.push_back(std::to_string(src_node_iid));

                        auto src_node = wf->get_node_by_instance(src_node_iid);
                        if (nullptr == src_node)
                        {
                            LogInternalLogicS("Parent node " << src_node_iid << " not found");
                        }

                        int32_t index = 0;
                        iid out_pin_id = 0;
                        for (auto out_pin : src_node->m_output)
                        {
                            if (out_pin == link->m_output)
                            {
                                out_pin_id = out_pin->get_instance_id();
                                break;
                            }
                            index++;
                        }

                        if (0 == out_pin_id)
                        {
                            LogInternalLogicS("Output pin not found");
                        }

                        jarr.push_back(index);
                        inputs_counter++;
                        break;
                    }
                }

                inputs[pin->m_object->def_state.name] = std::move(jarr);
            }
        }

        if (inputs_counter > 0)
            jnode["inputs"] = std::move(inputs);
        else
            jnode["inputs"] = nlohmann::json::object();

        j[std::to_string(node->get_instance_id())] = std::move(jnode);
    }

    return j;
}

} // namespace data
