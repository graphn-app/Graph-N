
#include "ui/workflow/workflow_drawer.h"
#include "core/workflow/workflow.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/data/node.h"
#include "core/data/pin.h"
#include "imgui_node_editor.h"

namespace ui
{

namespace ed = ax::NodeEditor;

WorkflowDrawer::WorkflowDrawer()
{}

WorkflowDrawer::~WorkflowDrawer()
{
    if (m_context != nullptr)
        ed::DestroyEditor(m_context);
}

void WorkflowDrawer::init(data::Workflow* wf)
{
    LogF;

    ed::Config config;
    config.SettingsFile = nullptr;
    m_context = ed::CreateEditor(&config);

    m_wf = wf;
}

void WorkflowDrawer::draw()
{
    const ImVec2 canvas_size(-FLT_MIN, -FLT_MIN);
    auto old_editor = ed::GetCurrentEditor();
    ed::SetCurrentEditor(m_context);
    ed::Begin(m_wf->get_id_str().c_str(), canvas_size);

    auto& nodes = m_wf->get_nodes();
    auto& links = m_wf->get_links();

    for (const auto& node : nodes)
    {
        if (!node->is_valid())
            continue;

        ed::BeginNode(node->get_graph_id());

        draw_node(node.get());

        const auto& input = node->get_input();
        for (auto pin : input)
        {
            if (!pin->is_valid() || pin->get_state()->advanced || pin->get_object()->is_widget())
                continue;

            ed::BeginPin(pin->get_graph_id(), ed::PinKind::Input);
            draw_input_pin(pin);
            ed::EndPin();
        }

        const auto& output = node->get_output();
        for (auto pin : output)
        {
            if (!pin->is_valid())
                continue;

            ed::BeginPin(pin->get_graph_id(), ed::PinKind::Output);
            draw_output_pin(pin);
            ed::EndPin();
        }

        for (auto pin : input)
        {
            if (!pin->is_valid() || pin->get_state()->advanced || !pin->get_object()->is_widget())
                continue;

            draw_input_widget(pin);
        }

        ed::EndNode();
    }

    for (const auto& link : links)
    {        
        draw_link(link.get());
    }

    // ed::NavigateToContent();

    ed::End();
    ed::SetCurrentEditor(old_editor);
}

void WorkflowDrawer::draw_node(const data::NodeInstance* node)
{
    auto obj = node->get_object();
    ImGui::TextUnformatted(obj->name.c_str());
}

void WorkflowDrawer::draw_input_pin(const data::PinInstance* pin)
{
    auto obj = pin->get_object();
    ImGui::TextUnformatted(obj->name.c_str());
}

void WorkflowDrawer::draw_output_pin(const data::PinInstance* pin)
{
    auto obj = pin->get_object();
    ImGui::TextUnformatted(obj->name.c_str());
}

void WorkflowDrawer::draw_link(const data::Link* link)
{
    ed::Link(link->get_graph_id(), link->get_input()->get_graph_id(), link->get_output()->get_graph_id(), link->get_input()->get_state()->pin_color);
}

void WorkflowDrawer::draw_input_widget(const data::PinInstance* pin)
{
    auto obj = pin->get_object();
    ImGui::TextUnformatted(obj->name.c_str());
}

} // namespace ui
