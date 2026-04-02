
#include "core/workflow/workflow_serializer_to_json.h"
#include "core/workflow/workflow.h"
#include "core/workflow/workflow_action.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/managers/nodes_manager.h"

namespace data
{

ImU32 WorkflowSerializerToJson::parse_color(const std::string& color)
{
    const auto make_clr3 = [](char c)
    {
        const std::string clr(2, c);
        return std::strtoul(clr.c_str(), nullptr, 16);
    };
    const auto make_clr6 = [](char c1, char c2)
    {
        std::string clr(1, c1);
        clr.push_back(c2);
        return std::strtoul(clr.c_str(), nullptr, 16);
    };

    if (color.size() == 4 && color[0] == '#')
    {
        const auto r = make_clr3(color[1]);
        const auto g = make_clr3(color[2]);
        const auto b = make_clr3(color[3]);
        return IM_COL32(r, g, b, 255);
    }
    if (color.size() == 7 && color[0] == '#')
    {
        const auto r = make_clr6(color[1], color[2]);
        const auto g = make_clr6(color[3], color[4]);
        const auto b = make_clr6(color[5], color[6]);
        return IM_COL32(r, g, b, 255);
    }

    return 0;
}

bool WorkflowSerializerToJson::deserialize(Workflow* wf, const nlohmann::json& j)
{
    try
    {
        if (!j.contains("nodes") || !j["nodes"].is_array())
        {
            LogS("'nodes' array not found");
            return false;
        }

        std::map<std::string, iid> node_ids;
        LinkMap in_pins, out_pins; // std::map<iid, std::map<int32_t, iid>>

        for (auto& jnode : j["nodes"])
        {
            std::string jid;
            auto node = deserialize_node(wf, jnode, jid, in_pins, out_pins);
            if (nullptr == node)
                return false;
            node_ids[jid] = node->get_instance_id();
            if (node->m_object->is_dummy)
                wf->get_data().valid = false;
        }

        if (!deserialize_links(wf, j, node_ids, in_pins, out_pins))
            return false;

        if (!deserialize_groups(wf, j))
            return false;

        return true;
    }
    catch (std::exception& ex)
    {
        LogExcepS(ExcepCategory::API, ex.what());
    }
    return false;
}

NodeInstance* WorkflowSerializerToJson::deserialize_node(
    Workflow* wf, const nlohmann::json& j, std::string& node_id_from_json, LinkMap& in_pins, LinkMap& out_pins)
{
    try
    {
        // node definition
        if (!j.contains("type") || !j["type"].is_string() || !j.contains("id"))
            return nullptr;

        if (j["id"].is_string())
            node_id_from_json = j["id"];
        else if (j["id"].is_number_integer())
            node_id_from_json = std::to_string(j["id"].get<int32_t>());
        else
            return nullptr;

        NodeInstance* node = wf->add_blank_node();
        node->m_iid = wf->get_new_child_id();
        node->m_wid = wf->get_id();

        const std::string node_class = j["type"];
        node->m_obj_hid = base::HashID::create_id(node_class);
        node->m_object = g::pNodesManager->find_node(node->m_obj_hid);
        if (nullptr == node->m_object)
        {
            LogWarnS("Node " << node_class << " not found");
            auto obj = g::pNodesManager->make_dummy_node(std::string(node_class));
            node->set_object(obj);
        }
        if (node->m_object->get_id() != node->m_obj_hid)
        {
            LogAndThrowInternalErrorS(ExcepCategory::Logic, "Node ids don't match " << node->m_object->get_id() << " != " << node->m_obj_hid);
        }

        // name
        if (j.contains("title") && j["title"].is_string())
            node->set_name(j["title"]);

        // position
        if (j.contains("pos") && j["pos"].is_array() && j["pos"].size() == 2 && j["pos"][0].is_number() && j["pos"][1].is_number())
        {
            node->m_rc.Min.x = j["pos"][0];
            node->m_rc.Min.y = j["pos"][1];
        }

        // mode
        if (j.contains("mode") && j["mode"].is_number_integer())
        {
            switch (j["mode"].get<int32_t>())
            {
            case 0:
                node->set_mode(NodeMode::Always);
                break;
            case 1:
                node->set_mode(NodeMode::OnEvent);
                break;
            case 2:
                node->set_mode(NodeMode::Mute);
                break;
            case 3:
                node->set_mode(NodeMode::OnTrigger);
                break;
            case 4:
                node->set_mode(NodeMode::Bypass);
                break;
            }
        }

        // color
        if (j.contains("bgcolor") && j["bgcolor"].is_string())
        {
            auto color = parse_color(j["bgcolor"]);
            if (color != 0)
                node->set_node_color(ImGui::ColorConvertU32ToFloat4(color));
        }
        else
            node->reset_to_default_colors();

        // pinned, collapsed
        node->m_expanded = true;
        node->m_pinned = false;
        if (j.contains("flags") && j["flags"].is_object())
        {
            for (const auto& [name, value] : j["flags"].items())
            {
                if (name == "collapsed" && value.is_boolean() && value.get<bool>() == true)
                    node->m_expanded = false;
                else if (name == "pinned" && value.is_boolean() && value.get<bool>() == true)
                    node->m_pinned = true;
            }
        }

        // input
        if (j.contains("inputs") && j["inputs"].is_array())
        {
            int32_t index = 0;
            for (const auto& input : j["inputs"])
            {
                if (input.is_object())
                {
                    if (!input.contains("name") || !input["name"].is_string() || !input.contains("type") || !input["type"].is_string()) // TODO: type=-1 onTrigger
                    {
                        index++;
                        continue;
                    }

                    const std::string name = input["name"];
                    const std::string type = input["type"];

                    const auto pin_hash = Pin::create_id(node->m_object, name, -1);
                    auto pin = node->m_object->find_pin(pin_hash, true);
                    if (nullptr == pin || pin->def_state.type_str != type)
                    {
                        LogWarnS("Input Pin " << name << " not found");
                        pin = g::pNodesManager->make_dummy_pin(node->m_object, name, -1);

                        pin->initialize(type);
                        pin->is_dummy = true;
                    }
                    pin->def_state.name = name;
                    pin->finalize();

                    if (pin->get_id() != pin_hash)
                    {
                        LogAndThrowInternalErrorS(ExcepCategory::Logic, "Pin ids don't match " << pin->get_id() << " != " << pin_hash);
                    }

                    auto instance = wf->add_pin(node, pin, wf->get_new_child_id());
                    instance->m_obj_class = name;
                    instance->m_state = pin->def_state;

                    if (input.contains("label") && input["label"].is_string())
                        instance->m_state.name = input["label"];

                    node->m_input.push_back(instance);
                    if (pin->is_dummy)
                        node->m_object->is_dummy = true;

                    in_pins[node->get_instance_id()][index++] = instance->get_instance_id();
                }
            }
        }

        // values
        if (j.contains("widgets_values") && j["widgets_values"].is_array())
        {
            std::vector<PinInstance*> widgets;
            for (auto pin : node->m_input)
            {
                if (pin->m_object->is_widget())
                    widgets.push_back(pin);
            }

            size_t widget_index = 0;
            for (const auto& value : j["widgets_values"])
            {
                if (widget_index >= widgets.size())
                    break;

                primitive pval;
                if (value.is_boolean())
                    pval = value.get<bool>();
                else if (value.is_number_float())
                    pval = value.get<double>();
                else if (value.is_string())
                    pval = value.get<std::string>();
                else if (value.is_number_unsigned())
                    pval = value.get<uint64_t>();
                else if (value.is_number_integer())
                    pval = value.get<int64_t>();
                else
                {
                    widget_index++;
                    continue;
                }

                auto pin = widgets[widget_index++];

                std::visit([pin](const auto& v)
                {
                    using T = std::decay_t<decltype(v)>;

                    if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>)
                    {
                        if (PinType::INT == pin->m_object->def_state.type || PinType::NUMBER == pin->m_object->def_state.type || PinType::COLOR == pin->m_object->def_state.type)
                        {
                            if (pin->m_object->is_unsigned)
                                pin->m_state.value = uint64_t(v);
                            else
                                pin->m_state.value = int64_t(v);
                        }
                        else if (PinType::FLOAT == pin->m_object->def_state.type)
                        {
                            pin->m_state.value = double(v);
                        }
                        else if (PinType::BOOLEAN == pin->m_object->def_state.type)
                        {
                            pin->m_state.value = bool(v != 0);
                        }
                    }
                    if constexpr (std::is_same_v<T, double>)
                    {
                        if (PinType::FLOAT == pin->m_object->def_state.type)
                        {
                            pin->m_state.value = v;
                        }
                    }
                    else if constexpr (std::is_same_v<T, bool>)
                    {
                        if (PinType::BOOLEAN == pin->m_object->def_state.type)
                        {
                            pin->m_state.value = v;
                        }
                    }
                    else if constexpr (std::is_same_v<T, std::string>)
                    {
                        if (PinType::STRING == pin->m_object->def_state.type)
                        {
                            pin->m_state.value = v;
                        }
                        else if (PinType::COMBO == pin->m_object->def_state.type)
                        {
                            for (auto& combo_item : pin->m_object->combo_items_str)
                            {
                                int64_t combo_idx = 0;
                                if (v == combo_item)
                                {
                                    pin->m_state.value = combo_idx;
                                    break;
                                }
                                combo_idx++;
                            }
                        }
                    }
                }, pval);
            }
        }

        // output
        if (j.contains("outputs") && j["outputs"].is_array())
        {
            int32_t index = 0;
            for (const auto& output : j["outputs"])
            {
                if (output.is_object())
                {
                    if (!output.contains("name") || !output["name"].is_string() || !output.contains("type") || !output["type"].is_string()) // TODO: type=-1 onExecuted
                    {
                        index++;
                        continue;
                    }

                    const std::string name = output["name"];
                    const std::string type = output["type"];

                    const auto pin_hash = Pin::create_id(node->m_object, name, index);
                    auto pin = node->m_object->find_pin(pin_hash, false);
                    if (nullptr == pin || pin->def_state.type_str != type)
                    {
                        LogWarnS("Output Pin " << type << " not found");
                        pin = g::pNodesManager->make_dummy_pin(node->m_object, name, index);

                        pin->initialize(type);
                        pin->is_dummy = true;
                    }
                    pin->def_state.name = name;
                    pin->finalize();

                    if (pin->get_id() != pin_hash)
                    {
                        LogAndThrowInternalErrorS(ExcepCategory::Logic, "Pin ids don't match " << pin->get_id() << " != " << pin_hash);
                    }

                    auto instance = wf->add_pin(node, pin, wf->get_new_child_id());
                    instance->m_obj_class = name;
                    instance->m_state = pin->def_state;

                    if (output.contains("label") && output["label"].is_string())
                        instance->m_state.name = output["label"];

                    node->m_output.push_back(instance);
                    if (pin->is_dummy)
                        node->m_object->is_dummy = true;

                    out_pins[node->get_instance_id()][index++] = instance->get_instance_id();
                }
            }
        }

        node->calc_node_width(false, false);
        return node;
    }
    catch (std::exception& ex)
    {
        LogExcepS(ExcepCategory::API, ex.what());
    }

