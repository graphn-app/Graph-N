
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
    NodeInstance(Workflow* wf);
    ~NodeInstance() override;

    void init();
    void draw() const;

    const std::vector<PinInstance*>& get_input() const { return m_input; }
    const std::vector<PinInstance*>& get_output() const { return m_output; }

    void remove_pin(PinInstance* pin);

protected:
    void make_graph_id();
    data::Node* find_obj(hid obj_id);

private:
    std::vector<PinInstance*> m_input;
    std::vector<PinInstance*> m_output;
};

} // namespace ui