
#include "ui/panels/panel_nodes.h"
#include "ui/graph-n_flow.h"
#include "core/managers/settings_manager.h"
#include "core/managers/nodes_manager.h"

namespace ui
{

PanelNodes::PanelNodes()
    : PanelBase(VisibleObject::PanelNodes, "Node Library", "0C72C6D56FFC4902A037A187719CA0AC")
{}

void PanelNodes::init()
{
    PanelBase::init();
    data()->buffer.resize(2);	// 0 - filter buffer, 1 - lowered

    data()->clr_bg = g::pOptsManager->flow_get_color(VisibleObject::PanelNodes);
    data()->clr_hover_group = g::pOptsManager->flow_get_color(VisibleObject::PanelNodesHoverGroup);
    data()->clr_hover_node = g::pOptsManager->flow_get_color(VisibleObject::PanelNodesHoverNode);
}

DlgCode PanelNodes::show(bool& focused)
{
    if (!begin_panel(focused, data()->clr_bg))
        return end_panel(false);

    auto& nodes = g::pNodesManager->get_nodes_tree();
    if (nodes.empty())
    {
        ImGui::Text("Loading...");
        return end_panel(true);
    }

    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::InputTextWithHint("##filter nodes", "filter nodes", &data()->buffer[0],
        ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoUndoRedo | ImGuiInputTextFlags_EscapeClearsAll))
    {
        data()->buffer[1] = tools::str::to_lower_ansi(data()->buffer[0]);
        g::pNodesManager->calc_number_inside(data()->buffer[1]);
    }

    ImGui::Spacing(3);
    ImGui::BeginChild("NodesGroup", ImVec2(ImGui::GetContentRegionAvail().x, 0));

    std::function<void(const std::vector<data::Nodes>&, uint32_t)> show_tree;
    const float indent_w = GImGui->Style.IndentSpacing;
    const auto init_pos = ImGui::GetCursorPosX();

    show_tree = [&show_tree, data = data(), indent_w, init_pos]
				(const std::vector<data::Nodes>& items, uint32_t indent_level)
    {
        for (auto& node : items)
        {
            if (indent_level > 0)
                ImGui::SetCursorPosX(init_pos + indent_w * indent_level);

            ImGui::BeginGroup();
            if (!node.is_node)
            {
                const auto nodes_inside = data->buffer[1].empty() ? node.nodes_inside : node.nodes_inside_filtered;
                if (0 == nodes_inside)
                    continue;

                ImGui::PushStyleColor(ImGuiCol_Header, data->clr_bg);
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, data->clr_bg);
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, data->clr_bg);

                if (!data->buffer[1].empty() || data->open_items.count(node.get_id()) > 0)
                    ImGui::SetNextItemOpen(true);

                if (ImGui::CollapsingHeader((node.name_or_category + " (" + std::to_string(nodes_inside) + ")##" + node.get_id_str()).c_str(),
                    ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_SpanFullWidth))
                {
                    show_tree(node.children, indent_level + 1);
					if (data->buffer[1].empty())
						data->open_items.insert(node.get_id());
                }
                else if (data->buffer[1].empty())
                {
                    data->open_items.erase(node.get_id());
                }

                ImGui::PopStyleColor(3);
            }
            else if (!node.filtered)
            {
                ImGui::PushStyleColor(ImGuiCol_Header, data->clr_bg);
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, data->clr_bg);
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, data->clr_bg);

                if (ImGui::CollapsingHeader((node.name_or_category + "##" + node.get_id_str()).c_str(), ImGuiTreeNodeFlags_Bullet |
                    ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_SpanFullWidth))
                {

                }

                ImGui::PopStyleColor(3);
            }
            ImGui::EndGroup();

            ImVec2 vMin = ImGui::GetItemRectMin();
            ImVec2 vMax = ImGui::GetItemRectMax();
            vMax.x = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x - ImGui::GetStyle().FramePadding.x;
            if (ImGui::IsMouseHoveringRect(vMin, vMax, true))
            {
                if (node.is_node)
                {
                    vMin.x = ImGui::GetCursorScreenPos().x + ImGui::GetStyle().FramePadding.x;
                }
				if (indent_level > 0)
	                vMax.x -= ImGui::GetStyle().FramePadding.x * indent_level;

                ImGui::GetWindowDrawList()->AddRect(vMin, vMax,
                    node.is_node ? data->clr_hover_node : data->clr_hover_group,
                    0.0f, 0, 1.0f
                );
            }
        }
    };

    show_tree(nodes, 0);
    ImGui::EndChild();

    return end_panel(true);
}

} // namespace ui