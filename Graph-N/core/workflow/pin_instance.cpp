
#include "core/workflow/pin_instance.h"
#include "core/workflow/node_instance.h"
#include "core/managers/nodes_manager.h"
#include "core/workflow/pin.h"

namespace data
{

PinInstance::PinInstance(NodeInstanceBase* parent_node, Pin* obj, wid workflow_id, iid instance_id) noexcept
    : Instance<Pin>(obj, workflow_id, instance_id)
    , m_parent_node(parent_node->get_instance_id())
{
}

PinInstance::~PinInstance() noexcept = default;

void PinInstance::init()
{
    Instance<data::Pin>::init();
    get_object()->def_state.copy_to(m_state);
}

void PinInstance::set_object(Pin* obj)
{
    Instance<Pin>::set_object(obj);
    get_object()->def_state.copy_to(m_state);
}

void PinInstance::set_name(const std::string& name)
{
    m_state.name = name;
    if (m_state.type != PinType::STRING)
        m_state.w_name = ImGui::CalcTextSize(name.c_str()).x;
}

} // namespace data
