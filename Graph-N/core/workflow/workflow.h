
#pragma once
#include "core/managers/comfyui_object_validity_listener.h"

namespace data
{
struct Node;
struct Pin;
class NodeInstance;
class PinInstance;
class Link;

class Workflow final : public mngr::ComfyUiObjectValidityListener
{
public:
    Workflow();
    Workflow(WfData&& data);
    ~Workflow() override;

    void init();
    void stop_all_threads();

    void save();

    inline const std::string& get_uuid() const { return m_wf_data.uuid; }
    inline wid get_uuid_hash() const { return m_id_crc32; }

    WfData& get_data() { return m_wf_data; }
    const WfData& get_data() const { return m_wf_data; }

    NodeInstance* add_node(Node* node);
    PinInstance* add_pin(NodeInstance* node, Pin* pin);
    Link* add_link();

    inline const std::vector<std::unique_ptr<Link>>& get_links() { return m_links; }

    void remove_node(const NodeInstance* node);
    void remove_link(const Link* link);

    void mark_invalid(hid id);

    // ComfyUiObjectValidityListener
    void finalize_resolve_comfyui_objects() override;

private:
    static uint32_t get_wf_id_by_child_gid(gid child_id);       // returns WF' crc32
    static uint32_t get_child_id_by_child_gid(gid child_id);    // returns item's json id

    void remove_pin(const PinInstance* pin);
    std::pair<gid, iid> get_new_child_id();

private:
    WfData m_wf_data;
    uint32_t m_id_crc32 = 0;
    std::atomic<lid> m_child_id = 0;    // Unique ID across current graph, starts from 1
    std::vector<std::unique_ptr<NodeInstance>> m_nodes;
    std::vector<std::unique_ptr<PinInstance>> m_pins;
    std::vector<std::unique_ptr<Link>> m_links;
    std::set<hid> m_invalid_items;
    bool m_invalidation_state = false;
};

} // namespace data
