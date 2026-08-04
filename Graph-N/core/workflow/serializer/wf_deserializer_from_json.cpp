#include "core/workflow/serializer/wf_deserializer_from_json.h"
#include "core/workflow/workflow.h"
#include "core/workflow/workflow_action.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/managers/nodes_manager.h"

namespace data
{

namespace
{
ImU32 parse_color(const std::string& color)
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

    LogWarnS("Unknown color format: " << color);
    return 0;
}
} // namespace


WfDeserializerFromJson::WfDeserializerFromJson(Workflow* wf, const o_json& j)
    : m_wf(wf)
    , m_json(j)
{}

bool WfDeserializerFromJson::deserialize()
{
    LogRAssert(m_wf, "Zero ptr", false);
    try
    {
        if (!m_json.contains("nodes") || !m_json["nodes"].is_array())
        {
            Log("'nodes' array not found");
            return false;
        }

        m_subgraphs.clear();
        m_node_in_pins.clear();
        m_node_out_pins.clear();
        m_node_ids.clear();

        if (m_json.contains("definitions") && m_json["definitions"].contains("subgraphs") && m_json["definitions"]["subgraphs"].is_array())
        {
            for (auto& jitem : m_json["definitions"]["subgraphs"])
            {
                if (jitem.contains("id"))
                {
                    m_subgraphs[jitem["id"]] = {};
                }
            }
        }

        for (auto& jnode : m_json["nodes"])
        {
            std::string jid;
            const auto* node = deserialize_node(jnode, jid, false);
            if (nullptr == node)
                continue;
            m_node_ids[jid] = node->get_instance_id();
            if (node->get_object()->is_dummy)
                m_wf->get_data().valid = false;
        }

        if (m_json.contains("definitions") && m_json["definitions"].contains("subgraphs") && m_json["definitions"]["subgraphs"].is_array())
        {
            for (auto& jsubgraph : m_json["definitions"]["subgraphs"])
            {
                if (!deserialize_subgraph(jsubgraph))
                    return false;
            }
        }

        if (m_json.contains("groups") && m_json["groups"].is_array())
        {
            for (const auto& g : m_json["groups"])
            {
                deserialize_group(g);
            }
        }

        if (!deserialize_links())
            return false;

        return true;
    }
    catch (const std::exception& ex)
    {
        LogExcep("Wf json parse ex:" << ex.what());
    }
    return false;
}

