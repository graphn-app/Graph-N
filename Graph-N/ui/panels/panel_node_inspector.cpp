
#include "ui/panels/panel_node_inspector.h"
#include "ui/text_wrapper_ro.h"
#include "ui/workflow/workflow.h"
#include "ui/graph-n_flow.h"
#include "platform/main.h"
#include "core/managers/settings_manager.h"
#include "core/data/node.h"

namespace ui
{

PanelNodeInspector::PanelNodeInspector()
    : PanelBase(VisibleObject::PanelNodeInspector, "Node Inspector", "95E5A1E7883E495E8712FA33A123F3BB")
{}

PanelNodeInspector::~PanelNodeInspector() = default;

void PanelNodeInspector::init()
{
    LogF;
    PanelBase::init();

    data()->clr_bg = g::pOptsManager->flow_get_color(VisibleObject::PanelNodeInspector);
    data()->clr_descr_frame = g::pOptsManager->flow_get_color(VisibleObject::PanelNodeInspectorDescrFrame);
    data()->clr_selected_node = g::pOptsManager->flow_get_color(VisibleObject::PanelNodesSelectedNode);
}

void PanelNodeInspector::add_inspecting_node(data::Node* node)
{
    Log("Add node '" << node->name << "' id=" << node->get_id());
    if (nullptr == node || m_inspecting.find(node->get_id()) != m_inspecting.end())
        return;

    Data data;
    data.node_name = node->display_name.empty() ? node->name.c_str() : node->display_name.c_str();
    data.descr_wrapper = std::make_unique<TextWrapperRO>();
    data.descr_wrapper->set_font(g::pMain->get_regular_font());
    if (!node->description.empty())
        data.descr_wrapper->set_raw_text(&node->description);
    data.wf = g::pFlow->create_workflow();
    data.wf->add_node(node);

    m_inspecting.emplace(node->get_id(), std::move(data));
}

void PanelNodeInspector::remove_inspecting_node(data::Node* node)
{
    Log("Remove node '" << node->name << "' id=" << node->get_id());

    auto it = m_inspecting.find(node->get_id());
    if (it == m_inspecting.end())
    {
        LogInternalError(ExcepCategory::inconsistent_map, "Can't remove node");
    }
    else
    {
        g::pFlow->destroy_workflow(it->second.wf->get_id());
        m_inspecting.erase(it);
    }
}

void PanelNodeInspector::reset_inspecting_nodes()
{
    Log("Remove all nodes");
    for (const auto& [key, value] : m_inspecting)
    {
        g::pFlow->destroy_workflow(value.wf->get_id());
    }
    m_inspecting.clear();
}

DlgCode PanelNodeInspector::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused, data()->clr_bg))
    {
        if (m_inspecting.empty())
        {
            ImGui::Spacing(3);
            ImGui::Text(ICON_LC_TANGENT " Select node(s) to inspect");
            return end_panel(true);
        }
        else
        {
            show_nodes();
        }
    }
    return end_panel(opened);
}

void PanelNodeInspector::show_nodes()
{
    for (auto& node : m_inspecting)
    {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, data()->clr_selected_node);
        ImGui::TextUnformatted(node.second.node_name.c_str());
        ImGui::PopStyleColor();
        ImGui::Spacing();

        show_description(node.second.descr_wrapper.get());
        show_preview(node.second.wf);
    }
}

void PanelNodeInspector::show_description(TextWrapperRO* descr_wrapper)
{
    auto raw = descr_wrapper->get_raw_text();
    if (raw != nullptr && *raw != 0)
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, data()->clr_bg);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Border, data()->clr_descr_frame);

        ImGui::InputTextMultiline("##NodeDescription", descr_wrapper->get_wrapped_text(), descr_wrapper->get_wrapped_capacity() + 1,
            ImVec2(-FLT_MIN, descr_wrapper->get_wrapped_height()),
            ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);
        descr_wrapper->re_calc(ImGui::GetItemRectSize());

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();
        ImGui::Spacing(3);
    }
}

void PanelNodeInspector::show_preview(Workflow* wf)
{
    if (ImGui::BeginChild(wf->get_id(), ImVec2(ImGui::GetContentRegionAvail().x, std::min(ImGui::GetContentRegionAvail().x, 300.0f))))
    {
        wf->draw();
    }
    ImGui::EndChild();
}

} // namespace ui
