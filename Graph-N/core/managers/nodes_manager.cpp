
#include "core/managers/nodes_manager.h"
#include "core/managers/settings_manager.h"

namespace mngr
{

void NodesManager::free()
{
    m_nodes.clear(); m_nodes.rehash(0);
    m_pins.clear(); m_pins.rehash(0);
    m_node_items.clear();
    m_nodes_tree.clear(); m_nodes_tree.shrink_to_fit();
    m_nodes_plain.clear(); m_nodes_plain.shrink_to_fit();
    m_labels.clear();
    m_nodes_in_label.clear();
}

void NodesManager::init_from_copy(NodesManager& copy)
{
    free();
    m_nodes = std::move(copy.m_nodes);
    m_pins = std::move(copy.m_pins);
    m_node_items = std::move(copy.m_node_items);
    m_nodes_tree = std::move(copy.m_nodes_tree);
    m_nodes_plain = std::move(copy.m_nodes_plain);
    m_labels = std::move(copy.m_labels);
    m_last_label_id = copy.m_last_label_id;
    m_nodes_in_label = std::move(copy.m_nodes_in_label);
    calc_number_inside("", {}, {});
}

void NodesManager::calc_number_inside(const std::string& filter, const std::list<PinType>& in_types, const std::list<PinType>& out_types)
{
    const auto pin_filter = [](const std::vector<data::Pin*>& pins, const std::list<PinType>& type_filter)
    {
        for (auto pin : pins)
        {
            if (PinType::ANY == pin->def_state.type && type_filter.front() == PinType::ANY)
                continue;
            if (type_filter.front() == PinType::ANY)
                return true;
            const auto it = std::ranges::find_if(type_filter, [type = pin->def_state.type](PinType filter) { return filter == type; });
            if (it != type_filter.end())
                return true;
        }
        return false;
    };

    const auto pins_filter = [&in_types, &out_types, &pin_filter](data::NodeItem* node_item)
    {
        if (in_types.empty() && out_types.empty())
            return true;

        if (!in_types.empty())
        {
            if (pin_filter(node_item->node->input, in_types))
                return true;
        }
        if (!out_types.empty())
        {
            if (pin_filter(node_item->node->output, out_types))
                return true;
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
}

data::Node* NodesManager::find_node(hid hash) const
{
    const auto it = m_nodes.find(hash);
    if (it == m_nodes.end())
        return nullptr;
    return it->second.get();
}

data::Pin* NodesManager::find_pin(hid hash) const
{
    const auto it = m_pins.find(hash);
    if (it == m_pins.end())
        return nullptr;
    return it->second.get();
}

data::Node* NodesManager::make_dummy_node(const std::string& node_class)
{
    auto node = std::make_unique<data::Node>(node_class);
    node->name = node_class;
    node->node_class = node_class;
    node->is_dummy = true;

    auto ptr = node.get();
    m_dummy_nodes.push_back(std::move(node));
    Log("Dummy node " << node_class << "/" << ptr->get_hash() << " created");
    return ptr;
}

void NodesManager::remove_dummy_node(data::Node* node)
{
    const auto remove_pin = [this](data::Pin* pin)
    {
        for (auto it = m_dummy_pins.begin(); it != m_dummy_pins.end(); ++it)
        {
            auto& dummy = *it;
            if (dummy.get() == pin)
            {
                LogS("Dummy pin removed. i:" << pin->input << ", p:" << pin->def_state.type_str);
                m_dummy_pins.erase(it);
                break;
            }
        }
    };

    for (auto pin : node->input)
    {
        remove_pin(pin);
    }
    for (auto pin : node->output)
    {
        remove_pin(pin);
    }

    for (auto it = m_dummy_nodes.begin(); it != m_dummy_nodes.end(); ++it)
    {
        auto& dummy = *it;
        if (dummy.get() == node)
        {
            LogS("Dummy node removed. n:" << node->node_class);
            m_dummy_nodes.erase(it);
            break;
        }
    }
}

data::Pin* NodesManager::make_dummy_pin(data::Node* parent, const std::string& type, const std::string& name, int32_t out_index)
{
    std::unique_ptr<data::Pin> pin;
    if (out_index >= 0)
    {
        pin = std::make_unique<data::Pin>(parent, out_index);
        parent->output.push_back(pin.get());
    }
    else
    {
        pin = std::make_unique<data::Pin>(parent, name, true, false, false);
        parent->input.push_back(pin.get());
    }
    pin->initialize(type);
    pin->finalize();
    pin->def_state.name = name;
    if (pin->output || pin->def_state.type != PinType::STRING)
        pin->def_state.w_name = ImGui::CalcTextSize(name.c_str(), name.c_str() + name.size()).x;

    auto ptr = pin.get();
    m_dummy_pins.push_back(std::move(pin));
    Log("Dummy pin " << parent->name << "/" << name << "/" << ptr->get_hash() << "/" << out_index << " created");
    return ptr;
}

std::unique_ptr<data::Pin> NodesManager::make_pin_ptr(data::Node* parent, const std::string& type, const std::string& name, int32_t out_index)
{
    std::unique_ptr<data::Pin> pin;
    if (out_index >= 0)
    {
        pin = std::make_unique<data::Pin>(parent, out_index);
        parent->output.push_back(pin.get());
    }
    else
    {
        pin = std::make_unique<data::Pin>(parent, name, true, false, false);
        parent->input.push_back(pin.get());
    }
    pin->parent_node = parent;
    pin->initialize(type);
    pin->finalize();
    pin->def_state.name = name;
    if (pin->output || pin->def_state.type != PinType::STRING)
        pin->def_state.w_name = ImGui::CalcTextSize(name.c_str(), name.c_str() + name.size()).x;    
    return pin;
}

data::Pin* NodesManager::make_pin(data::Node* parent, const std::string& type, const std::string& name, int32_t out_index)
{
    auto pin = make_pin_ptr(parent, type, name, out_index);
    auto ptr = pin.get();
    const auto hash = pin->get_hash();
    LogRAssert(m_pins.try_emplace(hash, std::move(pin)).second, "Pin duplicate. n:" << parent->name << ", p:" << pin->def_state.type_str, nullptr);
    Log("Pin " << parent->name << "/" << name << "/" << ptr->get_hash() << " created");
    return ptr;
}

void NodesManager::remove_pin(data::Node* parent, const data::Pin* pin) noexcept
{
    LogRAssert(parent, "Zero ptr", );
    LogRAssert(pin, "Zero ptr", );

    const auto hash = pin->get_hash();
    const auto name = pin->def_state.name;

    bool found = false;
    auto input_it = parent->input.begin();
    while (input_it != parent->input.end())
    {
        auto input = *input_it;
        if (input == pin)
        {
            parent->input.erase(input_it);
            found = true;
            break;
        }
        ++input_it;
    }

    if (!found)
    {
        for (auto output_it = parent->output.begin(); output_it != parent->output.end(); ++output_it)
        {
            auto output = *output_it;
            if (output == pin)
            {
                parent->output.erase(output_it);
                found = true;
                break;
            }
        }
    }

    if (found)
        m_pins.erase(hash);
    else
        LogError("Pin not found. p:" << pin->def_state.type_str);
}

void NodesManager::init_labels()
{
    m_labels.clear();
    m_nodes_in_label.clear();
    m_last_label_id = 0;

    auto labels = g::pOptsManager->nodes_labels_load(base::HashID::make_hash(DlgObjID[cast::to_szt(DlgObj::PanelNodes)]));
    if (labels.empty())
        return;

    Log("Node labels loaded: " << labels);
    try
    {
        const auto j = u_json::parse(labels);

        for (auto& obj : j)
        {
            std::string name = obj["name"];
            uint32_t id = obj["id"];
            m_labels.push_back({ name, id });

            if (id > m_last_label_id)
                m_last_label_id = id;

            auto& nodes_in_label = m_nodes_in_label[id];
            std::vector<hid> node_hashes = obj["nodes"];
            for (auto node_hash : node_hashes)
            {
                for (auto node_item : m_nodes_plain)
                {
                    if (node_item->is_node && node_item->node->get_hash() == node_hash)
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
        LogExcep("Failed to parse labels (" << labels << "). Err: " << ex.what());
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

    LogRAssert(label_it != m_labels.end(), "Can't rename label " << label_id << ": id not found", false);

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

    LogRAssert(label_found, "Can't remove label " << label_id << ": id not found", );

    m_nodes_in_label.erase(label_id);
    save_labels();
}

void NodesManager::set_node_labels(data::NodeItem* node_item, std::set<uint32_t> labels)
{
    bool changed = false;
    for (auto& [label_id, nodes] : m_nodes_in_label)
    {
        const auto it = std::ranges::find(nodes, node_item);
        if (it == nodes.end() && labels.contains(label_id))
        {
            nodes.push_back(node_item);
            sort_nodes(nodes);
            changed = true;
        }
        else if (it != nodes.end() && !labels.contains(label_id))
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
    o_json j;
    for (auto& [label, id] : m_labels)
    {        
        o_json jlabel;
        jlabel["name"] = label;
        jlabel["id"] = id;

        auto it = m_nodes_in_label.find(id);
        if (it != m_nodes_in_label.end())
        {
            std::vector<hid> node_hashes;
            node_hashes.reserve(it->second.size());
            for (auto node_obj : it->second)
            {
                node_hashes.push_back(node_obj->node->get_hash());
            }
            jlabel["nodes"] = std::move(node_hashes);
        }
        else
        {
            jlabel["nodes"] = std::vector<hid>{};
        }

        j.push_back(std::move(jlabel));
    }

    g::pOptsManager->nodes_labels_save(base::HashID::make_hash(DlgObjID[cast::to_szt(DlgObj::PanelNodes)]), j.dump());
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
    return std::ranges::find(nodes, node_item) != nodes.end();
}

namespace
{
const char* find_first_sortable(const char* str) noexcept   // ignore non-ascii like smiles and icons
{
    while (*str && cast::to_uchar(*str) > 127)
        str++;
    while (*str && ' ' == *str)
        str++;
    return str;
}

bool sort_callback(const data::NodeItem* lhs, const data::NodeItem* rhs)
{
    if (!lhs->is_node && rhs->is_node)
        return true;
    if (lhs->is_node && !rhs->is_node)
        return false;

    const char* lhs_sort = find_first_sortable(lhs->name_or_category.c_str());
    const char* rhs_sort = find_first_sortable(rhs->name_or_category.c_str());

    return tools::str::to_lower_ascii(lhs_sort) < tools::str::to_lower_ascii(rhs_sort);
}
} // namespace

void NodesManager::sort_nodes(std::vector<data::NodeItem*>& nodes)
{
    std::ranges::sort(nodes, sort_callback);
}

void NodesManager::sort_nodes(std::list<data::NodeItem*>& nodes)
{
    nodes.sort(sort_callback);
}

} // namespace mngr
