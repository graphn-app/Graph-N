
#include "core/workflow/pin_instance.h"
#include "core/workflow/workflow.h"
#include "core/managers/nodes_manager.h"
#include "core/data/pin.h"

namespace data
{

PinInstance::~PinInstance() = default;

void PinInstance::init()
{
    Instance<data::Pin>::init();
    m_state = std::make_unique<PinState>();
    *m_state = m_object->def_state;
}

data::Pin* PinInstance::find_obj(hid obj_id)
{
    return g::pNodesManager->find_pin(obj_id);
}

} // namespace data
