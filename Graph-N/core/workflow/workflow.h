
#pragma once
#include "core/managers/comfyui_object_validity_listener.h"

namespace data
{
struct Node;
struct Pin;
class NodeInstance;
class PinInstance;
class Link;

class Workflow final :
    public base::LocalID,
    public mngr::ComfyUiObjectValidityListener
{
public:
    Workflow();
    ~Workflow() override;

    void init();

    const std::vector<std::unique_ptr<NodeInstance>>& get_nodes() const { return m_nodes; }
    const std::vector<std::unique_ptr<PinInstance>>& get_pins() const { return m_pins; }
    const std::vector<std::unique_ptr<Link>>& get_links() const { return m_links; }

    NodeInstance* add_node(Node* node);
    PinInstance* add_pin(Pin* pin);
    Link* add_link();

    void remove_node(const NodeInstance* node);
    void remove_pin(const PinInstance* pin);
    void remove_link(const Link* link);

    void mark_invalid(hid id);

    // ComfyUiObjectValidityListener
    void finalize_resolve_comfyui_objects() override;

private:
    std::atomic<lid> m_child_id = 0;    // Unique ID across current graph, starts from 1
    std::vector<std::unique_ptr<NodeInstance>> m_nodes;
    std::vector<std::unique_ptr<PinInstance>> m_pins;
    std::vector<std::unique_ptr<Link>> m_links;
    std::set<hid> m_invalid_items;
    bool m_invalidation_state = false;
};

} // namespace data
