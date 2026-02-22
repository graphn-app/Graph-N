
#pragma once

namespace data
{
struct Node;
struct Pin;
class NodeInstance;
class NodeInstanceRo;
class PinInstance;
class Link;

//TODO - invalidate objects ?
class Workflow final
{
public:
    Workflow();
    Workflow(const std::string& name);
    Workflow(WfData&& data);
    ~Workflow() /*override*/;

    void init();
    void stop_all_threads();

    inline const std::string& get_uuid() const { return m_wf_data.uuid; }
    inline wid get_id() const { return m_wf_data.id; }

    WfData& get_data() { return m_wf_data; }
    const WfData& get_data() const { return m_wf_data; }

    NodeInstance* add_group(const ImRect& rc, const std::string& name = "");
    NodeInstance* add_node(Node* node, const ImRect& rc = ImRect(0, 0, 0, 0), const std::string& select = "");
    NodeInstanceRo* add_node_ro(Node* node);
    PinInstance* add_pin(NodeInstance* node, Pin* pin);
    PinInstance* add_pin(NodeInstanceRo* node, Pin* pin);
    Link* add_link();

    NodeInstance* get_node(iid id) const;
    PinInstance* get_pin(iid id) const;

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
    std::vector<std::unique_ptr<NodeInstance>> m_nodes;
    std::vector<std::unique_ptr<NodeInstanceRo>> m_nodes_ro;
    std::map<iid, std::unique_ptr<PinInstance>> m_pins;
    std::vector<std::unique_ptr<Link>> m_links;
    //TODO
    //std::set<hid> m_invalid_items;
    bool m_invalidation_state = false;
};

} // namespace data
