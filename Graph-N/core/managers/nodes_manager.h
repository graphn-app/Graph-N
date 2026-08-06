
#pragma once
#include "core/managers/manager.h"
#include "core/data/node.h"

namespace data
{
struct NodeItem final
{
    NO_COPY_MOVE_CTORS(NodeItem);

    Node* node = nullptr;
    bool is_node = false;

    std::string name_or_category;
    size_t name_or_category_len = 0;
    std::string search;

    std::vector<NodeItem*> children;

    uint32_t nodes_inside = 0;
    bool filtered = false;
};
}

namespace mngr
{

class NodesManager final : public Manager
{
    friend class NodesParser;

public:
    NodesManager() noexcept = default;
    ~NodesManager() noexcept override = default;

    void free() override;

    const std::vector<data::NodeItem*>& get_nodes_tree() const noexcept { return m_nodes_tree; }
    const std::vector<data::NodeItem*>& get_nodes_plain() const noexcept { return m_nodes_plain; }
    size_t get_node_items_total() const noexcept { return m_node_items.size(); }

    void calc_number_inside(const std::string& filter, const std::list<PinType>& in_pins, const std::list<PinType>& out_pins);

    data::Node* find_node(hid hash) const;
    data::Pin* find_pin(hid hash) const;

    data::Node* make_dummy_node(const std::string& node_class);
    void remove_dummy_node(data::Node* node);

    std::unique_ptr<data::Pin> make_pin_ptr(data::Node* parent, const std::string& type, const std::string& name, int32_t out_index);
    data::Pin* make_pin(data::Node* parent, const std::string& type, const std::string& name, int32_t out_index);
    data::Pin* make_dummy_pin(data::Node* parent, const std::string& type, const std::string& name, int32_t out_index);

    void remove_pin(data::Node* parent, const data::Pin* pin) noexcept;

    const std::list<std::pair<std::string, uint32_t>>& get_labels() const noexcept { return m_labels; }
    const std::list<data::NodeItem*>& get_nodes_in_label(uint32_t label_id) const;
    bool get_is_node_in_label(data::NodeItem* node_item, uint32_t label_id) const;

    uint32_t create_label();
    bool rename_label(uint32_t label_id, const std::string& new_name);
    void remove_label(uint32_t label_id);

    void set_node_labels(data::NodeItem* node_item, std::set<uint32_t> labels);

private:
    void init_from_copy(NodesManager& copy);
    void init_labels();
    void save_labels() const;

    static void sort_nodes(std::vector<data::NodeItem*>& nodes);
    static void sort_nodes(std::list<data::NodeItem*>& nodes);

private:
    std::unordered_map<hid, std::unique_ptr<data::Node>> m_nodes;
    std::vector<std::unique_ptr<data::Node>> m_dummy_nodes;
    std::unordered_map<hid, std::unique_ptr<data::Pin>> m_pins;
    std::vector<std::unique_ptr<data::Pin>> m_dummy_pins;
    std::list<std::unique_ptr<data::NodeItem>> m_node_items;

    std::vector<data::NodeItem*> m_nodes_tree;			// tree view
    std::vector<data::NodeItem*> m_nodes_plain;		// plain view: sorted by display name

    std::list<std::pair<std::string, uint32_t>> m_labels;
    uint32_t m_last_label_id = 0;
    std::map<uint32_t, std::list<data::NodeItem*>> m_nodes_in_label;
};

} // namespace mngr
