#include "core/workflow/serializer/wf_serializer_to_json_api.h"
#include "core/workflow/workflow.h"
#include "core/workflow/workflow_action.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/workflows_manager.h"

namespace data
{

WfSerializerToJsonApi::WfSerializerToJsonApi(const Workflow* wf) noexcept
    : m_wf(wf)
{}

o_json WfSerializerToJsonApi::serialize()
{
    LogRAssert(m_wf, "Zero ptr", o_json{});

    std::shared_ptr<Workflow> virt_wf = g::pWfsManager->create_copy_virtual(m_wf);
    virt_wf->prepare_for_api();
    m_wf = virt_wf.get();

    o_json j;
    for (auto& node : m_wf->get_nodes_all())
    {
        o_json jnode;
        jnode["class_type"] = node->get_object()->node_class;
        jnode["_meta"]["title"] = node->get_name();

        int32_t inputs_counter = 0;
        o_json inputs;

        for (auto pin : node->get_inputs())
        {
            if (pin->get_object()->cag_pin)
                continue;

            if (!pin->get_object()->is_widget(pin->state()) && 0 == pin->state().links)
                continue;

            bool has_links = (pin->state().links > 0);
            if (has_links)
            {
                const auto links = m_wf->get_links_by_pin(pin);
                if (links.size() != 1)
                {
                    LogError("Invalid number of links for input pin");
                }
                else
                {
                    const auto* output_pin = (*links.begin())->get_output();
                    const auto* output_node = m_wf->get_node_by_instance(output_pin->get_parent_node());
                    if (output_node->is_cni_node() && !output_node->cni_json_api_should_export())
                        has_links = false;
                }
            }

            if (!has_links)
            {
                o_json& input = inputs[pin->get_object()->def_state.name];

                std::visit([&input, pin](const auto& v)
                {
                    using T = std::decay_t<decltype(v)>;

                    if constexpr (std::is_same_v<T, double> || std::is_same_v<T, bool>)
                    {
                        input = v;
                    }
                    else if constexpr (std::is_same_v<T, int64_t>)
                    {
                        if (PinType::COMBO == pin->get_object()->def_state.type)
                        {
                            if (pin->get_object()->combo_items_ptr.size() <= cast::to_szt(v))
                                input = "";
                            else
                                input = pin->get_object()->combo_items_ptr[v];
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
                }, pin->state().value.value());

                inputs_counter++;
            }
            else
            {
                auto jarr = o_json::array();

                for (auto& link : m_wf->get_links_all())
                {
                    if (link->get_input() == pin)
                    {
                        const auto src_node_iid = link->get_output()->get_parent_node();
                        jarr.push_back(std::to_string(src_node_iid));

                        const auto* src_node = m_wf->get_node_by_instance(src_node_iid);
                        LogRAssert(src_node, "Node not found. w:" << m_wf->get_id() << ", n:" << src_node_iid, o_json{});

                        int32_t index = 0;
                        iid out_pin_id = 0;
                        for (auto out_pin : src_node->get_outputs())
                        {
                            if (out_pin == link->get_output())
                            {
                                out_pin_id = out_pin->get_instance_id();
                                break;
                            }
                            index++;
                        }

                        LogRAssert(out_pin_id != 0, "Output pin not found. w:" << m_wf->get_id() << ", n:" << src_node_iid << ", p:" << out_pin_id, o_json{});

                        jarr.push_back(index);
                        inputs_counter++;
                        break;
                    }
                }

                inputs[pin->get_object()->def_state.name] = std::move(jarr);
            }
        }

        if (inputs_counter > 0)
            jnode["inputs"] = std::move(inputs);
        else
            jnode["inputs"] = nlohmann::json::object();

        j[std::to_string(node->get_instance_id())] = std::move(jnode);
    }

    m_wf = nullptr;
    g::pWfsManager->remove_virtual_wf(virt_wf->get_id());

    return j;
}

} // namespace data
