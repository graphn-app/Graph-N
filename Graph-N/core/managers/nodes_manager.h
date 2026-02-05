
#pragma once
#include "core/managers/manager.h"
#include "core/data/node.h"

namespace data
{

// TODO: rename Nodes to Holder
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

    const std::vector<data::Nodes*>& get_nodes_tree() const { return m_nodes_tree; }
    const std::vector<data::Nodes*>& get_nodes_plain() const { return m_nodes_plain; }
    size_t get_node_items_total() const { return m_node_holders.size(); }

    void calc_number_inside(const std::string& filter);

    data::Node* find_node(hid id) const;		    // returns nullptr if not found
    data::Pin* find_pin(hid id) const;			    // returns nullptr if not found

    inline const std::list<std::pair<std::string, uint32_t>>& get_labels() const { return m_labels; }
    const std::list<data::Nodes*>& get_nodes_in_label(uint32_t label_id) const;
    bool get_is_node_in_label(data::Nodes* node, uint32_t label_id) const;

    std::string create_label();
    bool rename_label(uint32_t label_id, const std::string& new_name);
    void remove_label(uint32_t label_id);

    void set_node_labels(data::Nodes* node, std::set<uint32_t> labels);

private:
    void init_from_copy(NodesManager& copy);
    void init_labels();
    void save_labels() const;

    static void sort_nodes(std::vector<data::Nodes*>& nodes);
    static void sort_nodes(std::list<data::Nodes*>& nodes);

private:
    std::unordered_map<hid, std::unique_ptr<data::Node>> m_nodes;
    std::unordered_map<hid, std::unique_ptr<data::Pin>> m_pins;
    std::list<std::unique_ptr<data::Nodes>> m_node_holders;

    std::vector<data::Nodes*> m_nodes_tree;			// tree view
    std::vector<data::Nodes*> m_nodes_plain;		// plain view: sorted by display name

    std::list<std::pair<std::string, uint32_t>> m_labels;
    uint32_t m_last_label_id = 0;
    std::map<uint32_t, std::list<data::Nodes*>> m_nodes_in_label;
};

} // namespace mngr
