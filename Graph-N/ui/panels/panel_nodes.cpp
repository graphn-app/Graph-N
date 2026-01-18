
#include "ui/panels/panel_nodes.h"
#include "ui/panels/panel_node_inspector.h"
#include "ui/graph-n_flow.h"
#include "ui/font_guard.h"
#include "platform/main.h"
#include "core/managers/nodes_manager.h"
#include "core/data/node.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/workflow.h"

namespace ui
{

PanelNodes::PanelNodes(PanelNodeInspector* inspector)
    : PanelBase(DlgObj::PanelNodes, ICON_LC_BOOK_OPEN " Node Library", ICON_LC_BOOK_OPEN, "02EE3A92-9ED3-44AD-A285-95FD52C78BE3")
    , m_inspector(inspector)
{}

PanelNodes::~PanelNodes()
{
    LogF;
}

void PanelNodes::init()
{
    LogF;
    PanelBase::init();
    data()->buffer.resize(PanelNodesData::_buf_count);

    m_wf = g::pFlow->create_workflow();

    g::pFlow->subscribe_comfyui_validity_state(this);

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

    m_tt_callbacks.on_alloc_tt_data = std::bind(&PanelNodes::on_alloc_tt_data, this, std::placeholders::_1);
    m_tt_callbacks.on_release_tt_data = std::bind(&PanelNodes::on_release_tt_data, this, std::placeholders::_1);
    m_tt_callbacks.on_show_tt = std::bind(&PanelNodes::on_show_tt, this, std::placeholders::_1);
    m_tt_callbacks.on_pre_tt = std::bind(&PanelNodes::on_pre_tt, this, std::placeholders::_1);
    m_tt_callbacks.on_post_tt = std::bind(&PanelNodes::on_post_tt, this, std::placeholders::_1);

    rebuild_items();
}

void PanelNodes::invalidate_comfyui_objects()
{
    m_panel_items.clear();
}

void PanelNodes::finalize_resolve_comfyui_objects()
{
    rebuild_items();
}

DlgCode PanelNodes::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused, data()->clr_bg) && opened)
    {
        if (g::pNodesManager->get_nodes_tree().empty())
        {
            show_empty_panel_descr(
                ICON_LC_INFO " Node Library: Browse, search, and add nodes to your workflow.",
                {
                    ICON_LC_UNPLUG,
                    "Waiting for Connection.",
                    "Node definitions are unavailable."
                },
                {
                    ICON_LC_SETTINGS " Configuration:",
                    "Check Address, Port & Timeouts",
                    "in Settings > " APP_COMFYUI
                },
                "Open settings",
                []()
                {
                    g::pFlow->open_settings_dlg(OptPane::ComfyUi);
                }
            );
            return end_panel(true);
        }

        show_cfg_buttons();
        ImGui::Spacing();

        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGuiEx::InputTextWithHint("##filter nodes", "filter nodes", &data()->buffer[PanelNodesData::BuffFilter],
            ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoUndoRedo | ImGuiInputTextFlags_EscapeClearsAll))
        {
            data()->buffer[PanelNodesData::BuffFilterLowered] = tools::str::to_lower_ascii(data()->buffer[PanelNodesData::BuffFilter]);
            Log("Filter by: " << data()->buffer[PanelNodesData::BuffFilterLowered]);
            g::pNodesManager->calc_number_inside(tools::str::trim_this(data()->buffer[PanelNodesData::BuffFilterLowered]));
            if (!data()->buffer[PanelNodesData::BuffFilterLowered].empty())
                expand_all();
        }

        ImGuiEx::Spacing(3);
        if (ImGui::BeginChild("NodesGroup", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        {
            PanelItemState state;
            if (draw_list(m_panel_items, state, &m_tt_callbacks))
                handle_dirty(state);
        }
        ImGui::EndChild();
    }
    return end_panel(opened);
}

