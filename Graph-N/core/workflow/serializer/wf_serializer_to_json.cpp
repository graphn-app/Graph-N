#include "core/workflow/serializer/wf_serializer_to_json.h"
#include "core/workflow/workflow.h"
#include "core/workflow/workflow_action.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/workflows_manager.h"

namespace data
{
namespace
{
std::string color_to_string(const ImVec4& color)
{
    return fmt::format(FMT_STRING("#{:02X}{:02X}{:02X}"), cast::to_i32(color.x * 255.0f), cast::to_i32(color.y * 255.0f), cast::to_i32(color.z * 255.0f));
}

std::string color_to_string(ImU32 color)
{
    return color_to_string(ImGui::ColorConvertU32ToFloat4(color));
}
} // namespace

WfSerializerToJson::WfSerializerToJson(const Workflow* wf) noexcept
    : m_wf(wf)
{}

o_json WfSerializerToJson::serialize()
{
    LogRAssert(m_wf, "Zero ptr", o_json{});

    std::shared_ptr<Workflow> virt_wf = g::pWfsManager->create_copy_virtual(m_wf);
    m_wf = virt_wf.get();

    auto& all_nodes = m_wf->get_nodes_all();
    std::set<NodeInstance*> reroutes;

    for (auto& node : all_nodes)
    {
        if (node->flags().has_not(NodeInstFlags::RerouteSplitted_R))
        {
            reroutes.insert(node.get());
        }
    }
    if (!reroutes.empty())
        cast::remove_const(m_wf)->separate_reroutes(reroutes);

    m_json = o_json();
    m_json["id"] = m_wf->get_uuid();
    m_json["revision"] = 0;
    m_json["version"] = 0.4;
    m_json["last_node_id"] = serialize_nodes();
    m_json["last_link_id"] = serialize_links();

    m_json["extra"]["workflowRendererVersion"] = "LG";
    m_json["extra"]["ds"]["scale"] = 1.0f;
    m_json["extra"]["ds"]["offset"] = { 0.0f, 0.0f };

    serialize_groups();

    m_wf = nullptr;
    g::pWfsManager->erase_wf(virt_wf->get_id());

    return std::move(m_json);
}

iid WfSerializerToJson::serialize_nodes()
{
    iid last_id = 0;
    int32_t order = 0;

    auto jarr = o_json::array();
    const auto nodes = m_wf->get_nodes_no_reroutes_r();
    for (auto node : nodes)
    {
        if (node->flags().has(NodeInstFlags::Group))
            continue;

        m_curr_node = node;
        ImVec2 node_pos(m_curr_node->m_rc.Min.x, m_curr_node->m_rc.Min.y);

        last_id = std::max(last_id, m_curr_node->get_instance_id());
        o_json jn;
        jn["id"] = m_curr_node->get_instance_id();
        jn["type"] = m_curr_node->get_object()->node_class;
        jn["pos"] = { node_pos.x, node_pos.y };
        jn["order"] = order++;
        if (m_curr_node->get_name() != m_curr_node->get_name_original())
            jn["title"] = m_curr_node->get_name();

        if (m_curr_node->flags().has(NodeInstFlags::Collapsed))
            jn["flags"]["collapsed"] = true;
        if (m_curr_node->flags().has(NodeInstFlags::Pinned))
            jn["flags"]["pinned"] = true;

        jn["color"] = color_to_string(m_curr_node->m_clr_head);
        jn["bgcolor"] = color_to_string(m_curr_node->m_clr_node);

        if (m_curr_node->flags().has(NodeInstFlags::ModeOnEvent))
            jn["mode"] = 1;
        else if (m_curr_node->flags().has(NodeInstFlags::ModeMute))
            jn["mode"] = 2;
        else if (m_curr_node->flags().has(NodeInstFlags::ModeOnTrigger))
            jn["mode"] = 3;
        else if (m_curr_node->flags().has(NodeInstFlags::ModeBypass))
            jn["mode"] = 4;
        else
            jn["mode"] = 0;

        if (m_curr_node->flags().has(NodeInstFlags::JsonCustomSerialization))
        {
            serialize_cni_node(jn);
        }
        else
        {
            jn["properties"]["Node name for S&R"] = m_curr_node->get_object()->node_class;

            serialize_pins(jn);
            serialize_widgets(jn);
        }

        jarr.push_back(std::move(jn));

        node_pos.x += 10;
        node_pos.y += 10;
    }

    m_json["nodes"] = std::move(jarr);
    return last_id;
}

void WfSerializerToJson::serialize_pins(o_json& j)
{
    const auto serialize_pin = [this](const PinInstance* pin)
    {
        o_json j;
        j["localized_name"] = pin->get_object()->def_state.name;
        j["name"] = pin->get_object()->def_state.name;
        j["type"] = pin->get_object()->def_state.type_str;
        if (pin->get_object()->def_state.name != pin->state().name)
            j["label"] = pin->state().name;
        if (pin->get_object()->is_widget())
            j["widget"]["name"] = pin->get_object()->def_state.name;

        if (pin->obj_flags().has(PinObjFlags::Input))
        {
            if (0 == pin->state().links)
                j["link"] = nullptr;
            else
            {
                for (auto& link : m_wf->get_links_all())
                {
                    if (link->get_input()->get_instance_id() == pin->get_instance_id())
                    {
                        j["link"] = link->get_instance_id();
                        break;
                    }
                }
            }
        }
        else if (pin->obj_flags().has(PinObjFlags::Output))
        {
            if (0 == pin->state().links)
                j["links"] = nullptr;
            else
            {
                j["links"] = o_json::array();
                for (auto& link : m_wf->get_links_all())
                {
                    if (link->get_output()->get_instance_id() == pin->get_instance_id())
                    {
                        j["links"].push_back(link->get_instance_id());
                    }
                }
            }
        }
        else [[unlikely]]
        {
            LogError("Unknown pin direction");
        }

        return j;
    };

    auto jarr = o_json::array();
    for (auto& pin : m_curr_node->get_inputs())
    {
        jarr.push_back(serialize_pin(pin));
    }
    j["inputs"] = std::move(jarr);

    jarr = o_json::array();
    for (auto& pin : m_curr_node->get_outputs())
    {
        jarr.push_back(serialize_pin(pin));
    }
    j["outputs"] = std::move(jarr);
}

void WfSerializerToJson::serialize_widgets(o_json& j)
{
    auto jarr = o_json::array();
    for (auto pin : m_curr_node->get_inputs())
    {
        if (pin->get_object()->is_widget())
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
                    if (PinType::COMBO == pin->state().type)
                    {
                        if (cast::to_szt(v) < pin->get_object()->combo_items_ptr.size())
                            jarr.push_back(pin->get_object()->combo_items_ptr[v]);
                    }
                    else if (PinType::COLOR == pin->state().type)
                    {
                        jarr.push_back(color_to_string(ImU32(v)));
                    }
                    else
                    {
                        jarr.push_back(v);

                        if (pin->obj_flags().has(PinObjFlags::CagRequire))
                        {
                            switch (pin->state().cag_val)
                            {
                            case ControlAfterGenerate::Fixed:
                                jarr.push_back("fixed");
                                break;
                            case ControlAfterGenerate::Increment:
                                jarr.push_back("increment");
                                break;
                            case ControlAfterGenerate::Decrement:
                                jarr.push_back("decrement");
                                break;
                            case ControlAfterGenerate::Randomize:
                                jarr.push_back("randomize");
                                break;
                            default:
                                LogErrorS("Invalid vag value");
                            }
                        }
                    }
                }
            }, pin->state().value.value());
        }
    }

    if (!jarr.empty())
        j["widgets_values"] = std::move(jarr);
}

