
#include "core/workflow/node_instance.h"
#include "core/workflow/workflow.h"
#include "core/workflow/pin_instance.h"
#include "core/managers/nodes_manager.h"

namespace data
{

NodeInstance::~NodeInstance() {};

void NodeInstance::init()
{
    Instance<data::Node>::init();

    m_input.reserve(m_object->required.pins.size() + m_object->optional.pins.size());
    m_output.reserve(m_object->output.size());

    const auto add_pins = [](Workflow* wf, const std::vector<data::Pin*>& source, std::vector<PinInstance*>& dest)
    {
        for (auto& pin : source)
        {
            dest.push_back(wf->add_pin(pin));
        }
    };

    add_pins(m_wf, m_object->required.pins, m_input);
    add_pins(m_wf, m_object->optional.pins, m_input);
    add_pins(m_wf, m_object->hidden.pins, m_input);
    add_pins(m_wf, m_object->output, m_output);
}

data::Node* NodeInstance::find_obj(hid obj_id)
{
    return g::pNodesManager->find_node(obj_id);
}

} // namespace data
