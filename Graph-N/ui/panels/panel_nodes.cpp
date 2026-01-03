
#include "ui/panels/panel_nodes.h"
#include "ui/panels/panel_node_inspector.h"
#include "ui/graph-n_flow.h"
#include "ui/font_guard.h"
#include "platform/main.h"
#include "core/managers/settings_manager.h"
#include "core/managers/nodes_manager.h"

namespace ui
{

PanelNodes::PanelNodes(PanelNodeInspector* inspector)
    : PanelBase(VisibleObject::PanelNodes, "Node Library", "0C72C6D56FFC4902A037A187719CA0AC")
    , m_inspector(inspector)
{}

PanelNodes::~PanelNodes() = default;

void PanelNodes::init()
{
    LogF;
    PanelBase::init();
    data()->buffer.resize(PanelNodesData::buff_count);

    g::pFlow->subscribe_comfyui_validity_state(this);

    data()->tree_view = g::pOptsManager->flow_get_nodes_pane_tree_view();

    data()->clr_bg = g::pOptsManager->flow_get_color(VisibleObject::PanelNodes);
    data()->clr_hover_group = g::pOptsManager->flow_get_color(VisibleObject::PanelNodesHoverGroup);
    data()->clr_hover_node = g::pOptsManager->flow_get_color(VisibleObject::PanelNodesHoverNode);
    data()->clr_selected_node = g::pOptsManager->flow_get_color(VisibleObject::PanelNodesSelectedNode);

    const auto adjust_brightness = [](ImU32 color, float factor)
    {
        ImVec4 c = ImGui::ColorConvertU32ToFloat4(color);

        c.x = std::min(c.x * factor, 1.0f); // R
        c.y = std::min(c.y * factor, 1.0f); // G
        c.z = std::min(c.z * factor, 1.0f); // B

        return ImGui::ColorConvertFloat4ToU32(c);
    };

    data()->clr_hover_btn = adjust_brightness(data()->clr_bg, 1.5f);
    data()->clr_active_btn = adjust_brightness(data()->clr_bg, 1.7f);
}

void PanelNodes::invalidate_comfyui_objects()
{
    data()->open_items.clear();
    data()->selected_nodes.clear();
}

DlgCode PanelNodes::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused, data()->clr_bg) && opened)
    {
        if (g::pNodesManager->get_nodes_tree().empty())
        {
            ImGui::TextUnformatted("Loading...");
            return end_panel(true);
        }

        show_cfg_buttons();

        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::InputTextWithHint("##filter nodes", "filter nodes", &data()->buffer[PanelNodesData::buff_filter],
            ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoUndoRedo | ImGuiInputTextFlags_EscapeClearsAll))
        {
            data()->buffer[PanelNodesData::buff_filter_lowered] = tools::str::to_lower_ascii(data()->buffer[PanelNodesData::buff_filter]);
            Log("Filter by: " << data()->buffer[PanelNodesData::buff_filter_lowered]);
            g::pNodesManager->calc_number_inside(tools::str::trim_this(data()->buffer[PanelNodesData::buff_filter_lowered]));
            if (!data()->buffer[PanelNodesData::buff_filter_lowered].empty())
                expand_all();
        }

        ImGui::Spacing(3);
        if (ImGui::BeginChild("NodesGroup", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        {
            if (data()->tree_view)
                show_tree_view();
            else
                show_plain_view();
        }
        ImGui::EndChild();
    }
    return end_panel(opened);
}

