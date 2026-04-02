
#include "core/managers/nodes_manager.h"
#include "core/managers/settings_manager.h"

namespace mngr
{

void NodesManager::free()
{
    m_nodes.clear();
    m_pins.clear();
    m_node_items.clear();
    m_nodes_tree.clear();
    m_nodes_plain.clear();
    m_labels.clear();
    m_nodes_in_label.clear();
}

void NodesManager::init_from_copy(NodesManager& copy)
{
    LogF;
    free();
    m_nodes = std::move(copy.m_nodes);
    m_pins = std::move(copy.m_pins);
    m_node_items = std::move(copy.m_node_items);
    m_nodes_tree = std::move(copy.m_nodes_tree);
    m_nodes_plain = std::move(copy.m_nodes_plain);
    m_labels = std::move(copy.m_labels);
    m_last_label_id = copy.m_last_label_id;
    m_nodes_in_label = std::move(copy.m_nodes_in_label);
    calc_number_inside("", "", "");
}

void NodesManager::calc_number_inside(const std::string& filter, const std::string& in_pin, const std::string& out_pin)
{
    const auto pin_filter = [](const std::vector<data::Pin*>& pins, const std::string& filter)
    {
        if ("*" == filter && !pins.empty())
            return true;

        for (auto pin : pins)
        {
            if (pin->type_any)
                return true;
            if (pin->def_state.type_str == filter)
                return true;
        }
        return false;
    };

    const auto pins_filter = [&in_pin, &out_pin, &pin_filter](data::NodeItem* node_item)
    {
        if (in_pin.empty() && out_pin.empty())
            return true;

        if (!in_pin.empty())
        {
            if (pin_filter(node_item->node->required.pins, in_pin) ||
                pin_filter(node_item->node->optional.pins, in_pin))
            {
                return true;
            }
        }
        if (!out_pin.empty())
        {
            if (pin_filter(node_item->node->output, out_pin))
            {
                return true;
            }
        }

        return false;
    };

    std::function<void(std::vector<data::NodeItem*>&, uint32_t*, bool)> calculator;
    calculator = [&calculator, &filter, &pins_filter](std::vector<data::NodeItem*>& nodes, uint32_t* count, bool force_unfilter)
    {
        for (auto node : nodes)
        {
            node->nodes_inside = 0;
            if (!node->children.empty())
            {
                const bool category_unfiltered = force_unfilter || (!filter.empty() && node->search.find(filter) != std::string::npos);
                calculator(node->children, &node->nodes_inside, category_unfiltered);
                if (count != nullptr)
                    *count += node->nodes_inside;
            }
            else if (node->is_node)
            {
                if (force_unfilter || filter.empty())
                {
                    node->filtered = !pins_filter(node);
                    if (count != nullptr)
                        *count += node->filtered ? 0 : 1;
                }
                else
                {
                    node->filtered = (node->search.find(filter) == std::string::npos);
                    if (!node->filtered)
                    {
                        node->filtered = !pins_filter(node);
                        if (count != nullptr)
                            *count += node->filtered ? 0 : 1;
                    }
                }
            }
        }
    };

    uint32_t count = 0;
    calculator(m_nodes_tree, &count, false);
    Log("Filter = (" << filter << "), count=" << count);
}

data::Node* NodesManager::find_node(hid id) const
{
    const auto it = m_nodes.find(id);
    if (it == m_nodes.end())
        return nullptr;
    return it->second.get();
}

data::Pin* NodesManager::find_pin(hid id) const
{
    const auto it = m_pins.find(id);
    if (it == m_pins.end())
        return nullptr;
    return it->second.get();
}

data::Node* NodesManager::make_dummy_node(const std::string& node_class)
{
    auto node = std::make_unique<data::Node>(node_class);
    auto it = m_nodes.find(node->get_id());
    if (it != m_nodes.end())
    {
        LogAndThrowInternalError(ExcepCategory::Logic, "Dummy node " << node_class << " already exists");
        return it->second.get();
    }

    node->name = node_class;
    node->node_class = node_class;
    node->is_dummy = true;

    auto ptr = node.get();
    m_nodes.emplace(node->get_id(), std::move(node));
    Log("Dummy node " << node_class << "/" << ptr->get_id() << " created");
    return ptr;
}

data::Pin* NodesManager::make_dummy_pin(data::Node* parent, const std::string& name, int64_t out_index)
{
    std::unique_ptr<data::Pin> pin;
    if (out_index >= 0)
    {
        pin = std::make_unique<data::Pin>(parent, out_index);
        pin->output = true;
        parent->output.push_back(pin.get());
    }
    else
    {
        pin = std::make_unique<data::Pin>(parent, name);
        pin->input = true;
        pin->required = true;
        parent->required.pins.push_back(pin.get());
    }
    auto it = m_pins.find(pin->get_id());
    if (it != m_pins.end())
    {
        LogAndThrowInternalError(ExcepCategory::Logic, "Dummy pin " << parent->name << "/" << name << " already exists");
        return it->second.get();
    }

    pin->finalize();
    pin->is_dummy = true;
    pin->def_state.name = name;
    pin->def_state.w_name = ImGui::CalcTextSize(name.c_str(), name.c_str() + name.size()).x;

    auto ptr = pin.get();
    m_pins.emplace(pin->get_id(), std::move(pin));
    Log("Dummy pin " << parent->name << "/" << name << "/" << ptr->get_id() << " created");
    return ptr;
}

void NodesManager::init_labels()
{
    m_labels.clear();
    m_nodes_in_label.clear();
    m_last_label_id = 0;

    auto labels = g::pOptsManager->nodes_labels_load(base::HashID::create_id(DlgObjID[cast::to_szt(DlgObj::PanelNodes)]));
    if (labels.empty())
        return;

    Log("Node labels loaded: " << labels);
    try
    {
        using json = nlohmann::json;
        const json j = json::parse(labels);

        for (auto& obj : j)
        {
            std::string name = obj["name"];
            uint32_t id = obj["id"];
            m_labels.push_back({ name, id });

            if (id > m_last_label_id)
                m_last_label_id = id;

            auto& nodes_in_label = m_nodes_in_label[id];
            std::vector<hid> node_ids = obj["nodes"];
            for (auto node_id : node_ids)
            {
                for (auto node_item : m_nodes_plain)
                {
                    if (node_item->is_node && node_item->node->get_id() == node_id)
                    {
                        nodes_in_label.push_back(node_item);
                        break;
                    }
                }
            }

            sort_nodes(nodes_in_label);
        }

        m_labels.sort([](const std::pair<std::string, uint32_t>& l, const std::pair<std::string, uint32_t>& r)
            { return tools::str::to_lower_ascii(l.first) < tools::str::to_lower_ascii(r.first); });
    }
    catch (const std::exception& ex)
    {
        LogInternalLogic("Failed to parse labels (" << labels << "). Err: " << ex.what());
    }
}

uint32_t NodesManager::create_label()
{
    std::string label = "new label";
    size_t label_idx = 1;
    for (auto it = m_labels.begin(); it != m_labels.end(); )
    {
        const auto& name = it->first;
        if (tools::str::to_lower_ascii(name) == label)
        {
            label = fmt::format(FMT_STRING("new label ({})"), ++label_idx);
            it = m_labels.begin();
            continue;
        }
        ++it;
    }

    m_last_label_id++;
    m_nodes_in_label[m_last_label_id];

    Log("New node label added: " << label << " = " << m_last_label_id);

    m_labels.push_back({ label, m_last_label_id });
    m_labels.sort([](const std::pair<std::string, uint32_t>& l, const std::pair<std::string, uint32_t>& r)
        { return tools::str::to_lower_ascii(l.first) < tools::str::to_lower_ascii(r.first); });

    save_labels();
    return m_last_label_id;
}

bool NodesManager::rename_label(uint32_t label_id, const std::string& new_name)
{
    std::list<std::pair<std::string, uint32_t>>::iterator label_it = m_labels.begin();
    for (auto it = m_labels.begin(); it != m_labels.end(); ++it)
    {
        if (it->second == label_id)
        {
            if (it->first == new_name)
                return false;
            label_it = it;
        }
        else if (it->first == new_name)
            return false;
    }

    if (label_it == m_labels.end())
    {
        LogInternalLogic("Can't rename label " << label_id << ": id not found");
        return false;
    }

    Log("Label renamed from '" << label_it->first << "' to '" << new_name << "'");
    label_it->first = new_name;
    m_labels.sort([](const std::pair<std::string, uint32_t>& l, const std::pair<std::string, uint32_t>& r)
        { return tools::str::to_lower_ascii(l.first) < tools::str::to_lower_ascii(r.first); });

    save_labels();
    return true;
}

void NodesManager::remove_label(uint32_t label_id)
{
    bool label_found = false;
    for (auto it = m_labels.begin(); it != m_labels.end(); ++it)
    {
        if (it->second == label_id)
        {
            Log("Label " << it->first << " renamed");
            m_labels.erase(it);
            label_found = true;
            break;
        }
    }

    if (!label_found)
    {
        LogInternalLogic("Can't remove label " << label_id << ": id not found");
        return;
    }

    m_nodes_in_label.erase(label_id);
    save_labels();
}

void NodesManager::set_node_labels(data::NodeItem* node_item, std::set<uint32_t> labels)
{
    bool changed = false;
    for (auto& [label_id, nodes] : m_nodes_in_label)
    {
        const auto it = std::find(nodes.begin(), nodes.end(), node_item);
        if (it == nodes.end() && labels.count(label_id) > 0)
        {
            nodes.push_back(node_item);
            sort_nodes(nodes);
            changed = true;
        }
        else if (it != nodes.end() && labels.count(label_id) == 0)
        {
            nodes.erase(it);
            changed = true;
        }
    }

    if (changed)
        save_labels();
}

void NodesManager::save_labels() const
{
    nlohmann::json j;
    for (auto& [label, id] : m_labels)
    {        
        nlohmann::json jlabel;
        jlabel["name"] = label;
        jlabel["id"] = id;

        auto it = m_nodes_in_label.find(id);
        if (it != m_nodes_in_label.end())
        {
            std::vector<hid> node_ids;
            node_ids.reserve(it->second.size());
            for (auto node_obj : it->second)
            {
                node_ids.push_back(node_obj->node->get_id());
            }
            jlabel["nodes"] = std::move(node_ids);
        }
        else
        {
            jlabel["nodes"] = std::vector<hid>{};
        }

        j.push_back(std::move(jlabel));
    }

    g::pOptsManager->nodes_labels_save(base::HashID::create_id(DlgObjID[cast::to_szt(DlgObj::PanelNodes)]), j.dump());
}

const std::list<data::NodeItem*>& NodesManager::get_nodes_in_label(uint32_t label_id) const
{
    auto it = m_nodes_in_label.find(label_id);
    if (it == m_nodes_in_label.end())
    {
        static std::list<data::NodeItem*> empty;
        return empty;
    }
    return it->second;
}

bool NodesManager::get_is_node_in_label(data::NodeItem* node_item, uint32_t label_id) const
{
    auto& nodes = get_nodes_in_label(label_id);
    return std::find(nodes.begin(), nodes.end(), node_item) != nodes.end();
}

static const char* find_first_sortable(const char* str)   // ignore non-ascii like smiles and icons
{
    while (*str && cast::to_uchar(*str) > 127)
        str++;
    while (*str && ' ' == *str)
        str++;
    return str;
};

static bool sort_callback(const data::NodeItem* lhs, const data::NodeItem* rhs)
{
    if (!lhs->is_node && rhs->is_node)
        return true;
    if (lhs->is_node && !rhs->is_node)
        return false;

    const char* lhs_sort = find_first_sortable(lhs->name_or_category.c_str());
    const char* rhs_sort = find_first_sortable(rhs->name_or_category.c_str());

    return tools::str::to_lower_ascii(lhs_sort) < tools::str::to_lower_ascii(rhs_sort);
};

void NodesManager::sort_nodes(std::vector<data::NodeItem*>& nodes)
{
    std::sort(nodes.begin(), nodes.end(), sort_callback);
}

void NodesManager::sort_nodes(std::list<data::NodeItem*>& nodes)
{
    nodes.sort(sort_callback);
}

} // namespace mngr
