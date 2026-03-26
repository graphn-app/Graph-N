
#include "core/workflow/pin_instance.h"
#include "core/workflow/node_instance.h"
#include "core/managers/nodes_manager.h"
#include "core/data/pin.h"

namespace data
{

PinInstance::PinInstance(NodeInstanceBase* parent_node, Pin* obj, wid workflow_id, iid instance_id)
    : Instance<Pin>(obj, workflow_id, instance_id)
    , m_parent_node(parent_node->get_instance_id())
    , m_parent_obj_hid(parent_node->get_object()->get_id())
{
}

PinInstance::~PinInstance() = default;

void PinInstance::init()
{
    Instance<data::Pin>::init();
    m_state = m_object->def_state;
    m_obj_class = m_object->def_state.name;
    m_obj_in = m_object->input;
    m_obj_out_idx = m_object->out_index;
}

data::Pin* PinInstance::find_obj(hid obj_id) const
{
    return g::pNodesManager->find_pin(obj_id);
}

void PinInstance::set_object(Pin* obj)
{
    Instance<Pin>::set_object(obj);
    m_state = m_object->def_state;
    m_obj_class = m_object->def_state.name;
    m_obj_in = m_object->input;
    m_obj_out_idx = m_object->out_index;
}

} // namespace data
