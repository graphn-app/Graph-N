
#include "ui/panels/panel_node_inspector.h"
#include "core/data/node.h"
#include "core/data/pin.h"
#include "core/workflow/node_instance_ro.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/workflow.h"
#include "core/managers/workflows_manager.h"
#include "ui/graph-n_flow.h"
#include "main.h"

namespace ui
{

PanelNodeInspector::PanelNodeInspector()
    : PanelBase(DlgObj::PanelNodeInspector, "Node Inspector")
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

    data()->clr_descr_frame = g::pOptsManager->color_load(AppColor::PanelNodesInspDescrFrame);
    data()->clr_selected_node = g::pOptsManager->color_load(AppColor::PanelNodesSelNode);

    auto wf_id = g::pWfsManager->create(true /*skip_name*/).id;
    m_wf = g::pWfsManager->access(wf_id);
    g::pWfsManager->set_read_only(wf_id);
}

void PanelNodeInspector::add_inspecting_node(data::Node* node)
{    
    if (nullptr == node)
        return;
    if (std::find_if(m_inspecting.begin(), m_inspecting.end(), [node](const Data& d) { return d.node->get_object()->get_id() == node->get_id(); }) != m_inspecting.end())
        return;
    Log("Add node '" << node->name << "' id=" << node->get_id());

    Data data;
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
    Log("Remove node '" << node->name << "' id=" << node->get_id());

    const auto it = std::find_if(m_inspecting.begin(), m_inspecting.end(), [node](const Data& d) { return d.node->get_object()->get_id() == node->get_id(); });
    if (it == m_inspecting.end())
    {
        LogInternalMap("Can't remove node");
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

bool PanelNodeInspector::is_node_inspecting(data::Node* node) const
{
    for (auto& data : m_inspecting)
    {
        if (data.node->get_object()->get_id() == node->get_id())
            return true;
    }
    return false;
}

void PanelNodeInspector::invalidate_comfyui_objects()
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
                    "How to use:",
                    ICON_LC_MOUSE_POINTER_2 " Right-Click: Inspect single node",
                    ICON_LC_LAYERS " Ctrl + R-Click: Add/Remove nodes"
                }
            );
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

void PanelNodeInspector::show_description(std::string* descr, const ImVec2& descr_sz, float max_w)
{
    if (descr != nullptr && !descr->empty())
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, data()->clr_bg);
        ImGui::PushStyleColor(ImGuiCol_Border, 0);
        ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::NodeDescription));

        ImGuiEx::InputTextMultilineWithHScroll("##NodeDescr", "##NodeDescrBlock", descr, max_w, std::min(descr_sz.y, 120.0f), descr_sz, ImGuiInputTextFlags_ReadOnly);

        ImGui::PopStyleColor(3);
        ImGuiEx::Spacing(3);
    }
}

} // namespace ui
