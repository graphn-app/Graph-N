
#include "ui/workflow/node_instance.h"
#include "ui/workflow/workflow.h"
#include "ui/workflow/pin_instance.h"
#include "core/managers/nodes_manager.h"
#include "imgui_node_editor.h"

namespace ui
{

NodeInstance::~NodeInstance() {};

void NodeInstance::make_graph_id()
{
    const aid wid = m_wf->get_id();
    const aid nid = get_id();
    m_graph_id = (wid << 48) | (nid << 24);
    Log("Graph id=" << m_graph_id);
}

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

void NodeInstance::draw()
{
    if (!is_valid())
    {
        LogAndThrowInternalError(ExcepCategory::logic, "Node draw must not be called if invalid")
    }
    if (nullptr == m_object)    // invalidation state
        return;

    namespace ed = ax::NodeEditor;
    ed::BeginNode(get_graph_id());

    ImGui::TextUnformatted(m_object->name.c_str());

    for (auto it = m_input.begin(); it != m_input.end(); )
    {
        auto input = *it;
        if (!input->is_valid())
        {
            it = m_input.erase(it);
            continue;
        }

        if (!input->get_object()->advanced && !input->get_object()->is_widget())
        {
            input->draw();
        }

        ++it;
    }

    for (auto it = m_output.begin(); it != m_output.end(); )
    {
        auto output = *it;
        if (!output->is_valid())
        {
            it = m_output.erase(it);
            continue;
        }

        output->draw();
        ++it;
    }

    for (auto it = m_input.begin(); it != m_input.end(); ++it)
    {
        auto input = *it;
        if (input->get_object()->is_widget())
        {
            input->draw();
        }
    }

    ed::EndNode();
}

} // namespace ui
