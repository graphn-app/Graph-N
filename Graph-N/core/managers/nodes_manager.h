
#pragma once
#include "core/managers/manager.h"
#include "core/data/node.h"

namespace data
{
struct Nodes final
{
    NO_COPY_MOVE_CTORS(Nodes);

    Node* node = nullptr;
    bool is_node = false;

    std::string name_or_category;
    size_t name_or_category_len;
    std::string search;

    std::vector<Nodes*> children;

    uint32_t nodes_inside = 0;
    uint32_t nodes_inside_unfiltered = 0;
    bool filtered = false;
};
}

namespace mngr
{

class NodesManager final : public Manager
{
    friend class NodesParser;

public:
    NodesManager() = default;
    ~NodesManager() override = default;

    void free() override;

    const std::vector<data::Nodes*>& get_models_tree() const { return m_nodes_tree; }

    const std::vector<data::Nodes*>& get_nodes_tree() const { return m_nodes_tree; }
    const std::vector<data::Nodes*>& get_nodes_plain() const { return m_nodes_plain; }
    size_t get_node_items_total() const { return m_node_holders.size(); }

    void calc_number_inside(const std::string& filter);

    data::Node* find_node(hid id) const;		    // returns nullptr if not found
    data::Pin* find_pin(hid id) const;			    // returns nullptr if not found

private:
    void init_from_copy(NodesManager& copy);

private:
    std::unordered_map<hid, std::unique_ptr<data::Node>> m_nodes;
    std::unordered_map<hid, std::unique_ptr<data::Pin>> m_pins;
    std::list<std::unique_ptr<data::Nodes>> m_node_holders;

    std::vector<data::Nodes*> m_nodes_tree;			// tree view
    std::vector<data::Nodes*> m_nodes_plain;		// plain view: sorted by display name
};

} // namespace mngr