iid WfSerializerToJson::serialize_links()
{
    iid last_id = 0;
    auto jarr = o_json::array();

    const auto get_pin_index = [this](iid node_id, iid pin_id, bool in)
    {
        const auto* node = m_wf->get_node_by_instance(node_id);
        if (nullptr == node)
        {
            LogWarnS("Node " << node_id << " not found in m_wf " << m_wf->get_id());
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

        LogWarnS("Pin " << pin_id << " not found in m_wf " << m_wf->get_id());
        return -1;
    };

    for (auto& link : m_wf->get_links_all())
    {
        last_id = std::max(last_id, link->get_instance_id());

        const auto in_idx = get_pin_index(link->get_input()->get_parent_node(), link->get_input()->get_instance_id(), true);
        const auto out_idx = get_pin_index(link->get_output()->get_parent_node(), link->get_output()->get_instance_id(), false);
        if (in_idx != -1 && out_idx != -1)
        {
            auto jl = o_json::array();
            jl.push_back(link->get_instance_id());
            jl.push_back(link->get_output()->get_parent_node());
            jl.push_back(out_idx);
            jl.push_back(link->get_input()->get_parent_node());
            jl.push_back(in_idx);
            jl.push_back(link->get_output()->state().type_str);
            jarr.push_back(std::move(jl));
        }
    }

    m_json["links"] = std::move(jarr);
    return last_id;
}

void WfSerializerToJson::serialize_groups()
{
    auto jarr = o_json::array();
    for (auto& node : m_wf->get_nodes_all())
    {
        if (node->flags().has_not(NodeInstFlags::Group))
            continue;

        o_json jg;
        jg["id"] = node->get_instance_id();
        jg["title"] = node->get_name();
        jg["color"] = color_to_string(node->m_clr_group);
        if (node->flags().has(NodeInstFlags::Pinned))
            jg["flags"]["pinned"] = true;
        jg["bounding"] = { node->m_rc.Min.x, node->m_rc.Min.y, node->m_rc.Max.x, node->m_rc.Max.y };

        jarr.push_back(std::move(jg));
    }

    if (!jarr.empty())
        m_json["groups"] = std::move(jarr);
}

} // namespace data