NodeInstance* WfDeserializerFromJson::deserialize_node(const o_json& j, std::string& node_id_from_json, bool dummy)
{
    // node definition
    if (!j.contains("type") || !j["type"].is_string() || !j.contains("id"))
        return nullptr;

    if (j["id"].is_string())                node_id_from_json = j["id"];
    else if (j["id"].is_number_integer())   node_id_from_json = std::to_string(j["id"].get<int32_t>());
    else                                    return nullptr;

    const std::string node_class = j["type"];

    // position
    ImRect rc = {};
    if (j.contains("pos") && j["pos"].is_array() && j["pos"].size() == 2 && j["pos"][0].is_number() && j["pos"][1].is_number())
    {
        rc.Min.x = j["pos"][0];
        rc.Min.y = j["pos"][1];
    }

    if (m_subgraphs.count(node_class) > 0)
    {
        m_subgraphs[node_class] = { node_id_from_json, rc };
        return nullptr;
    }

    //BREAK_IF(node_class == "ImpactCompare");

    const auto node_hash = base::HashID::make_hash(node_class);
    auto node = g::pNodesManager->find_node(node_hash);
    if (node != nullptr)
    {
        LogRAssert(!node->is_dummy, "Dummy node " << node_class << " in base map", nullptr);
    }
    if (nullptr == node || dummy)
    {
        node = g::pNodesManager->make_dummy_node(std::string(node_class));
    }

    auto node_instance = m_wf->add_node(node, rc);
    LogRAssert(node_instance->get_object()->get_hash() == node_hash, "Node ids don't match. w:" << m_wf->get_id() << ", n:" << node_instance->get_instance_id() << ", nh1:" << node_instance->get_object()->get_hash() << ", nh2: " << node_hash, nullptr);

    // name
    if (j.contains("title") && j["title"].is_string())
        node_instance->set_name(j["title"]);

    // mode
    if (j.contains("mode") && j["mode"].is_number_integer())
    {
        switch (j["mode"].get<int32_t>())
        {
        case 0:
        default:
            node_instance->set_mode(NodeMode::Always);
            break;
        case 1:
            node_instance->set_mode(NodeMode::OnEvent);
            break;
        case 2:
            node_instance->set_mode(NodeMode::Mute);
            break;
        case 3:
            node_instance->set_mode(NodeMode::OnTrigger);
            break;
        case 4:
            node_instance->set_mode(NodeMode::Bypass);
            break;
        }
    }

    // color
    if (j.contains("bgcolor") && j["bgcolor"].is_string())
    {
        const auto color = parse_color(j["bgcolor"]);
        if (color != 0)
            node_instance->set_node_color(ImGui::ColorConvertU32ToFloat4(color));
    }

    // pinned, collapsed
    node_instance->expand(true);
    node_instance->pin(false);
    if (j.contains("flags") && j["flags"].is_object())
    {
        for (const auto& [jname, jvalue] : j["flags"].items())
        {
            if (jname == "collapsed" && jvalue.is_boolean() && jvalue.get<bool>() == true)
                node_instance->expand(false);
            else if (jname == "pinned" && jvalue.is_boolean() && jvalue.get<bool>() == true)
                node_instance->pin(true);
        }
    }

    if (node_instance->cni_json_need_custom_deserialization())
    {
        if (nullptr == deserialize_cni_node(node_instance, j))
            return deserialize_node(j, node_id_from_json, true);
    }
    else
    {
        LinkMap in_pins, out_pins;

        // input
        if (j.contains("inputs") && j["inputs"].is_array())
        {
            int32_t index = 0;
            for (const auto& jinput : j["inputs"])
            {
                if (jinput.is_object())
                {
                    if (!jinput.contains("name") || !jinput["name"].is_string() || !jinput.contains("type") || !jinput["type"].is_string()) // TODO: type=-1 onTrigger
                    {
                        index++;
                        continue;
                    }

                    const std::string name = jinput["name"];
                    const std::string type = jinput["type"];

                    const auto pin_hash = Pin::make_hash(node_instance->get_object(), name, -1);
                    auto pin = node_instance->get_object()->find_pin(pin_hash, true);

                    if (node->is_dummy || nullptr == pin || pin->def_state.type_str != type)
                    {
                        if (!node_instance->get_object()->is_dummy)
                        {
                            m_wf->remove_node(node_instance);
                            return deserialize_node(j, node_id_from_json, true);
                        }

                        pin = g::pNodesManager->make_dummy_pin(node_instance->get_object(), type, name, -1);
                    }
                    else
                    {
                        pin->def_state.name = name;
                        pin->finalize();
                    }

                    if (pin->get_hash() != pin_hash)
                    {
                        LogError("Pin ids don't match. w:" << m_wf->get_id() << ", n:" << node_instance->get_instance_id() << ", ph1:" << pin->get_hash() << ", ph2:" << pin_hash);
                        m_wf->remove_node(node_instance);
                        return deserialize_node(j, node_id_from_json, true);
                    }

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
                        if (nullptr == pin_instance)
                        {
                            LogError("Pin not found in wf. w:" << m_wf->get_id() << ", n:" << node_instance->get_instance_id() << ", ph:" << pin_hash);
                            m_wf->remove_node(node_instance);
                            return deserialize_node(j, node_id_from_json, true);
                        }
                    }
                    if (jinput.contains("label") && jinput["label"].is_string())
                        pin_instance->set_name(jinput["label"]);

                    in_pins[node_instance->get_instance_id()][index++] = pin_instance->get_instance_id();
                }
            }
        }

        // output
        if (j.contains("outputs") && j["outputs"].is_array())
        {
            int32_t index = 0;
            for (const auto& joutput : j["outputs"])
            {
                if (joutput.is_object())
                {
                    if (!joutput.contains("name") || !joutput["name"].is_string() || !joutput.contains("type") || !joutput["type"].is_string()) // TODO: type=-1 onExecuted
                    {
                        index++;
                        continue;
                    }

                    const std::string name = joutput["name"];
                    const std::string type = joutput["type"];
                    std::string widget_name;
                    if (joutput.contains("widget") && joutput["widget"].is_object() && joutput["widget"].contains("name") && joutput["widget"]["name"].is_string())
                        widget_name = joutput["widget"]["name"];

                    hid pin_hash = 0;
                    if (node_instance->get_object()->output.size() > index &&
                        node_instance->get_object()->output[index]->def_state.type == PinType::ANY &&
                        name != "*")
                    {
                        pin_hash = Pin::make_hash(node_instance->get_object(), "*", index);
                    }
                    else
                    {
                        pin_hash = Pin::make_hash(node_instance->get_object(), name, index);
                    }
                    data::Pin* pin = node_instance->get_object()->find_pin(pin_hash, false);

                    if (node->is_dummy || nullptr == pin)
                    {
                        if (!node_instance->get_object()->is_dummy)
                        {
                            m_wf->remove_node(node_instance);
                            return deserialize_node(j, node_id_from_json, true);
                        }

                        pin = g::pNodesManager->make_dummy_pin(node_instance->get_object(), type, name, index);
                        node_instance->get_object()->is_dummy = true;
                    }
                    else
                    {
                        pin->def_state.name = name;
                        pin->finalize();
                    }

                    PinInstance* pin_instance = nullptr;
                    if (node->is_dummy)
                    {
                        pin_instance = m_wf->add_pin(node_instance, pin, m_wf->get_new_child_id());
                        pin->def_state.copy_to(pin_instance->state());
                        node_instance->add_output(pin_instance);
                    }
                    else
                    {
                        pin_instance = m_wf->get_pin_by_hash(node_instance->get_instance_id(), pin_hash);
                        if (nullptr == pin_instance)
                        {
                            LogError("Pin not found in wf. w:" << m_wf->get_id() << ", n:" << node_instance->get_instance_id() << ", ph:" << pin_hash);
                            m_wf->remove_node(node_instance);
                            return deserialize_node(j, node_id_from_json, true);
                        }
                    }

                    if (joutput.contains("label") && joutput["label"].is_string())
                        pin_instance->set_name(joutput["label"]);
                    else if (!widget_name.empty())
                        pin_instance->set_name(widget_name);

                    out_pins[node_instance->get_instance_id()][index++] = pin_instance->get_instance_id();
                }
            }
        }

        // values
        if (j.contains("widgets_values") && j["widgets_values"].is_array())
        {
            std::vector<PinInstance*> widgets;
            for (auto pin : node_instance->get_inputs())
            {
                if (pin->get_object()->is_widget(pin->state()))
                    widgets.push_back(pin);
            }

            size_t widget_index = 0;
            for (const auto& jvalue : j["widgets_values"])
            {
                if (widget_index >= widgets.size())
                    break;

                Primitive pval;
                if (jvalue.is_boolean())
                    pval = jvalue.get<bool>();
                else if (jvalue.is_number_float())
                    pval = jvalue.get<double>();
                else if (jvalue.is_string())
                    pval = jvalue.get<std::string>();
                else if (jvalue.is_number_unsigned())
                    pval = jvalue.get<uint64_t>();
                else if (jvalue.is_number_integer())
                    pval = jvalue.get<int64_t>();
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
                        if (PinType::INT == pin->get_object()->def_state.type ||
                            PinType::COLOR == pin->get_object()->def_state.type)
                        {
                            if (pin->get_object()->is_unsigned)
                                pin->state().value = uint64_t(v);
                            else
                                pin->state().value = int64_t(v);
                        }
                        else if (PinType::FLOAT == pin->get_object()->def_state.type)
                        {
                            pin->state().value = double(v);
                        }
                        else if (PinType::NUMBER == pin->get_object()->def_state.type)
                        {
                            pin->state().value = number(number_t(v));
                        }
                        else if (PinType::BOOLEAN == pin->get_object()->def_state.type)
                        {
                            pin->state().value = bool(v != 0);
                        }
                    }
                    if constexpr (std::is_same_v<T, double>)
                    {
                        if (PinType::FLOAT == pin->get_object()->def_state.type)
                        {
                            pin->state().value = v;
                        }
                    }
                    else if constexpr (std::is_same_v<T, bool>)
                    {
                        if (PinType::BOOLEAN == pin->get_object()->def_state.type)
                        {
                            pin->state().value = v;
                        }
                    }
                    else if constexpr (std::is_same_v<T, std::string>)
                    {
                        if (PinType::STRING == pin->get_object()->def_state.type)
                        {
                            pin->state().value = v;
                        }
                        else if (PinType::COMBO == pin->get_object()->def_state.type)
                        {
                            pin->state().value = int64_t(0);
                            pin->get_object()->def_state.value = int64_t(0);

                            if (pin->get_object()->combo_items.empty() && !v.empty())
                            {
                                auto obj = pin->get_object();
                                obj->combo_items.push_back(v);
                                obj->combo_items_str.push_back(v);
                                obj->combo_items_ptr.push_back(obj->combo_items_str.begin()->c_str());
                                obj->min = int64_t(0);
                                obj->max = cast::to_i64(obj->combo_items.size() - 1);
                                obj->def_state.value = int64_t(0);
                                pin->state().value = obj->def_state.value;
                            }
                            else
                            {
                                int64_t combo_idx = 0;
                                for (auto combo_item : pin->get_object()->combo_items_ptr)
                                {
                                    if (v == combo_item)
                                    {
                                        pin->state().value = combo_idx;
                                        break;
                                    }
                                    combo_idx++;
                                }
                            }
                        }
                    }
                }, pval.value());
            }
        }

        m_node_in_pins.merge(in_pins);
        m_node_out_pins.merge(out_pins);
    }

    node_instance->reset_width();
    return node_instance;
}

