
#include "ui/workflow/workflow.h"
#include "ui/workflow/node_instance.h"
#include "ui/workflow/pin_instance.h"
#include "core/managers/nodes_manager.h"
#include "imgui_node_editor.h"

namespace ui
{

Workflow::Workflow()
{}

Workflow::~Workflow()
{}

NodeInstance* Workflow::find_node(aid node_id) const
{
    const auto it = m_nodes.find(node_id);
    if (it == m_nodes.end())
        return nullptr;
    return it->second.get();
}

PinInstance* Workflow::find_pin(aid pin_id) const
{
    const auto it = m_pins.find(pin_id);
    if (it == m_pins.end())
        return nullptr;
    return it->second.get();
}

NodeInstance* Workflow::add_node(hid node_id)
{
    auto node = g::pNodesManager->find_node(node_id);
    if (nullptr == node)
    {
        LogAndThrowInternalErr("Can't find node=" << node_id);
    }

    auto instance = std::make_unique<NodeInstance>(this);
    instance->set_base(node);
    auto ptr = instance.get();
    ptr->init();
    m_nodes[ptr->get_graph_id()] = std::move(instance);
    Log("Node added id=" << node_id);
    return ptr;
}

PinInstance* Workflow::add_pin(hid pin_id)
{
    auto pin = g::pNodesManager->find_pin(pin_id);
    if (nullptr == pin)
    {
        LogAndThrowInternalErr("Can't find pin=" << pin_id);
    }

    auto instance = std::make_unique<PinInstance>(this);
    instance->set_base(pin);
    auto ptr = instance.get();
    m_pins[ptr->get_graph_id()] = std::move(instance);
    Log("Pin added id=" << pin_id);
    return ptr;
}

void Workflow::init()
{
    LogF;
    ax::NodeEditor::Config config;
    config.SettingsFile = nullptr;
    m_context = ax::NodeEditor::CreateEditor(&config);
}

void Workflow::remove(hid id)
{
    Log("Obj to remove id=" << id);
}

void Workflow::draw() const
{
}

} // namespace ui
