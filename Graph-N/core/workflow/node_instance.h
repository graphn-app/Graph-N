
#pragma once
#include "core/workflow/instance.h"

namespace data
{

class Workflow;
class PinInstance;
struct Node;

class NodeInstance final : public Instance<Node>
{
public:
    using Instance<data::Node>::Instance;
    ~NodeInstance() override;

    void init() override;

    void set_node_width(float node_width, bool allow_unlimited_w);

    float get_node_width() const { return m_node_w; }
    float get_right_group_width() const { return m_right_group_w; }

    static inline float get_node_min_width() { return 150.0f; }

    Node* find_obj(hid obj_id);
    const std::vector<PinInstance*>& get_input() const { return m_input; }
    const std::vector<PinInstance*>& get_output() const { return m_output; }

private:
    std::vector<PinInstance*> m_input;
    std::vector<PinInstance*> m_output;
    float m_node_w = 0.0f;
    float m_right_group_w = 0.0f;
};

} // namespace data
