
#include "ui/panels/panel_node_inspector.h"
#include "core/managers/settings_manager.h"
#include "core/data/node.h"
#include "core/data/pin.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/workflow.h"
#include "ui/graph-n_flow.h"
#include "platform/main.h"

namespace ui
{

PanelNodeInspector::PanelNodeInspector()
    : PanelBase(VisibleObject::PanelNodeInspector, ICON_LC_TANGENT " Node Inspector", ICON_LC_TANGENT, "95E5A1E7883E495E8712FA33A123F3BB")
{}

PanelNodeInspector::~PanelNodeInspector()
{
    LogF;
}

void PanelNodeInspector::init()
{
    LogF;
    PanelBase::init();

    g::pFlow->subscribe_comfyui_validity_state(this);

    data()->clr_descr_frame = g::pOptsManager->color_load(AppColor::panel_node_insp_descr_frame);
    data()->clr_selected_node = g::pOptsManager->color_load(AppColor::panel_nodes_sel_node);

    m_wf = g::pFlow->create_workflow();
}

void PanelNodeInspector::add_inspecting_node(data::Node* node)
{    
    if (nullptr == node)
        return;
    if (std::find_if(m_inspecting.begin(), m_inspecting.end(), [node](const Data& d) { return d.node->get_object()->get_id() == node->get_id(); }) != m_inspecting.end())
        return;
    Log("Add node '" << node->name << "' id=" << node->get_id());

    Data data;
    data.node = m_wf->add_node(node);
    data.ideal_size = data.node->calc_node_width();
    data.descr_wrapper.set_font(g::pMain->get_regular_font());
    if (!node->description.empty())
        data.descr_wrapper.set_raw_text(&node->description);

    m_inspecting.emplace_back(std::move(data));
}

void PanelNodeInspector::remove_inspecting_node(data::Node* node)
{
    Log("Remove node '" << node->name << "' id=" << node->get_id());

    const auto it = std::find_if(m_inspecting.begin(), m_inspecting.end(), [node](const Data& d) { return d.node->get_object()->get_id() == node->get_id(); });
    if (it == m_inspecting.end())
    {
        LogInternalError(ExcepCategory::inconsistent_map, "Can't remove node");
    }
    else
    {
        m_wf->remove_node(it->node);
        m_inspecting.erase(it);
    }
}

void PanelNodeInspector::reset_inspecting_nodes()
{
    Log("Remove all nodes");
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
            show_empty_panel_descr(
                ICON_LC_INFO " Node Inspector: View details of selected nodes, compare parameters, and configure widget visibility.",
                {
                    ICON_LC_SPLINE_POINTER,
                    "No node selected",
                },
                {
                    "How to use:",
                    ICON_LC_MOUSE_POINTER_2 " Right-Click: Inspect single node",
                    ICON_LC_LAYERS " Ctrl + R-Click: Add/Remove nodes"
                }
            );
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
    ImGuiStyle& style = ImGui::GetStyle();
    const auto avail_w = ImGui::GetContentRegionAvail().x;
    constexpr float min_descr_w = 200.0f;
    float curr_y = 0.0f;

    for (size_t i = 0; i < m_inspecting.size(); i++)
    {
        auto& node = m_inspecting[i];
        auto node_obj = node.node->get_object();

        float col_w = std::max(node.ideal_size, min_descr_w);
        col_w = std::min(col_w, avail_w);

        ImGui::PushID(node_obj->get_id_str().c_str());
        ImGui::BeginGroup();

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, data()->clr_selected_node);
        ImGui::TextUnformatted(node.node->get_name().c_str(), node.node->get_name().c_str() + node.node->get_name().size());
        ImGui::PopStyleColor();

        ImGui::Spacing();
        node.drawer.draw(node.node, false, col_w);
        show_description(node.descr_wrapper, col_w);

        ImGui::EndGroup();
        ImGui::PopID();

        if (i + 1 < m_inspecting.size())
        {
            const float last_group_w = ImGui::GetItemRectSize().x;
            curr_y += last_group_w;
            const float next_col_w = std::max(m_inspecting[i + 1].ideal_size, min_descr_w);
            if (curr_y + style.ItemSpacing.x * 2 + next_col_w < avail_w)
                ImGui::SameLine();
            else
                curr_y = 0.0f;
        }
    }
}

void PanelNodeInspector::show_description(TextWrapperRO& descr_wrapper, float max_w)
{
    auto raw = descr_wrapper.get_raw_text();
    if (raw != nullptr && *raw != 0)
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, data()->clr_bg);
       // ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Border, data()->clr_descr_frame);

        ImGuiEx::InputTextMultiline("##NodeDescription", descr_wrapper.get_wrapped_text(),
            ImVec2(max_w, descr_wrapper.get_wrapped_height()),
            ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);
        descr_wrapper.re_calc(ImGui::GetItemRectSize());

        ImGui::PopStyleColor(2);
       // ImGui::PopStyleVar();
        ImGuiEx::Spacing(3);
    }
}

} // namespace ui
