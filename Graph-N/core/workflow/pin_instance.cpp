
#include "core/workflow/pin_instance.h"
#include "core/workflow/workflow.h"
#include "core/managers/nodes_manager.h"
#include "core/data/pin.h"

namespace data
{

PinInstance::PinInstance(Workflow* wf, Pin* obj, NodeInstance* parent_instance, iid instance_id, gid graph_id)
    : Instance<Pin>(wf, obj, instance_id, graph_id)
    , m_parent_instance(parent_instance)
{}

PinInstance::~PinInstance() = default;

void PinInstance::init()
{
    Instance<data::Pin>::init();
    m_state = std::make_unique<PinState>();
    *m_state = m_object->def_state;
}

data::Pin* PinInstance::find_obj(hid obj_id) const
{
    return g::pNodesManager->find_pin(obj_id);
}

} // namespace data