void PanelNodes::show_tree_view()
{
    auto& nodes = g::pNodesManager->get_nodes_tree();

    std::function<void(const std::vector<data::Nodes*>&, uint32_t)> show_tree;
    const float indent_w = GImGui->Style.IndentSpacing;
    const auto init_pos = ImGui::GetCursorPosX();

    ImGui::PushStyleColor(ImGuiCol_Header, data()->clr_bg);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, data()->clr_bg);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, data()->clr_bg);

    show_tree = [this, &show_tree, data = data(), indent_w, init_pos]
    (const std::vector<data::Nodes*>& items, uint32_t indent_level)
    {
        for (auto node_holder : items)
        {
            if (nullptr == node_holder || node_holder->is_node && nullptr == node_holder->node)
                continue;

            const auto nodes_inside = data->buffer[PanelNodesData::buff_filter_lowered].empty() ? node_holder->nodes_inside : node_holder->nodes_inside_unfiltered;
            if (!node_holder->is_node && 0 == nodes_inside || node_holder->is_node && node_holder->filtered)
                continue;

            if (indent_level > 0)
                ImGui::SetCursorPosX(init_pos + indent_w * indent_level);

            ImGui::BeginGroup();
            if (!node_holder->is_node)
            {
                ImGui::SetNextItemOpen(data->open_items.count(node_holder->get_id()) > 0);

                ImGui::PushID(node_holder->get_id());
                if (ImGui::CollapsingHeader(node_holder->name_or_category.c_str(),
                    ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_SpanFullWidth))
                {
                    show_tree(node_holder->children, indent_level + 1);
                    data->open_items.insert(node_holder->get_id());
                }
                else
                {
                    data->open_items.erase(node_holder->get_id());
                }
                ImGui::PopID();
            }
            else
            {
                if (data->selected_nodes.count(node_holder->get_id()) > 0)
                    ImGui::PushStyleColor(ImGuiCol_Text, data->clr_selected_node);

                ImGui::PushID(node_holder->get_id());
                ImGui::CollapsingHeader(node_holder->name_or_category.c_str(),
                    ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_SpanFullWidth);
                ImGui::PopID();

                if (data->selected_nodes.count(node_holder->get_id()) > 0)
                    ImGui::PopStyleColor();

                if (!test_node_clicked(node_holder) &&
                    !node_holder->node->description.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && ImGui::BeginTooltip())
                {
                    ImGui::TextUnformatted(node_holder->node->description.c_str());
                    ImGui::EndTooltip();
                }
            }
            ImGui::EndGroup();

            ImVec2 v_min = ImGui::GetItemRectMin();
            ImVec2 v_max = ImGui::GetItemRectMax();
            v_max.x = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x - ImGui::GetStyle().FramePadding.x;
            if (ImGui::IsMouseHoveringRect(v_min, v_max, true))
            {
                if (node_holder->is_node)
                {
                    v_min.x = ImGui::GetCursorScreenPos().x + ImGui::GetStyle().FramePadding.x;
                }
                if (indent_level > 0)
                    v_max.x -= ImGui::GetStyle().FramePadding.x * indent_level;

                ImGui::GetWindowDrawList()->AddRect(v_min, v_max,
                    node_holder->is_node ? data->clr_hover_node : data->clr_hover_group,
                    0.0f, 0, 1.0f
                );
            }
        }
    };

    show_tree(nodes, 0);

    ImGui::PopStyleColor(3);
}

void PanelNodes::show_plain_view()
{
    auto& nodes = g::pNodesManager->get_nodes_plain();

    ImGui::PushStyleColor(ImGuiCol_Header, data()->clr_bg);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, data()->clr_bg);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, data()->clr_bg);

    for (auto node_holder : nodes)
    {
        if (nullptr == node_holder || nullptr == node_holder->node || !node_holder->is_node || node_holder->filtered)
            continue;
        auto node = node_holder->node;

        ImGui::BeginGroup();

        if (data()->selected_nodes.count(node_holder->get_id()) > 0)
            ImGui::PushStyleColor(ImGuiCol_Text, data()->clr_selected_node);

        ImGui::PushID(node_holder->get_id());
        ImGui::CollapsingHeader(node_holder->name_or_category.c_str(),
            ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_SpanFullWidth);
        ImGui::PopID();

        if (data()->selected_nodes.count(node_holder->get_id()) > 0)
            ImGui::PopStyleColor();

        if (!test_node_clicked(node_holder) &&
            !node->description.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && ImGui::BeginTooltip())
        {
            ImGui::TextUnformatted(node->description.c_str());
            ImGui::EndTooltip();
        }

        ImGui::EndGroup();

        ImVec2 v_min = ImGui::GetItemRectMin();
        ImVec2 v_max = ImGui::GetItemRectMax();
        v_max.x = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x - ImGui::GetStyle().FramePadding.x;
        if (ImGui::IsMouseHoveringRect(v_min, v_max, true))
        {
            ImGui::GetWindowDrawList()->AddRect(v_min, v_max,
                data()->clr_hover_group,
                0.0f, 0, 1.0f
            );
        }
    }

    ImGui::PopStyleColor(3);
}

