
#include "core/workflow/pin_instance.h"
#include "core/workflow/workflow.h"
#include "core/managers/nodes_manager.h"
#include "core/data/pin.h"

namespace data
{

PinInstance::PinInstance(iid parent_node, Pin* obj, wid workflow_id, iid instance_id)
    : Instance<Pin>(obj, workflow_id, instance_id)
    , m_parent_node(parent_node)
{}

PinInstance::~PinInstance() = default;

void PinInstance::init()
{
    Instance<data::Pin>::init(true);
    m_state = std::make_unique<PinState>();
    *m_state = m_object->def_state;
}

data::Pin* PinInstance::find_obj(hid obj_id) const
{
    return g::pNodesManager->find_pin(obj_id);
}

} // namespace data
