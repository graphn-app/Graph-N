
#include "ui/panels/panel_node_inspector.h"
#include "core/workflow/workflow.h"
#include "core/managers/settings_manager.h"
#include "core/data/node.h"
#include "ui/graph-n_flow.h"
#include "ui/workflow/workflow_drawer.h"
#include "platform/main.h"

namespace ui
{

inline constexpr float g_def_preview_width = 340.0f;

PanelNodeInspector::PanelNodeInspector()
    : PanelBase(VisibleObject::PanelNodeInspector, "Node Inspector", "95E5A1E7883E495E8712FA33A123F3BB")
{}

PanelNodeInspector::~PanelNodeInspector() = default;

void PanelNodeInspector::init()
{
    LogF;
    PanelBase::init();

    g::pFlow->subscribe_comfyui_validity_state(this);

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
    data.node = node;
    data.descr_wrapper.set_font(g::pMain->get_regular_font());
    if (!node->description.empty())
        data.descr_wrapper.set_raw_text(&node->description);
    data.wf = g::pFlow->create_workflow(true);
    data.wf->get_wf()->add_node(node);

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
        g::pFlow->destroy_workflow(it->second.wf->get_wf()->get_id());
        m_inspecting.erase(it);
    }
}

void PanelNodeInspector::reset_inspecting_nodes()
{
    Log("Remove all nodes");
    for (const auto& [key, value] : m_inspecting)
    {
        g::pFlow->destroy_workflow(value.wf->get_wf()->get_id());
    }
    m_inspecting.clear();
}

void PanelNodeInspector::invalidate_comfyui_objects()
{
    reset_inspecting_nodes();
}

DlgCode PanelNodeInspector::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused, data()->clr_bg) && opened)
    {
        if (m_inspecting.empty())
        {
            ImGuiEx::Spacing(3);
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
    const auto avail_w = ImGui::GetContentRegionAvail().x;

    for (auto& node : m_inspecting)
    {
        ImGui::BeginGroup();
        ImGui::PushID(node.second.node->get_id_str().c_str());
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, data()->clr_selected_node);
        ImGui::TextUnformatted(node.second.node->display_name.empty() ? node.second.node->name.c_str() : node.second.node->display_name.c_str());
        ImGui::PopStyleColor();
        ImGui::Spacing();
        show_description(node.second.descr_wrapper, avail_w);
        show_preview(node.second, avail_w);
        ImGui::PopID();
        ImGui::EndGroup();
    }
}

void PanelNodeInspector::show_description(TextWrapperRO& descr_wrapper, float max_w)
{
    auto raw = descr_wrapper.get_raw_text();
    if (raw != nullptr && *raw != 0)
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, data()->clr_bg);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Border, data()->clr_descr_frame);

        ImGui::SetNextItemWidth(max_w);
        ImGuiEx::InputTextMultiline("##NodeDescription", descr_wrapper.get_wrapped_text(),
            ImVec2(-FLT_MIN, descr_wrapper.get_wrapped_height()),
            ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);
        descr_wrapper.re_calc(ImGui::GetItemRectSize());

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();
        ImGuiEx::Spacing(3);
    }
}

void PanelNodeInspector::show_preview(Data& data, float max_w)
{
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(std::max(g_def_preview_width, data.node->node_width + 10), 0.0f));
    data.wf->draw(max_w, false);
    ImGui::EndGroup();
}

} // namespace ui
