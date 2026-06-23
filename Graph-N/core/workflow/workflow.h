
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
    friend class WorkflowSerializerToBlob;
    friend class WorkflowSerializerToJson;

public:
    Workflow();
    Workflow(const char* uuid, const char* name);
    Workflow(mngr::WfData&& data);
    ~Workflow();

    void init();
    void stop_all_threads();

    inline const std::string& get_uuid() const { return m_wf_data.uuid; }
    inline wid get_id() const { return m_wf_data.id; }

    inline mngr::WfData& get_data() { return m_wf_data; }
    inline const mngr::WfData& get_data() const { return m_wf_data; }

    inline WorkflowAction* action() const { return m_action.get(); }

    NodeInstance* add_group(const ImRect& rc, const std::string& name = "");
    NodeInstance* add_node(Node* node, const ImRect& rc = ImRect(0, 0, 0, 0), const std::string& select = "");
    NodeInstance* add_node(const NodeInstance* copy_from_source, bool copy_id);
    NodeInstance* add_blank_node();
    NodeInstanceRo* add_node_ro(Node* node);
    PinInstance* add_pin(NodeInstance* node, Pin* pin, iid instance_id = 0);
    PinInstance* add_pin(NodeInstanceRo* node, Pin* pin);
    Link* add_link(iid instance_id = 0);

    NodeInstance* get_node_by_instance(iid id) const;
    PinInstance* get_pin_by_instance(iid id) const;

    NodeInstance* get_node_by_hash(hid id) const;
    PinInstance* get_pin_by_hash(iid node_iid, hid pin_hid) const;

    Link* get_link_by_instance(iid id) const;
    std::list<Link*> get_links_by_pin(const PinInstance* pin) const;

    inline const std::vector<std::unique_ptr<NodeInstance>>& get_nodes() { return m_nodes; }
    inline const std::vector<std::unique_ptr<PinInstance>>& get_pins() { return m_pins; }
    inline const std::vector<std::unique_ptr<Link>>& get_links() { return m_links; }

    inline bool get_is_invalidation_state() const { return m_invalidation_state; }
    void invalidate_current();

    void remove_node(const NodeInstance* node);
    void remove_node(const NodeInstanceRo* node);
    void remove_link(const Link* link);

    void reset();
    void clear();

    void increment(Workflow* copy_to);
    std::string collect_search_data() const;

private:
    void remove_pin(const PinInstance* pin);
    iid get_new_child_id();

    // ComfyUiObjectValidityListener
    void subscribe_comfyui_validity_state();
    void invalidate_comfyui_objects() override;
    void resolve_comfyui_objects(listener::ComfyUiObjectValidityListener::Step step) override;

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
