
#include "core/workflow/workflow_serializer_to_json.h"
#include "core/workflow/workflow.h"
#include "core/workflow/workflow_action.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/managers/nodes_manager.h"

namespace data
{

static std::string color_to_string(const ImVec4& color)
{
    return fmt::format(FMT_STRING("#{:02X}{:02X}{:02X}"), cast::to_i32(color.x * 255.0f), cast::to_i32(color.y * 255.0f), cast::to_i32(color.z * 255.0f));
}

static std::string color_to_string(ImU32 color)
{
    return color_to_string(ImGui::ColorConvertU32ToFloat4(color));
}

std::string WorkflowSerializerToJson::serialize(const Workflow* wf)
{
    o_json j;
    j["id"] = wf->get_uuid();
    j["revision"] = 0;
    j["version"] = 0.4;
    j["last_node_id"] = serialize_nodes(wf, j);
    j["last_link_id"] = serialize_links(wf, j);

    j["extra"]["workflowRendererVersion"] = "LG";
    j["extra"]["ds"]["scale"] = 1.0f;
    j["extra"]["ds"]["offset"] = { 0.0f, 0.0f };

    serialize_groups(wf, j);

    return j.dump();
}

iid WorkflowSerializerToJson::serialize_nodes(const Workflow* wf, o_json& j)
{
    iid last_id = 0;
    int32_t order = 0;

    auto jarr = o_json::array();
    const auto& nodes = wf->m_nodes;

    for (auto& node : nodes)
    {
        if (node->m_group)
            continue;

        last_id = std::max(last_id, node->get_instance_id());
        o_json jn;
        jn["id"] = node->get_instance_id();
        jn["type"] = node->m_object->node_class;
        jn["pos"] = { node->m_rc.Min.x, node->m_rc.Min.y };
        jn["order"] = order++;
        if (node->get_name() != node->get_name_original())
            jn["title"] = node->get_name();

        if (!node->m_expanded)
            jn["flags"]["collapsed"] = true;
        if (node->m_pinned)
            jn["flags"]["pinned"] = true;

        jn["color"] = color_to_string(node->m_clr_head);
        jn["bgcolor"] = color_to_string(node->m_clr_node);

        switch (node->m_mode)
        {
        case NodeMode::OnEvent:
            jn["mode"] = 1;
            break;
        case NodeMode::Mute:
            jn["mode"] = 2;
            break;
        case NodeMode::OnTrigger:
            jn["mode"] = 3;
            break;
        case NodeMode::Bypass:
            jn["mode"] = 4;
            break;
        case NodeMode::Always:
        default:
            jn["mode"] = 0;
            break;
        }

        jn["properties"]["Node name for S&R"] = node->m_object->node_class;

        serialize_pins(wf, node.get(), jn);
        serialize_widgets(node.get(), jn);

        jarr.push_back(std::move(jn));
    }

    j["nodes"] = std::move(jarr);
    return last_id;
}

void WorkflowSerializerToJson::serialize_pins(const Workflow* wf, const NodeInstance* node, o_json& j)
{
    const auto serialize_pin = [wf](const PinInstance* pin)
    {
        o_json j;
        j["localized_name"] = pin->m_object->def_state.name;
        j["name"] = pin->m_object->def_state.name;
        j["type"] = pin->m_object->def_state.type_str;
        if (pin->m_object->def_state.name != pin->m_state.name)
            j["label"] = pin->m_state.name;
        if (pin->m_object->is_widget())
            j["widget"]["name"] = pin->m_object->def_state.name;

        if (pin->m_object->input)
        {
            if (0 == pin->m_state.links)
                j["link"] = nullptr;
            else
            {
                for (auto& link : wf->m_links)
                {
                    if (link->get_input()->get_instance_id() == pin->get_instance_id())
                    {
                        j["link"] = link->get_instance_id();
                        break;
                    }
                }
            }
        }
        else if (pin->m_object->output && 0 == pin->m_state.links)
        {
            if (0 == pin->m_state.links)
                j["links"] = nullptr;
            else
            {
                j["links"] = o_json::array();
                for (auto& link : wf->m_links)
                {
                    if (link->get_input()->get_instance_id() == pin->get_instance_id())
                    {
                        j["links"].push_back(link->get_instance_id());
                    }
                }
            }
        }

        return j;
    };

    auto jarr = o_json::array();
    for (auto& pin : node->m_input)
    {
        jarr.push_back(serialize_pin(pin));
    }
    j["inputs"] = std::move(jarr);

    jarr = o_json::array();
    for (auto& pin : node->m_output)
    {
        jarr.push_back(serialize_pin(pin));
    }
    j["outputs"] = std::move(jarr);
}

void WorkflowSerializerToJson::serialize_widgets(const NodeInstance* node, o_json& j)
{
    auto jarr = o_json::array();
    for (auto pin : node->m_input)
    {
        if (pin->m_object->is_widget())
        {
            std::visit([&jarr, pin](const auto& v)
            {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, double> ||
                              std::is_same_v<T, bool> || std::is_same_v<T, std::string>)
                {
                    jarr.push_back(v);
                }
                else if constexpr (std::is_same_v<T, int64_t>)
                {
                    if (PinType::COMBO == pin->m_state.type)
                    {
                        jarr.push_back(pin->m_object->combo_items_ptr[v]);
                    }
                    else
                        jarr.push_back(v);
                }
                else if constexpr (std::is_same_v<T, uint64_t>)
                {
                    if (PinType::COLOR == pin->m_state.type)
                    {
                        jarr.push_back(color_to_string(ImU32(v)));
                    }
                    else
                        jarr.push_back(v);
                }
            },pin->m_state.value);
        }
    }

