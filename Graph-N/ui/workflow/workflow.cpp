
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

void Workflow::init()
{
    LogF;
    ax::NodeEditor::Config config;
    config.SettingsFile = nullptr;
    m_context = ax::NodeEditor::CreateEditor(&config);
}

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

NodeInstance* Workflow::add_node(data::Node* node)
{
    auto instance = std::make_unique<NodeInstance>(this, node);
    auto ptr = instance.get();
    ptr->init();
    m_nodes[ptr->get_graph_id()] = std::move(instance);
    Log("Node added id=" << node->get_id());
    return ptr;
}

PinInstance* Workflow::add_pin(data::Pin* pin)
{
    auto instance = std::make_unique<PinInstance>(this, pin);
    auto ptr = instance.get();
    ptr->init();
    m_pins[ptr->get_graph_id()] = std::move(instance);
    Log("Pin added id=" << pin->get_id());
    return ptr;
}

void Workflow::remove_invalid(hid id)
{
    Log("Obj to remove id=" << id);
    m_invalid_items.insert(id);
}

void Workflow::draw()
{
    namespace ed = ax::NodeEditor;

    const ImVec2 canvas_size(-FLT_MIN, -FLT_MIN);
    auto old_editor = ed::GetCurrentEditor();
    ed::SetCurrentEditor(m_context);
    ed::Begin(get_id_str().c_str(), canvas_size);

    for (const auto& [id, node] : m_nodes)
    {
        if (node->is_valid())
            node->draw();
    }

    // ed::NavigateToContent();

    ed::End();
    ed::SetCurrentEditor(old_editor);

    if (!m_invalid_items.empty())
    {
        for (auto id : m_invalid_items)
        {
            m_nodes.erase(id);
            m_pins.erase(id);
        }
        m_invalid_items.clear();
    }
}

} // namespace ui