void PanelNodes::rebuild_items(bool force_expand_all /*= false*/)
{
    LogF;

    const std::string fav_folder_name = "Favorited";

    // keep selected/expanded state
    std::set<data::Nodes*> selected, expanded;
    for (auto& item : m_panel_items)
    {
        if (item.selected)
            selected.insert((data::Nodes*)item.ptr);
        else if (item.expanded)
            expanded.insert((data::Nodes*)item.ptr);
    }

    bool fav_folder_expanded = false;
    if (!m_panel_items.empty() && nullptr == m_panel_items[0].ptr)
        fav_folder_expanded = m_panel_items[0].expanded;

    m_panel_items.clear();
    m_panel_items.reserve(20); // starred

    if (g::pOptsManager->ui_nodes_pane_tree_view_load())
    {
        size_t fav_items = 0;
        auto& plain = g::pNodesManager->get_nodes_plain();
        for (auto nodes : plain)
        {
            if (nodes->is_node && nodes->node->starred)
            {
                fav_items++;
                if (!fav_folder_expanded && !force_expand_all)
                    continue;
                PanelListItem item;
                item.depth = 1;
                item.name = nodes->name_or_category;
                item.can_star = item.starred = true;
                item.can_lclick = item.can_rclick = true;
                item.selected = selected.count(nodes) > 0;
                item.ptr = nodes;
                m_panel_items.push_back(std::move(item));
            }
        }

        m_panel_items.reserve(m_panel_items.size() + g::pNodesManager->get_node_items_total() + 2);

        PanelListItem star_folder;
        star_folder.name = fav_folder_name + " [" + std::to_string(fav_items) + "]";
        star_folder.can_expand = star_folder.can_lclick = true;
        star_folder.expanded = force_expand_all || fav_folder_expanded;
        m_panel_items.insert(m_panel_items.begin(), std::move(star_folder));

        PanelListItem separator;
        separator.is_separator = true;
        m_panel_items.push_back(std::move(separator));

        std::function<void(const std::vector<data::Nodes*>&, int32_t)> go_tree;
        go_tree = [&go_tree, &panel_items = m_panel_items, &expanded, &selected, force_expand_all](const std::vector<data::Nodes*>& nodes, int32_t depth)
        {
            for (auto node : nodes)
            {
                if (!node->is_node)
                {
                    PanelListItem item;
                    item.depth = depth;
                    item.name = node->name_or_category;
                    item.can_expand = item.can_lclick = true;
                    item.expanded = force_expand_all || expanded.count(node) > 0;
                    const bool item_expanded = item.expanded;
                    item.ptr = node;
                    panel_items.push_back(std::move(item));

                    if (item_expanded)
                        go_tree(node->children, depth + 1);
                }
                else
                {
                    PanelListItem item;
                    item.depth = depth;
                    item.selected = selected.count(node) > 0;
                    item.name = node->name_or_category;
                    item.can_star = true;
                    item.starred = node->node->starred;
                    item.can_lclick = item.can_rclick = true;
                    item.ptr = node;
                    panel_items.push_back(std::move(item));
                }
            }
        };

        go_tree(g::pNodesManager->get_nodes_tree(), 0);
    }
    else
    {
        auto& plain = g::pNodesManager->get_nodes_plain();
        m_panel_items.reserve(plain.size() + 1);

        for (auto nodes : plain)
        {
            if (nodes->is_node && nodes->node->starred)
            {
                PanelListItem item;
                item.name = nodes->name_or_category;
                item.can_star = item.starred = true;
                item.can_lclick = item.can_rclick = true;
                item.selected = selected.count(nodes) > 0;
                item.ptr = nodes;
                m_panel_items.push_back(std::move(item));
            }
        }

        if (!m_panel_items.empty())
        {
            PanelListItem separator;
            separator.is_separator = true;
            m_panel_items.push_back(std::move(separator));
        }

        for (auto nodes : plain)
        {
            if (nodes->is_node && !nodes->node->starred)
            {
                PanelListItem item;
                item.name = nodes->name_or_category;
                item.can_star = item.can_lclick = item.can_rclick = true;
                item.selected = selected.count(nodes) > 0;
                item.ptr = nodes;
                m_panel_items.push_back(std::move(item));
            }
        }
    }
}

