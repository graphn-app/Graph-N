/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once
#include "core/listeners.h"

namespace mngr
{
class HistoryManager;
}

namespace data
{
struct Node;
struct Pin;
class NodeInstance;
class NodeInstanceRo;
class PinInstance;
class Link;
class WorkflowAction;

class Workflow final : public listener::ComfyUiObjectValidityListener
{
public:
    Workflow();
    Workflow(const char* uuid, const char* name);
    Workflow(mngr::WfData&& data);
    ~Workflow() noexcept override;

    void init();

    const std::string& get_uuid() const noexcept { return m_wf_data.uuid; }
    wid get_id() const noexcept { return m_wf_data.id; }

    mngr::WfData& get_data() noexcept { return m_wf_data; }
    const mngr::WfData& get_data() const noexcept { return m_wf_data; }

    WorkflowAction* action() const noexcept { return m_action.get(); }

    NodeInstance* add_group(const ImRect& rc, const std::string& name = "", iid node_id = 0);
    NodeInstance* add_node(Node* node, const ImRect& rc = ImRect(0, 0, 0, 0), const std::string& select = "", iid node_id = 0, iid node_id2 = 0);
    NodeInstance* add_node(const NodeInstance* copy_from_source, bool copy_id);
    NodeInstance* add_group_node();
    NodeInstanceRo* add_node_ro(Node* node);
    PinInstance* add_pin(NodeInstance* node, Pin* pin, iid instance_id = 0);
    PinInstance* add_pin(NodeInstanceRo* node, Pin* pin);
    Link* add_link(iid instance_id = 0);

    NodeInstance* get_node_by_instance(iid id) const noexcept;
    PinInstance* get_pin_by_instance(iid id) const noexcept;

    NodeInstance* get_node_by_hash(hid hash) const noexcept;
    PinInstance* get_pin_by_hash(iid node_iid, hid pin_hash) const noexcept;

    Link* get_link_by_instance(iid id) const noexcept;
    std::list<Link*> get_links_by_pin(const PinInstance* pin) const;

    std::vector<NodeInstance*> get_nodes_no_reroutes_r() const;
    const std::vector<std::unique_ptr<NodeInstance>>& get_nodes_all() const noexcept { return m_nodes; }

    std::vector<NodeInstanceRo*> get_nodes_ro() const;

    std::vector<Link*> get_links_no_reroutes() const;
    const std::vector<std::unique_ptr<Link>>& get_links_all() const noexcept { return m_links; }

    bool get_is_invalidation_state() const noexcept { return m_invalidation_state; }
    void invalidate_current() noexcept;

    void remove_node(NodeInstance* node);
    void remove_node(NodeInstanceRo* node);
    void remove_link(const Link* link);
    void remove_pin(const PinInstance* pin, bool cleanup_ed = true);

    void unlink_node(const NodeInstance* node);

    void register_bookmarks();
    void register_bookmark(const NodeInstance* node);
    void unregister_bookmark(const NodeInstance* node);
    void handle_bookmark(ImGuiKey key) noexcept;
    const std::map<ImGuiKey, ui::Bookmark>& get_bookmarks() const noexcept { return m_bookmarks; }

    void bundle_reroutes(std::set<data::NodeInstance*> nodes);
    void separate_reroutes(std::set<data::NodeInstance*> nodes);
    void insert_reroute(data::Link* link, const ImVec2& pos);
    void collapse_reroute(data::NodeInstance* node);
    void split_reroute_to_parts(data::NodeInstance* node);
    void merge_reroute_parts(data::NodeInstance* node, bool to_left);

    void handle_rgthree_switchers(const data::PinInstance* pin);

    void bypass_node(data::NodeInstance* node);

    void prepare_for_api();

    void reset_wf() noexcept;
    void clear_wf();

    void increment(const Workflow* copy_to);
    void apply_pre_flight_mutations();
    void apply_runtime_updates(iid node_id, NodeInstFlags mode);
    void apply_runtime_updates(iid node_id, const std::string& pin_name, PinType pin_type, Primitive value);
    void apply_runtime_updates(uint32_t link_id, std::string value);

    std::string collect_search_data() const;

    iid get_new_child_id() noexcept { return ++m_last_child_id; }
    iid get_last_child_id() const noexcept { return m_last_child_id; }
    void set_last_child_id(iid id) noexcept { m_last_child_id = std::max(id, m_last_child_id); }

    data::NodeInstance* ed_get_node(uint64_t id) const { return ed_get(id, m_ed_objects_nodes); }
    data::PinInstance* ed_get_pin(uint64_t id) const { return ed_get(id, m_ed_objects_pins); }
    data::Link* ed_get_link(uint64_t id) const { return ed_get(id, m_ed_objects_links); }

#ifdef _DEBUG
    void test_duplicates() const;
    const std::vector<std::unique_ptr<PinInstance>>& get_all_pins() const noexcept { return m_pins; }
#else
    void test_duplicates() const {}
#endif

private:
    // ComfyUiObjectValidityListener
    void subscribe_comfyui_validity_state();
    void invalidate_comfyui_objects() override;
    void resolve_comfyui_objects(listener::ComfyUiObjectValidityListener::Step step) override;

    std::unique_ptr<NodeInstance> create_node(data::Node* node, iid node_id, iid node_id2) const;
    std::unique_ptr<NodeInstanceRo> create_node_ro(data::Node* node, iid node_id) const;

    std::set<data::NodeInstance*> filter_reroutes(const std::set<data::NodeInstance*>& nodes) const;
    data::NodeInstance* filter_reroute(data::NodeInstance* node) const noexcept;

    void ed_create(data::NodeInstance* node);
    void ed_create(data::PinInstance* pin);
    void ed_create(data::Link* link);

    void ed_delete(const std::unordered_set<uint64_t>& ids);

    template <typename T>
    T* ed_get(uint64_t id, const std::vector<std::pair<uint64_t, T*>>& map) const
    {
        if (0 == id) return nullptr;
        const auto it = std::ranges::lower_bound(map, id, {}, &std::pair<uint64_t, T*>::first);
        if (it != map.end() && it->first == id) return it->second;
        return nullptr;
    }

private:
    mngr::WfData m_wf_data;
    iid m_last_child_id = 0;
    std::unique_ptr<WorkflowAction> m_action;
    std::vector<std::unique_ptr<NodeInstance>> m_nodes;
    std::vector<std::unique_ptr<NodeInstanceRo>> m_nodes_ro;
    std::vector<std::unique_ptr<PinInstance>> m_pins;
    std::vector<std::unique_ptr<Link>> m_links;

    std::map<ImGuiKey, ui::Bookmark> m_bookmarks; // std::map (not unordered_map) required for sorting purposes

    // maps EdType.id to object
    // sorted vector is still faster than a map; 1000 objects will fit into the L1/L2 cache
    std::vector<std::pair<uint64_t, data::NodeInstance*>> m_ed_objects_nodes;
    std::vector<std::pair<uint64_t, data::PinInstance*>> m_ed_objects_pins;
    std::vector<std::pair<uint64_t, data::Link*>> m_ed_objects_links;

    bool m_invalidation_state = false;
};

} // namespace data