    return nullptr;
}

bool WorkflowSerializerToJson::deserialize_links(
    Workflow* wf, const nlohmann::json& j, const std::map<std::string, iid>& node_ids, const LinkMap& in_pins, const LinkMap& out_pins)
{
    const auto find_pin = [wf](const LinkMap& map, const NodeInstance* node, int32_t index) -> PinInstance*
    {
        auto node_it = map.find(node->get_instance_id());
        if (node_it == map.end())
            return nullptr;

        auto& sub_map = node_it->second;
        auto pin_it = sub_map.find(index);
        if (pin_it == sub_map.end())
            return nullptr;

        auto pin_iid = pin_it->second;
        return wf->get_pin_by_instance(pin_iid);
    };

    try
    {
        if (!j.contains("links") || !j["links"].is_array())
            return true;

        for (const auto& link : j["links"])
        {
            if (link.size() < 5)
            {
                LogWarnS("Num of links must be at least 5. Currently: " << link.size());
                continue;
            }
            if (!link[0].is_number_integer() || 
                (!link[1].is_number_integer() && !link[1].is_string()) ||
                !link[2].is_number_integer() ||
                (!link[3].is_number_integer() && !link[3].is_string()) ||
                !link[4].is_number_integer())
            {
                LogWarnS("Unknown links format");
                continue;
            }

            std::string src_node_id, dest_node_id;
            if (link[1].is_number_integer())    src_node_id = std::to_string(link[1].get<int32_t>());
            else                                src_node_id = link[1];
            if (link[3].is_number_integer())    dest_node_id = std::to_string(link[3].get<int32_t>());
            else                                dest_node_id = link[3];

            iid src_node_iid = 0, dest_node_iid = 0;

            auto node_it = node_ids.find(src_node_id);
            if (node_it == node_ids.end())
            {
                LogWarnS("Node " << src_node_id << " not found on json definition");
                continue;
            }
            src_node_iid = node_it->second;

            node_it = node_ids.find(dest_node_id);
            if (node_it == node_ids.end())
            {
                LogWarnS("Node " << dest_node_id << " not found on json definition");
                continue;
            }
            dest_node_iid = node_it->second;

            auto src_node = wf->get_node_by_instance(src_node_iid);
            auto dest_node = wf->get_node_by_instance(dest_node_iid);
            if (nullptr == src_node || nullptr == dest_node)
            {
                LogWarnS("Linked node(s) not found");
                continue;
            }

            // const int32_t link_id = link[0].get<int32_t>(); - not interesting
            const int32_t src_pin_idx = link[2].get<int32_t>();
            const int32_t dest_pin_idx = link[4].get<int32_t>();

            auto src_pin = find_pin(out_pins, src_node, src_pin_idx);
            auto dest_pin = find_pin(in_pins, dest_node, dest_pin_idx);
            if (nullptr == src_pin || nullptr == dest_pin)
            {
                LogWarnS("Linked pin(s) not found");
                continue;
            }

            if (nullptr == Link::link(src_pin, dest_pin))
                LogWarnS("Can't link pins");
        }

        return true;
    }
    catch (std::exception& ex)
    {
        LogExcepS(ExcepCategory::API, ex.what());
    }
    return false;
}