void PanelNodes::handle_dirty(const PanelItemState& dirty_item)
{
    Log("Dirty: idx=" << dirty_item.index << ", rc=" << dirty_item.rclicked << " lc=" << dirty_item.lclicked << ", fv=" << dirty_item.starred << ", xp=" << dirty_item.expanded);

    if (dirty_item.expanded)
    {
        rebuild_items();
        return;
    }

    if (dirty_item.index >= m_panel_items.size())
    {
        LogError(ExcepCategory::Logic, "Index out of bounds");
    }

    auto& panel_item = m_panel_items[dirty_item.index];
    data::Nodes* nodes = (data::Nodes*)panel_item.ptr;

    if (nullptr == nodes || !nodes->is_node)
    {
        LogError(ExcepCategory::Logic, "Null dirty item or dirty item is folder");
    }

    if (dirty_item.starred)
    {
        nodes->node->starred = panel_item.starred;
        g::pOptsManager->node_starred_save(nodes->node->get_id(), nodes->node->starred);
        rebuild_items();
    }
    else if (dirty_item.rclicked)
    {
        if (ImGui::IsKeyDown(ImGuiMod_Ctrl) || ImGui::IsKeyDown(ImGuiMod_Shift))
        {
            if (panel_item.selected)
                m_inspector->remove_inspecting_node(nodes->node);
            else
                m_inspector->add_inspecting_node(nodes->node);
            panel_item.selected = !panel_item.selected;
        }
        else
        {
            for (auto& item : m_panel_items)
            {
                item.selected = false;
            }
            m_inspector->reset_inspecting_nodes();
            m_inspector->add_inspecting_node(nodes->node);
            panel_item.selected = true;
        }

        if (panel_item.starred && g::pOptsManager->ui_nodes_pane_tree_view_load())
        {
            int32_t handled_items = 2;
            for (auto& item : m_panel_items)
            {
                if (panel_item.ptr == item.ptr)
                {
                    item.selected = panel_item.selected;
                    if (--handled_items <= 0)
                        break;
                }
            }
        }
    }
    else if (dirty_item.lclicked)
    {
        add_node_to_wf(nodes);
    }
    else
    {
        LogError(ExcepCategory::Logic, "Unhandled dirty state");
    }
}

void PanelNodes::on_alloc_tt_data(int32_t idx)
{
    if (m_panel_items[idx].can_expand || nullptr == m_panel_items[idx].ptr)
        return;

    if (m_tt_node != nullptr)
        m_wf->remove_node(m_tt_node);
    m_tt_idx = idx;
    m_tt_node = m_wf->add_node(((data::Nodes*)m_panel_items[idx].ptr)->node);
    m_tt_width = m_tt_node->calc_node_width();
}

void PanelNodes::on_release_tt_data(int32_t /*idx*/)
{
    if (m_tt_node != nullptr)
        m_wf->remove_node(m_tt_node);
    m_tt_node = nullptr;
    m_tt_idx = -1;
}

void PanelNodes::on_show_tt(int32_t idx)
{
    if (m_tt_node && idx == m_tt_idx)
        m_tt_node_drawer.draw(m_tt_node, true, m_tt_width);
}

void PanelNodes::on_pre_tt(int32_t /*idx*/)
{
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
}

void PanelNodes::on_post_tt(int32_t /*idx*/)
{
    ImGui::PopStyleColor(2);
}

void PanelNodes::add_node_to_wf(data::Nodes* /*holder*/)
{}

void PanelNodes::expand_all()
{
    rebuild_items(true);
}

void PanelNodes::collapse_all()
{
    for (auto& item : m_panel_items)
    {
        item.expanded = false;
    }
    rebuild_items();
}

void PanelNodes::show_cfg_buttons()
{
    ImGui::PushStyleColor(ImGuiCol_Button, data()->clr_bg);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, data()->clr_hover_btn);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, data()->clr_active_btn);

    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const uint32_t button_count = 4;
    const uint32_t big_spaces = 2;
    const uint32_t small_spaces = 1;
    const float start_pos =
        ImGui::GetContentRegionAvail().x -
        (button_count * data()->button_width) -
        (big_spaces * spacing * data()->ungrouped_btn_spacing_x) -
        (small_spaces * spacing * data()->grouped_btn_spacing_x) -
        spacing * 3;

    ImGui::SameLine(start_pos);

    const bool tree_view = g::pOptsManager->ui_nodes_pane_tree_view_load();
    if (!tree_view)
        ImGui::BeginDisabled();
    FontGuard fg(g::pMain->get_icons_font());

    const auto tooltip = [&fg](const char* text)
    {
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
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
    if (ImGui::Button(g::pOptsManager->ui_nodes_pane_tree_view_load() ? ICON_LC_FOLDER_TREE : ICON_LC_LIST_TREE))
    {
        g::pOptsManager->ui_nodes_pane_tree_view_save(!g::pOptsManager->ui_nodes_pane_tree_view_load());
        rebuild_items();
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
