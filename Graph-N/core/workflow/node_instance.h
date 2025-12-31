
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

    Node* find_obj(hid obj_id);
    const std::vector<PinInstance*>& get_input() const { return m_input; }
    const std::vector<PinInstance*>& get_output() const { return m_output; }

private:
    std::vector<PinInstance*> m_input;
    std::vector<PinInstance*> m_output;
};

} // namespace data