bool WfDeserializerFromJson::deserialize_subgraph(const o_json& j)
{
    if (!j.contains("id") || !j["id"].is_string())
        return true;

    const auto sg_it = m_subgraphs.find(j["id"]);
    if (sg_it == m_subgraphs.end())
        return true;
    const auto& sg_rc = sg_it->second.second;

    if (!j.contains("nodes") || !j["nodes"].is_array())
        return true;

    for (auto& jnode : j["nodes"])
    {
        std::string jid;
        auto node = deserialize_node(jnode, jid, false);
        if (nullptr == node)
            continue;

        node->m_rc.Min.x = sg_rc.Min.x + node->m_rc.Min.x;
        node->m_rc.Min.y = sg_rc.Min.y + node->m_rc.Min.y;

        m_node_ids[jid] = node->get_instance_id();
        if (node->get_object()->is_dummy)
            m_wf->get_data().valid = false;
    }

    if (j.contains("groups") && j["groups"].is_array())
    {
        for (const auto& jg : j["groups"])
        {
            auto group = deserialize_group(jg);
            if (group != nullptr)
            {
                group->m_rc.Min.x = sg_rc.Min.x + group->m_rc.Min.x;
                group->m_rc.Min.y = sg_rc.Min.y + group->m_rc.Min.y;
            }
        }
    }

    return true;
}

