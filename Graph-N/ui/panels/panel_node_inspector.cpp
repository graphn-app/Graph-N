
#include "ui/panels/panel_node_inspector.h"
#include "platform/main.h"
#include "core/managers/settings_manager.h"
#include "core/data/node.h"
#include "imgui_node_editor.h"

namespace ui
{

PanelNodeInspector::PanelNodeInspector()
    : PanelBase(VisibleObject::PanelNodeInspector, "Node Inspector", "95E5A1E7883E495E8712FA33A123F3BB")
{}

void PanelNodeInspector::init()
{
    PanelBase::init();

    data()->clr_bg = g::pOptsManager->flow_get_color(VisibleObject::PanelNodeInspector);
    data()->clr_descr_frame = g::pOptsManager->flow_get_color(VisibleObject::PanelNodeInspectorDescrFrame);
    data()->clr_selected_node = g::pOptsManager->flow_get_color(VisibleObject::PanelNodesSelectedNode);

    m_descr_wrapper.set_font(g::pMain->get_regular_font());

    ax::NodeEditor::Config config;
    config.SettingsFile = nullptr;
    m_context = ax::NodeEditor::CreateEditor(&config);

    m_preview_node_id = base::LocalID::get_next_id();
}

void PanelNodeInspector::set_inspecting_node(data::Node* node)
{
    m_node_changed = true;
    m_inspecting_node = node;
    if (nullptr == node || node->description.empty())
        m_descr_wrapper.set_raw_text(nullptr);
    else
        m_descr_wrapper.set_raw_text(&node->description);
}

DlgCode PanelNodeInspector::show(bool& focused)
{
    if (!begin_panel(focused, data()->clr_bg))
        return end_panel(false);

    if (nullptr == m_inspecting_node)
    {
        ImGui::Spacing(3);
        ImGui::Text(ICON_LC_TANGENT " Select node to inspect");
        return end_panel(true);
    }

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, data()->clr_selected_node);
    ImGui::Text(m_inspecting_node->display_name.empty() ? m_inspecting_node->name.c_str() : m_inspecting_node->display_name.c_str());
    ImGui::PopStyleColor();
    ImGui::Spacing();

    show_description();
    show_node_preview();

    m_node_changed = false;
    return end_panel(true);
}

void PanelNodeInspector::show_description()
{
    if (!m_inspecting_node->description.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, data()->clr_bg);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Border, data()->clr_descr_frame);

        ImGui::InputTextMultiline("##NodeDescription", m_descr_wrapper.get_wrapped_text(), m_descr_wrapper.get_wrapped_capacity() + 1,
            ImVec2(-FLT_MIN, m_descr_wrapper.get_wrapped_height()),
            ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);
        m_descr_wrapper.re_calc(ImGui::GetItemRectSize());

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();
        ImGui::Spacing(3);
    }
}

void PanelNodeInspector::show_node_preview()
{
    namespace ed = ax::NodeEditor;

    const ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, -FLT_MIN);

    auto old_editor = ed::GetCurrentEditor();
    ed::SetCurrentEditor(m_context);

    ed::Begin("PreviewCanvas", canvas_size);
    ed::BeginNode(m_preview_node_id);

    ImGui::Text(m_inspecting_node->name.c_str());

    //// Inputs
    //for (auto& pin : selected_node->inputs) {
    //    ed::BeginPin(GetPreviewPinID(pin.name), ed::PinKind::Input);
    //    ImGui::Text(pin.name.c_str());
    //    ed::EndPin();
    //}

    // Outputs...

    ed::EndNode();

  //  if (m_node_changed) 
    //    ed::NavigateToContent();

    ed::End();
    ed::SetCurrentEditor(old_editor);
}

} // namespace ui