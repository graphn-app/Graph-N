
#include "ui/workflow/pin_instance.h"
#include "ui/workflow/workflow.h"
#include "core/managers/nodes_manager.h"

namespace ui
{

PinInstance::~PinInstance() = default;

void PinInstance::make_graph_id()
{
    const aid wid = m_wf->get_id();
    const aid nid = get_id();
    m_graph_id = (wid << 48) | (nid << 24);
}

data::Pin* PinInstance::find_obj(hid obj_id)
{
    return g::pNodesManager->find_pin(obj_id);
}

void PinInstance::draw() const
{}

} // namespace ui