NodeInstance* WfDeserializerFromJson::deserialize_group(const o_json& j)
{
    if (!j.contains("bounding") || !j["bounding"].is_array() || j["bounding"].size() != 4 ||
        !j["bounding"][0].is_number() || !j["bounding"][1].is_number() || !j["bounding"][2].is_number() || !j["bounding"][3].is_number())
    {
        return nullptr;
    }

    NodeInstance* node = m_wf->add_group_node();
    node->set_instance_id(m_wf->get_new_child_id());
    node->set_workflow_id(m_wf->get_id());

    auto& jbounding = j["bounding"];
    node->m_rc = ImRect(ImVec2(jbounding[0].get<float>(), jbounding[1].get<float>()), ImVec2(jbounding[2].get<float>(), jbounding[3].get<float>()));

    if (j.contains("title") && j["title"].is_string())
        node->set_name(j["title"]);

    if (j.contains("flags") && j["flags"].is_object())
    {
        for (const auto& [jname, jvalue] : j["flags"].items())
        {
            if (jname == "pinned" && jvalue.is_boolean() && jvalue.get<bool>() == true)
                node->pin(true);
        }
    }

    if (j.contains("color") && j["color"].is_string())
    {
        const auto color = parse_color(j["color"]);
        if (color != 0)
            node->set_group_color(ImGui::ColorConvertU32ToFloat4(color));
    }
    else
        node->reset_to_default_colors();

    return node;
}

