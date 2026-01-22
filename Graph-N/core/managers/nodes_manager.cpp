
#include "core/managers/nodes_manager.h"

namespace mngr
{

void NodesManager::free()
{
    m_nodes.clear();
    m_pins.clear();
    m_node_holders.clear();
    m_nodes_tree.clear();
    m_nodes_plain.clear();
}

void NodesManager::init_from_copy(NodesManager& copy)
{
    LogF;
    free();
    m_nodes = std::move(copy.m_nodes);
    m_pins = std::move(copy.m_pins);
    m_node_holders = std::move(copy.m_node_holders);
    m_nodes_tree = std::move(copy.m_nodes_tree);
    m_nodes_plain = std::move(copy.m_nodes_plain);
    calc_number_inside("");
}

void NodesManager::calc_number_inside(const std::string& filter)
{
    std::function<void(std::vector<data::Nodes*>&, uint32_t*, bool)> calculator;
    calculator = [&calculator, &filter](std::vector<data::Nodes*>& nodes, uint32_t* count, bool force_unfilter)
    {
        for (auto node : nodes)
        {
            node->nodes_inside = node->nodes_inside_unfiltered = 0;
            if (!node->children.empty())
            {
                const bool category_unfiltered = force_unfilter || (!filter.empty() && node->search.find(filter) != std::string::npos);
                node->nodes_inside_unfiltered = 0;
                calculator(node->children, filter.empty() ? &node->nodes_inside : &node->nodes_inside_unfiltered, category_unfiltered);
                node->name_or_category = node->name_or_category.substr(0, node->name_or_category_len) +
                    " [" + std::to_string(filter.empty() ? node->nodes_inside : node->nodes_inside_unfiltered) + "]";
                if (count != nullptr)
                    *count += filter.empty() ? node->nodes_inside : node->nodes_inside_unfiltered;
            }
            else if (node->is_node)
            {
                if (force_unfilter || filter.empty())
                {
                    node->filtered = false;
                    if (count != nullptr)
                        (*count)++;
                }
                else
                {
                    node->filtered = (node->search.find(filter) == std::string::npos);
                    if (count != nullptr)
                        *count += node->filtered ? 0 : 1;
                }
            }
        }
    };

    uint32_t count = 0;
    calculator(m_nodes_tree, &count, false);
    Log("Filter = (" << filter << "), count=" << count);
}

data::Nodes* NodesManager::find_holder(lid id) const
{
    const auto it = m_node_holders.find(id);
    if (it == m_node_holders.end())
    {
        LogInternalMap("Nodes id=" << id << " not found");
        return nullptr;
    }
    return it->second.get();
}

data::Node* NodesManager::find_node(hid id) const
{
    const auto it = m_nodes.find(id);
    if (it == m_nodes.end())
    {
        LogInternalMap("Node id=" << id << " not found");
        return nullptr;
    }
    return it->second.get();
}

data::Pin* NodesManager::find_pin(hid id) const
{
    const auto it = m_pins.find(id);
    if (it == m_pins.end())
    {
        LogInternalMap("Pin id=" << id << " not found");
        return nullptr;
    }
    return it->second.get();
}

} // namespace mngr