bool PanelNodes::test_node_clicked(data::Nodes* holder)
{
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        if (ImGui::IsKeyDown(ImGuiMod_Ctrl) || ImGui::IsKeyDown(ImGuiMod_Shift))
        {
            if (data()->selected_nodes.count(holder->get_id()) > 0)
            {
                data()->selected_nodes.erase(holder->get_id());
                m_inspector->remove_inspecting_node(holder->node);
            }
            else
            {
                data()->selected_nodes.insert(holder->get_id());
                m_inspector->add_inspecting_node(holder->node);
            }
        }
        else
        {
            data()->selected_nodes.clear();
            m_inspector->reset_inspecting_nodes();

            data()->selected_nodes.insert(holder->get_id());
            m_inspector->add_inspecting_node(holder->node);
        }
        return true;
    }
    return false;
}

void PanelNodes::expand_all()
{
    std::function<void(const std::vector<data::Nodes*>&)> add_id;
    add_id = [&add_id, data = data()](const std::vector<data::Nodes*>& nodes)
    {
        for (auto node : nodes)
        {
            if (!node->is_node)
            {
                data->open_items.insert(node->get_id());
                add_id(node->children);
            }
        }
    };

    add_id(g::pNodesManager->get_nodes_tree());
}

void PanelNodes::collapse_all()
{
    data()->open_items.clear();
}

void PanelNodes::show_cfg_buttons()
{
    ImGui::PushStyleColor(ImGuiCol_Button, data()->clr_bg);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, data()->clr_hover_btn);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, data()->clr_active_btn);

    const float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const uint32_t button_count = 4;
    const uint32_t big_spaces = 2;
    const uint32_t small_spaces = 1;
    const float start_pos =
        window_visible_x2 -
        (button_count * data()->button_width) -
        (big_spaces * spacing * data()->ungrouped_btn_spacing_x) -
        (small_spaces * spacing * data()->grouped_btn_spacing_x) -
        spacing * 3;

    ImGui::SameLine(start_pos);

    const bool tree_view = data()->tree_view;
    if (!tree_view)
        ImGui::BeginDisabled();
    FontGuard fg(g::pMain->get_icons_font());

    const auto tooltip = [&fg](const char* text)
    {
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        {
            fg.pop();
            if (ImGui::BeginTooltip())
            {
                ImGui::TextUnformatted(text);
                ImGui::EndTooltip();
            }
        }
    };

    if (ImGui::Button(ICON_LC_LIST_CHEVRONS_UP_DOWN))
    {
        expand_all();
    }
    tooltip("Expand All");

    ImGui::SameLine(0.0f, spacing * data()->grouped_btn_spacing_x);
    fg.push();
    if (ImGui::Button(ICON_LC_LIST_CHEVRONS_DOWN_UP))
    {
        collapse_all();
    }
    tooltip("Collapse All");
    if (!tree_view)
        ImGui::EndDisabled();

    ImGui::SameLine(0.0f, spacing * data()->ungrouped_btn_spacing_x);
    fg.push();
    if (ImGui::Button(data()->tree_view ? ICON_LC_FOLDER_TREE : ICON_LC_LIST_TREE))
    {
        data()->tree_view = !data()->tree_view;
        g::pOptsManager->flow_set_nodes_pane_tree_view(data()->tree_view);
    }
    tooltip("Toggle Flat/Category View");

    const bool online = g::pFlow->get_online();
    if (!online)
        ImGui::BeginDisabled();
    ImGui::SameLine(0.0f, spacing * data()->ungrouped_btn_spacing_x);
    fg.push();
    if (ImGui::Button(ICON_LC_FOLDER_SYNC))
    {
        g::pFlow->reconnect();
    }
    tooltip("Refresh Node List");
    if (!online)
        ImGui::EndDisabled();

    //ImGui::SameLine();
    //if (ImGui::Button(ICON_LC_COLUMNS_3_COG))
    //{
    //    // Handle opts
    //}
    //if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && ImGui::BeginTooltip())
    //{
    //    ImGui::TextUnformatted("View options");
    //    ImGui::SetWindowFontScale(data()->icon_scale);
    //    ImGui::EndTooltip();
    //}

    ImGui::PopStyleColor(3);
}

} // namespace ui
