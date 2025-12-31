
#include "ui/workflow/pin_instance.h"
#include "ui/workflow/workflow.h"
#include "core/managers/nodes_manager.h"
#include "core/data/pin.h"
#include "imgui_node_editor.h"

namespace ui
{

PinInstance::~PinInstance() = default;

void PinInstance::make_graph_id()
{
    const aid wid = m_wf->get_id();
    const aid nid = get_id();
    m_graph_id = (wid << 48) | (nid << 24);
    Log("Graph id=" << m_graph_id);
}

void PinInstance::init()
{
    Instance<data::Pin>::init();
}

data::Pin* PinInstance::find_obj(hid obj_id)
{
    return g::pNodesManager->find_pin(obj_id);
}

void PinInstance::draw()
{
    if (!is_valid())
    {
        LogAndThrowInternalError(ExcepCategory::logic, "Pin draw must not be called if invalid")
    }
    if (nullptr == m_object)    // invalidation state
        return;

    namespace ed = ax::NodeEditor;

    ed::BeginPin(get_graph_id(), m_object->input ? ed::PinKind::Input : ed::PinKind::Output);

    ImGui::TextUnformatted(m_object->name.c_str());

    ed::EndPin();
}

} // namespace ui
