
#pragma once

namespace blob
{
class Blob;
}

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

//TODO - invalidate objects ?
class Workflow final
{
public:
    Workflow();
    Workflow(WfData&& data);
    ~Workflow();

    void init();
    void stop_all_threads();

    inline const std::string& get_uuid() const { return m_wf_data.uuid; }
    inline wid get_id() const { return m_wf_data.id; }

    inline WfData& get_data() { return m_wf_data; }
    inline const WfData& get_data() const { return m_wf_data; }

    inline WorkflowAction* action() const { return m_action.get(); }

    NodeInstance* add_group(const ImRect& rc, const std::string& name = "");
    NodeInstance* add_node(Node* node, const ImRect& rc = ImRect(0, 0, 0, 0), const std::string& select = "");
    NodeInstance* add_node(const NodeInstance* copy_from_source);
    NodeInstance* add_node(const blob::Blob& blob);
    NodeInstanceRo* add_node_ro(Node* node);
    PinInstance* add_pin(NodeInstance* node, Pin* pin, iid instance_id = 0);
    PinInstance* add_pin(NodeInstanceRo* node, Pin* pin);
    Link* add_link(iid instance_id = 0);

    NodeInstance* get_node_by_instance(iid id) const;
    PinInstance* get_pin_by_instance(iid id) const;

    NodeInstance* get_node_by_hash(hid id) const;
    PinInstance* get_pin_by_hash(hid id) const;

    Link* get_link_by_instance(iid id) const;
    std::list<Link*> get_links_by_pin(const PinInstance* pin) const;

    inline const std::vector<std::unique_ptr<NodeInstance>>& get_nodes() { return m_nodes; }
    inline const std::vector<std::unique_ptr<PinInstance>>& get_pins() { return m_pins; }
    inline const std::vector<std::unique_ptr<Link>>& get_links() { return m_links; }

    void remove_node(const NodeInstance* node);
    void remove_node(const NodeInstanceRo* node);
    void remove_link(const Link* link);

private:
    void remove_pin(const PinInstance* pin);
    iid get_new_child_id();

private:
    WfData m_wf_data;
    iid m_last_child_id = 0;
    std::unique_ptr<WorkflowAction> m_action;
    std::vector<std::unique_ptr<NodeInstance>> m_nodes;
    std::vector<std::unique_ptr<NodeInstanceRo>> m_nodes_ro;
    std::vector<std::unique_ptr<PinInstance>> m_pins;
    std::vector<std::unique_ptr<Link>> m_links;
    //TODO
    //std::set<hid> m_invalid_items;
    bool m_invalidation_state = false;
};

} // namespace data