bool WfDeserializerFromJson::deserialize_links()
{
    const auto find_pin = [this](const LinkMap& map, const NodeInstance* node, int32_t index) -> PinInstance*
    {
        auto node_it = map.find(node->get_instance_id());
        if (node_it == map.end())
            return nullptr;

        auto& sub_map = node_it->second;
        auto pin_it = sub_map.find(index);
        if (pin_it == sub_map.end())
            return nullptr;

        auto pin_iid = pin_it->second;
        return m_wf->get_pin_by_instance(pin_iid);
    };

    const auto build_link = [this, &find_pin]
            (const std::string& src_node_id, const std::string& dest_node_id, int32_t src_pin_idx, int32_t dest_pin_idx)
    {
        iid src_node_iid = 0, dest_node_iid = 0;

        auto node_it = m_node_ids.find(src_node_id);
        if (node_it == m_node_ids.end())
        {
            LogWarnS("Node " << src_node_id << " not found on json definition");
            return;
        }
        src_node_iid = node_it->second;

        node_it = m_node_ids.find(dest_node_id);
        if (node_it == m_node_ids.end())
        {
            LogWarnS("Node " << dest_node_id << " not found on json definition");
            return;
        }
        dest_node_iid = node_it->second;

        const auto* src_node = m_wf->get_node_by_instance(src_node_iid);
        const auto* dest_node = m_wf->get_node_by_instance(dest_node_iid);
        if (nullptr == src_node || nullptr == dest_node)
        {
            LogWarnS("Linked node(s) not found. w:" << m_wf->get_id() << ", n1:" << src_node_iid << ":" << (src_node != nullptr) << ", n2:" << dest_node_iid << ":" << (dest_node != nullptr));
            return;
        }

        auto src_pin = find_pin(m_node_out_pins, src_node, src_pin_idx);
        auto dest_pin = find_pin(m_node_in_pins, dest_node, dest_pin_idx);
        if (nullptr == src_pin || nullptr == dest_pin)
        {
            LogWarnS("Linked pin(s) not found. w:" << m_wf->get_id() << ", n1:" << src_node_iid << ", n2:" << dest_node_iid << ", p1:" << (src_pin != nullptr) << ", p2:" << (dest_pin != nullptr));
            return;
        }

        if (Link::can_link(src_pin, dest_pin))
            Link::link(src_pin, dest_pin, 0);
        else
            LogWarnS("Can't link pins");
    };

    using LinkDef = std::pair<std::string, int32_t>;
    using LinksDef = std::pair<LinkDef, LinkDef>; // source, dest
    std::list<LinksDef> links;
    std::list<LinksDef> sg_src_links;
    std::list<LinksDef> sg_dest_links;

    // externals links

    if (m_json.contains("links") && m_json["links"].is_array())
    {
        for (const auto& jlink : m_json["links"])
        {
            if (jlink.size() < 5)
            {
                LogWarn("Num of links array must be at least 5. Currently: " << jlink.size());
                continue;
            }
            if (!jlink[0].is_number_integer() ||
                (!jlink[1].is_number_integer() && !jlink[1].is_string()) ||
                !jlink[2].is_number_integer() ||
                (!jlink[3].is_number_integer() && !jlink[3].is_string()) ||
                !jlink[4].is_number_integer())
            {
                LogWarn("Unknown links format");
                continue;
            }

            const int32_t src_idx = jlink[2].get<int32_t>();
            const int32_t dest_idx = jlink[4].get<int32_t>();

            std::string src_node, dest_node;
            if (jlink[1].is_number_integer())    src_node = std::to_string(jlink[1].get<int32_t>());
            else                                 src_node = jlink[1].get<std::string>();
            if (jlink[3].is_number_integer())    dest_node = std::to_string(jlink[3].get<int32_t>());
            else                                 dest_node = jlink[3].get<std::string>();

            links.push_back(std::make_pair(std::make_pair(src_node, src_idx), std::make_pair(dest_node, dest_idx)));
        }
    }

    // subgraph links

    if (m_json.contains("definitions") && m_json["definitions"].contains("subgraphs") && m_json["definitions"]["subgraphs"].is_array())
    {
        for (auto& jsubgraph : m_json["definitions"]["subgraphs"])
        {
            if (!jsubgraph.contains("id") || !jsubgraph["id"].is_string())
                continue;

            if (!jsubgraph.contains("inputNode") || !jsubgraph["inputNode"].contains("id") ||
                !jsubgraph.contains("outputNode") || !jsubgraph["outputNode"].contains("id"))
            {
                continue;
            }

            std::string sg_in_id, sg_out_id;
            if (jsubgraph["inputNode"]["id"].is_string())                 sg_in_id = jsubgraph["inputNode"]["id"];
            else if (jsubgraph["inputNode"]["id"].is_number_integer())    sg_in_id = std::to_string(jsubgraph["inputNode"]["id"].get<int32_t>());
            else continue;
            if (jsubgraph["outputNode"]["id"].is_string())                sg_out_id = jsubgraph["outputNode"]["id"];
            else if (jsubgraph["outputNode"]["id"].is_number_integer())   sg_out_id = std::to_string(jsubgraph["outputNode"]["id"].get<int32_t>());
            else continue;

            const std::string sg_uuid = jsubgraph["id"];
            const auto sg_it = m_subgraphs.find(sg_uuid);
            if (sg_it == m_subgraphs.end())
                continue;
            const auto& sg_as_node_id = sg_it->second.first;

            if (jsubgraph.contains("links") && jsubgraph["links"].is_array())
            {
                for (auto& jitem : jsubgraph["links"])
                {
                    if (!(jitem.contains("origin_id") &&
                        jitem.contains("origin_slot") && jitem["origin_slot"].is_number_integer() &&
                        jitem.contains("target_id") &&
                        jitem.contains("target_slot") && jitem["target_slot"].is_number_integer()))
                    {
                        continue;
                    }

                    const int32_t src_pin_idx = jitem["origin_slot"].get<int32_t>();
                    const int32_t dest_pin_idx = jitem["target_slot"].get<int32_t>();
                    std::string src_node_id, dest_node_id;
                    if (jitem["origin_id"].is_number_integer())      src_node_id = std::to_string(jitem["origin_id"].get<int32_t>());
                    else if (jitem["origin_id"].is_string())         src_node_id = jitem["origin_id"];
                    else continue;
                    if (jitem["target_id"].is_number_integer())      dest_node_id = std::to_string(jitem["target_id"].get<int32_t>());
                    else if (jitem["target_id"].is_string())         dest_node_id = jitem["target_id"];
                    else continue;

                    if (src_node_id == sg_in_id)
                    {
                        src_node_id = sg_as_node_id;
                        sg_dest_links.push_back(std::make_pair(std::make_pair(src_node_id, src_pin_idx), std::make_pair(dest_node_id, dest_pin_idx)));
                    }
                    else if (dest_node_id == sg_out_id)
                    {
                        dest_node_id = sg_as_node_id;
                        sg_src_links.push_back(std::make_pair(std::make_pair(dest_node_id, dest_pin_idx), std::make_pair(src_node_id, src_pin_idx)));
                    }
                    else
                    {
                        links.push_back(std::make_pair(std::make_pair(src_node_id, src_pin_idx), std::make_pair(dest_node_id, dest_pin_idx)));
                    }
                }
            }
        }
    }

    const auto search_sg = [&sg_src_links, &sg_dest_links](std::string& node, int32_t& idx, bool search_src)
    {
        auto it = (search_src ? sg_src_links.begin() : sg_dest_links.begin());
        const auto end = (search_src ? sg_src_links.end() : sg_dest_links.end());
        for ( ; it != end; ++it)
        {
            auto& sg = it->first;
            if (sg.first == node && sg.second == idx)
            {
                auto& linked = it->second;
                node = linked.first;
                idx = linked.second;
                if (!search_src)
                    sg_dest_links.erase(it);
                return true;
            }
        }
        return false;
    };

    const auto locate_node = [this, &search_sg](LinkDef& link_definition, bool src_link)
    {
        do
        {
            if (m_node_ids.find(link_definition.first) != m_node_ids.end())
                break;
            if (search_sg(link_definition.first, link_definition.second, src_link))
                continue;
            return false;
        }
        while (true);
        return true;
    };

    std::set<std::tuple<std::string, std::string, int32_t, int32_t>> built_links;

    const auto final_build = [&links, &locate_node, &build_link, &built_links]()
    {
        int32_t links_built = 0;
        for (auto it = links.begin(); it != links.end(); ++it)
        {
            LinkDef src = it->first;
            if (!locate_node(src, true))
                continue;
            LinkDef dest = it->second;
            if (!locate_node(dest, false))
                continue;

            const auto link = std::make_tuple(src.first, dest.first, src.second, dest.second);
            if (built_links.count(link) > 0)
                continue;

            build_link(src.first, dest.first, src.second, dest.second);
            links_built++;

            built_links.insert(link);
        }
        return links_built;
    };

    while (final_build() > 0)
        ;

    if (!sg_dest_links.empty())     // TODO:FIX unpack subgraphs in subgraphs in subgraphs (copy of copy etc)
    {
        LogWarn("Not all link definitions were used");
    }

    return true;
}

} // namespace data