    if (!jarr.empty())
        j["widgets_values"] = std::move(jarr);
}

iid WorkflowSerializerToJson::serialize_links(const Workflow* wf, o_json& j)
{
    iid last_id = 0;
    auto jarr = o_json::array();

    const auto get_pin_index = [wf](iid node_id, iid pin_id, bool in)
    {
        auto node = wf->get_node_by_instance(node_id);
        if (nullptr == node)
        {
            LogWarnS("Node " << node_id << " not found in wf " << wf->get_id());
            return -1;
        }

        int32_t index = 0;
        const auto& pins = (in ? node->get_inputs() : node->get_outputs());
        for (auto pin : pins)
        {
            if (pin->get_instance_id() == pin_id)
            {
                return index;
            }
            index++;
        }

        LogWarnS("Pin " << pin_id << " not found in wf " << wf->get_id());
        return -1;
    };

    for (auto& link : wf->m_links)
    {
        last_id = std::max(last_id, link->get_instance_id());

        const auto in_idx = get_pin_index(link->m_input->m_parent_node, link->m_input->get_instance_id(), true);
        const auto out_idx = get_pin_index(link->m_output->m_parent_node, link->m_output->get_instance_id(), false);
        if (in_idx != -1 && out_idx != -1)
        {
            auto jl = o_json::array();
            jl.push_back(link->get_instance_id());
            jl.push_back(link->m_output->m_parent_node);
            jl.push_back(out_idx);
            jl.push_back(link->m_input->m_parent_node);
            jl.push_back(in_idx);
            jl.push_back(link->m_output->m_state.type_str);
            jarr.push_back(std::move(jl));
        }
    }

    j["links"] = std::move(jarr);
    return last_id;
}

void WorkflowSerializerToJson::serialize_groups(const Workflow* wf, o_json& j)
{
    auto jarr = o_json::array();
    for (auto& node : wf->m_nodes)
    {
        if (!node->m_group)
            continue;

        o_json jg;
        jg["id"] = node->get_instance_id();
        jg["title"] = node->get_name();
        jg["color"] = color_to_string(node->m_clr_group);
        if (node->m_pinned)
            jg["flags"]["pinned"] = node->m_pinned;
        jg["bounding"] = { node->m_rc.Min.x, node->m_rc.Min.y, node->m_rc.Max.x, node->m_rc.Max.y };

        jarr.push_back(std::move(jg));
    }

    if (!jarr.empty())
        j["groups"] = std::move(jarr);
}

} // namespace data
