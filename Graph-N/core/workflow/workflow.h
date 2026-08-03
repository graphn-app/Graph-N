
#pragma once
#include "core/base/structs.h"
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

class Workflow final :
    public listener::ComfyUiObjectValidityListener
{
public:
    Workflow();
    Workflow(const char* uuid, const char* name);
    Workflow(mngr::WfData&& data);
    ~Workflow() noexcept;

    void init();

    const std::string& get_uuid() const noexcept { return m_wf_data.uuid; }
    wid get_id() const noexcept { return m_wf_data.id; }

    mngr::WfData& get_data() noexcept { return m_wf_data; }
    const mngr::WfData& get_data() const noexcept { return m_wf_data; }

    WorkflowAction* action() const noexcept { return m_action.get(); }

    NodeInstance* add_group(const ImRect& rc, const std::string& name = "");
    NodeInstance* add_node(Node* node, const ImRect& rc = ImRect(0, 0, 0, 0), const std::string& select = "", iid node_id = 0);
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

    const std::vector<std::unique_ptr<NodeInstance>>& get_nodes() const noexcept { return m_nodes; }
    const std::vector<std::unique_ptr<PinInstance>>& get_pins() const noexcept { return m_pins; }
    const std::vector<std::unique_ptr<Link>>& get_links() const noexcept { return m_links; }

    bool get_is_invalidation_state() const noexcept { return m_invalidation_state; }
    void invalidate_current() noexcept;

    void remove_node(NodeInstance* node);
    void remove_node(NodeInstanceRo* node);
    void remove_link(const Link* link);
    void remove_pin(const PinInstance* pin) noexcept;

    void merge_reroutes(const std::set<data::NodeInstance*>& nodes);
    void split_reroutes(const std::set<data::NodeInstance*>& nodes);
    void insert_reroute(data::Link* link, const ImVec2& pos);
    void collapse_reroute(data::NodeInstance* node);

    void bypass_node(data::NodeInstance* node);

    void prepare_for_api();

    void reset() noexcept;
    void clear() noexcept;

    void increment(const Workflow* copy_to);
    std::string collect_search_data() const;

    iid get_new_child_id() noexcept { return ++m_last_child_id; }
    iid get_last_child_id() const noexcept { return m_last_child_id; }
    void set_last_child_id(iid id) noexcept { m_last_child_id = id; }

private:
    // ComfyUiObjectValidityListener
    void subscribe_comfyui_validity_state();
    void invalidate_comfyui_objects() override;
    void resolve_comfyui_objects(listener::ComfyUiObjectValidityListener::Step step) override;

    std::unique_ptr<NodeInstance> create_node(data::Node* node, iid node_id) const;
    std::unique_ptr<NodeInstanceRo> create_node_ro(data::Node* node, iid node_id) const;

private:
    mngr::WfData m_wf_data;
    iid m_last_child_id = 0;
    std::unique_ptr<WorkflowAction> m_action;
    std::vector<std::unique_ptr<NodeInstance>> m_nodes;
    std::vector<std::unique_ptr<NodeInstanceRo>> m_nodes_ro;
    std::vector<std::unique_ptr<PinInstance>> m_pins;
    std::vector<std::unique_ptr<Link>> m_links;

    bool m_invalidation_state = false;
};

} // namespace data
