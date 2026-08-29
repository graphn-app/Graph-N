#include "ui/panels/panel_node_inspector.h"
#include "core/workflow/node.h"
#include "core/workflow/pin.h"
#include "core/workflow/node_instance_ro.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/workflow.h"
#include "core/managers/workflows_manager.h"
#include "main.h"

namespace ui
{

PanelNodeInspector::PanelNodeInspector()
    : PanelBase(DlgObj::PanelNodeInspector, "Node Inspector")
{
    g::pGraphN->subscribe_comfyui_validity_state(listener::ComfyUiObjectValidityListener::Step::Invalidate, this);
    g::pGraphN->subscribe_comfyui_validity_state(listener::ComfyUiObjectValidityListener::Step::ResolvePanels, this);
}

PanelNodeInspector::~PanelNodeInspector() noexcept
{
    if (g::pGraphN != nullptr)
        g::pGraphN->unsubscribe_comfyui_validity_state(this);
}

void PanelNodeInspector::init()
{
    PanelBase::init();

    dlg_data()->clr_descr_frame = g::pOptsManager->color_load(AppColor::PanelNodesInspDescrFrame);
    dlg_data()->clr_selected_node = g::pOptsManager->color_load(AppColor::PanelNodesSelNode);

    const auto wf_id = g::pWfsManager->create(false /*generate_name*/, false /*dump_to_db*/, true /*ro*/).id;
    m_wf = g::pWfsManager->access(wf_id);
}

void PanelNodeInspector::add_inspecting_node(data::Node* node)
{    
    LogRAssert(node, "Zero ptr", );
    if (std::ranges::find_if(m_inspecting, [node](const Data& d) noexcept
        { return d.node->get_object()->get_hash() == node->get_hash(); }) != m_inspecting.end())
    {
        return;
    }
    Log("Add node. n:" << node->name << ", cl:" << node->node_class);

    Data data(m_wf);
    data.node = m_wf->add_node_ro(node);
    const auto ideal = data.node->calc_node_width(true, true);
    const auto actual = data.node->recalc_node_width(0.0f, true, true);
    data.ideal_size = std::max(ideal, actual);
    if (!node->description.empty())
    {
        data.descr = &node->description;
        data.descr_sz = ImGuiEx::CalcTextSizeForMultilineWithHScroll(node->description);
    }

    m_inspecting.push_back(std::move(data));
}

void PanelNodeInspector::remove_inspecting_node(data::Node* node)
{
    LogRAssert(node, "Zero ptr", );
    Log("Remove node. n:" << node->name << ", cl:" << node->node_class);

    const auto it = std::ranges::find_if(m_inspecting, [node](const Data& d) noexcept { return d.node->get_object()->get_hash() == node->get_hash(); });
    LogRAssert(it != m_inspecting.end(), "Node not found", );

    m_wf->remove_node(it->node);
    m_inspecting.erase(it);    
}

void PanelNodeInspector::reset_inspecting_nodes() noexcept
{
    Log("Remove all nodes");
    m_wf->clear();
    m_inspecting.clear(); m_inspecting.shrink_to_fit();
}

bool PanelNodeInspector::is_node_inspecting(const data::Node* node) const noexcept
{
    LogRAssert(node, "Zero ptr", false);
    for (auto& data : m_inspecting)
    {
        if (data.node->get_object()->get_hash() == node->get_hash())
            return true;
    }
    return false;
}

void PanelNodeInspector::invalidate_comfyui_objects() noexcept
{
    reset_inspecting_nodes();
}

DlgCode PanelNodeInspector::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused) && opened)
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
                    "How to use (in Nodes Library):",
                    ICON_LC_MOUSE_POINTER_2 " Right-Click: Inspect single node",
                    ICON_LC_LAYERS " Ctrl + R-Click: Add/Remove nodes"
                }
            );
        }
        else [[likely]]
        {
            show_nodes();
        }
    }

    return end_panel(opened);
}

void PanelNodeInspector::show_nodes()
{
    const ImGuiStyle& style = ImGui::GetStyle();
    const auto avail_w = ImGui::GetContentRegionAvail().x;
    constexpr float min_descr_w = 200.0f;
    float curr_y = 0.0f;

    for (size_t i = 0; i < m_inspecting.size(); i++)
    {
        auto& node = m_inspecting[i];
        auto node_obj = node.node->get_object();

        float col_w = std::max(node.ideal_size, min_descr_w);
        col_w = std::min(col_w, avail_w);

        ImGui::PushID(node_obj);
        ImGui::BeginGroup();

        ImGui::Spacing();
        node.drawer.draw(node.node, false, col_w);
        show_description(node.descr, node.descr_sz, col_w);

        ImGui::EndGroup();
        ImGui::PopID();

        if (i + 1 < m_inspecting.size())
        {
            const float last_group_w = ImGui::GetItemRectSize().x;
            curr_y += last_group_w + style.ItemSpacing.x;
            const float next_col_w = std::max(m_inspecting[i + 1].ideal_size, min_descr_w);
            if (curr_y + style.ItemSpacing.x + next_col_w <= avail_w)
                ImGui::SameLine();
            else
                curr_y = 0.0f;
        }
    }
}

void PanelNodeInspector::show_description(std::string* descr, const ImVec2& descr_sz, float max_w) noexcept
{
    if (descr != nullptr && !descr->empty())
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, dlg_data()->clr_bg);
        ImGui::PushStyleColor(ImGuiCol_Border, 0);
        ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::NodeDescription));

        ImGuiEx::InputTextMultilineWithHScroll("##NodeDescr", "##NodeDescrBlock", descr, max_w, std::min(descr_sz.y, 120.0f), descr_sz, ImGuiInputTextFlags_ReadOnly);

        ImGui::PopStyleColor(3);
        ImGuiEx::Spacing(3);
    }
}

} // namespace ui