bool WorkflowSerializerToJson::deserialize_groups(Workflow* wf, const nlohmann::json& j)
{
    try
    {
        if (!j.contains("groups") || !j["groups"].is_array())
            return true;

        for (const auto& g : j["groups"])
        {
            if (!g.contains("bounding") || !g["bounding"].is_array() || g["bounding"].size() != 4 ||
                !g["bounding"][0].is_number() || !g["bounding"][1].is_number() || !g["bounding"][2].is_number() || !g["bounding"][3].is_number())
            {
                continue;
            }

            NodeInstance* node = wf->add_blank_node();
            node->m_iid = wf->get_new_child_id();
            node->m_wid = wf->get_id();
            node->m_group = true;

            auto& bounding = g["bounding"];
            node->m_rc = ImRect(ImVec2(bounding[0].get<float>(), bounding[1].get<float>()), ImVec2(bounding[2].get<float>(), bounding[3].get<float>()));

            if (g.contains("title") && g["title"].is_string())
                node->set_name(g["title"]);

            if (g.contains("flags") && g["flags"].is_object())
            {
                for (const auto& [name, value] : g["flags"].items())
                {
                    if (name == "pinned" && value.is_boolean() && value.get<bool>() == true)
                        node->m_pinned = true;
                }
            }

            if (g.contains("color") && g["color"].is_string())
            {
                const auto color = parse_color(g["color"]);
                if (color != 0)
                    node->set_group_color(ImGui::ColorConvertU32ToFloat4(color));
            }
            else
                node->reset_to_default_colors();
        }
        return true;
    }
    catch (std::exception& ex)
    {
        LogExcepS(ExcepCategory::API, ex.what());
    }
    return false;
}

} // namespace data
