
#pragma once
#include "ui/workflow/instance.h"
#include "core/data/node.h"

namespace ui
{

class Workflow;
class PinInstance;

class NodeInstance final : public Instance<data::Node>
{
public:
    using Instance<data::Node>::Instance;
    ~NodeInstance() override;

    void init() override;
    void draw() override;

    data::Node* find_obj(hid obj_id);
    const std::vector<PinInstance*>& get_input() const { return m_input; }
    const std::vector<PinInstance*>& get_output() const { return m_output; }

protected:
    void make_graph_id();

private:
    std::vector<PinInstance*> m_input;
    std::vector<PinInstance*> m_output;
};

} // namespace ui
