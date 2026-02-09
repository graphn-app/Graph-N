
#pragma once
#include "core/managers/comfyui_object_validity_listener.h"

namespace data
{
struct Node;
struct Pin;
class NodeInstance;
class PinInstance;
class Link;

//TODO - invalidate objects ?
class Workflow final //: public mngr::ComfyUiObjectValidityListener
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

    NodeInstance* add_node(Node* node, const std::string& select = "");
    PinInstance* add_pin(NodeInstance* node, Pin* pin);
    Link* add_link();

    NodeInstance* get_node(iid id) const;
    PinInstance* get_pin(iid id) const;

    inline const std::vector<std::unique_ptr<Link>>& get_links() { return m_links; }

    void remove_node(const NodeInstance* node);
    void remove_link(const Link* link);

    // ComfyUiObjectValidityListener
    //void finalize_resolve_comfyui_objects() override;

private:
    void remove_pin(const PinInstance* pin);
    iid get_new_child_id();

private:
    WfData m_wf_data;
    iid m_last_child_id = 0;
    std::vector<std::unique_ptr<NodeInstance>> m_nodes;
    std::map<iid, std::unique_ptr<PinInstance>> m_pins;
    std::vector<std::unique_ptr<Link>> m_links;
    //TODO
    //std::set<hid> m_invalid_items;
    bool m_invalidation_state = false;
};

} // namespace data
