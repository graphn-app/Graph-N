
#include "ui/workflow/node_instance.h"
#include "ui/workflow/workflow.h"
#include "ui/workflow/pin_instance.h"
#include "core/managers/nodes_manager.h"

namespace ui
{

NodeInstance::NodeInstance(Workflow* wf)
    : Instance<data::Node>(wf)
{}

NodeInstance::~NodeInstance() {};

void NodeInstance::make_graph_id()
{
    const aid wid = m_wf->get_id();
    const aid nid = get_id();
    m_graph_id = (wid << 48) | (nid << 24);
    Log("Graph id=" << m_graph_id);
}

data::Node* NodeInstance::find_obj(hid obj_id)
{
    return g::pNodesManager->find_node(obj_id);
}

void NodeInstance::init()
{
    LogF;
}

void NodeInstance::remove_pin(PinInstance* pin)
{
    Log("Pin remove id=" << pin->get_graph_id() << "/" << pin->get_instance_id());
}

void NodeInstance::draw() const
{}

} // namespace ui
