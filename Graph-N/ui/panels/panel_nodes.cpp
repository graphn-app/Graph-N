
#include "ui/panels/panel_nodes.h"
#include "ui/panels/panel_node_inspector.h"
#include "ui/graph-n_flow.h"
#include "ui/font_guard.h"
#include "platform/main.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/workflows_manager.h"
#include "core/data/node.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/workflow.h"

#define TEST

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

    auto wf_id = g::pWfsManager->create().id;
    m_wf = g::pWfsManager->access(wf_id);
    g::pWfsManager->set_read_only(wf_id);

    g::pFlow->subscribe_comfyui_validity_state(this);

    {
        PanelToolbar::Button button;
        button.icon = ICON_LC_LIST_CHEVRONS_UP_DOWN;
        button.tooltip = "Expand All";
        button.on_click = std::bind(&PanelNodes::on_expand_all, this, std::placeholders::_1);
        button.on_preview = std::bind(&PanelNodes::on_pre_expand_collapse, this, std::placeholders::_1);
        add_button(std::move(button));
    }
    {
        PanelToolbar::Button button;
        button.icon = ICON_LC_LIST_CHEVRONS_DOWN_UP;
        button.tooltip = "Collapse All";
        button.on_click = std::bind(&PanelNodes::on_collapse_all, this, std::placeholders::_1);
        button.on_preview = std::bind(&PanelNodes::on_pre_expand_collapse, this, std::placeholders::_1);
        add_button(std::move(button));
    }
    {
        PanelToolbar::Button button;
        button.icon = g::pOptsManager->ui_nodes_pane_tree_view_load() ? ICON_LC_FOLDER_TREE : ICON_LC_LIST_TREE;
        button.tooltip = "Toggle Flat/Category View";
        button.on_click = std::bind(&PanelNodes::on_view, this, std::placeholders::_1);
        button.long_indent_after = true;
        add_button(std::move(button));
    }
    {
        PanelToolbar::Button button;
        button.icon = ICON_LC_FOLDER_SYNC;
        button.tooltip = "Refresh Node List";
        button.on_click = std::bind(&PanelNodes::on_sync, this, std::placeholders::_1);
        button.on_preview = std::bind(&PanelNodes::on_pre_sync, this, std::placeholders::_1);
        add_button(std::move(button));
    }
    finalize_toolbar();

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
    if (begin_panel(opened, focused) && opened)
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

        draw_toolbar();
        ImGui::Spacing();

        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGuiEx::InputTextWithHint("##filter nodes", "filter nodes", &data()->buffer[PanelNodesData::BuffFilter],
            ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoUndoRedo | ImGuiInputTextFlags_EscapeClearsAll))
        {
            data()->buffer[PanelNodesData::BuffFilterLowered] = tools::str::to_lower_ascii(data()->buffer[PanelNodesData::BuffFilter]);
            Log("Filter by: " << data()->buffer[PanelNodesData::BuffFilterLowered]);
            g::pNodesManager->calc_number_inside(tools::str::trim_this(data()->buffer[PanelNodesData::BuffFilterLowered]));
            rebuild_items(true);
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

    // save expanded state
    std::set<data::Nodes*> expanded;
    for (auto& item : m_panel_items)
    {
        if (item.expanded)
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
            if (nodes->is_node && nodes->node->starred && !nodes->filtered)
            {
                fav_items++;
                if (!fav_folder_expanded && !force_expand_all)
                    continue;
                PanelListItem item;
                item.depth = 1;
                item.name = nodes->name_or_category;
                item.icon = ICON_LC_STAR;
                item.icon_can_click = item.icon_active = true;
                item.can_lclick = item.can_rclick = true;
                item.selected = m_inspector->is_node_inspecting(nodes->node);
                item.ptr = nodes;
                m_panel_items.push_back(std::move(item));
            }
        }

        m_panel_items.reserve(m_panel_items.size() + g::pNodesManager->get_node_items_total() + 2);

        if (fav_items > 0)
        {
            PanelListItem star_folder;
            star_folder.name = fav_folder_name + " [" + std::to_string(fav_items) + "]";
            star_folder.can_expand = true;
            star_folder.icon = (force_expand_all || fav_folder_expanded ? ICON_LC_FOLDER_OPEN : ICON_LC_FOLDER);
            star_folder.expanded = force_expand_all || fav_folder_expanded;
            m_panel_items.insert(m_panel_items.begin(), std::move(star_folder));

            PanelListItem separator;
            separator.is_separator = true;
            m_panel_items.push_back(std::move(separator));
        }

        std::function<void(const std::vector<data::Nodes*>&, int32_t)> go_tree;
        go_tree = [&go_tree, &panel_items = m_panel_items, &expanded, force_expand_all, &inspector = m_inspector](const std::vector<data::Nodes*>& nodes, int32_t depth)
        {
            for (auto node : nodes)
            {
                if (!node->is_node)
                {
                    PanelListItem item;
                    item.depth = depth;
                    item.name = node->name_or_category;
                    item.can_expand = true;
                    item.icon = (force_expand_all || expanded.count(node) > 0 ? ICON_LC_FOLDER_OPEN : ICON_LC_FOLDER);
                    item.expanded = force_expand_all || expanded.count(node) > 0;
                    const bool item_expanded = item.expanded;
                    item.ptr = node;
#ifdef TEST
                    item.can_rclick = true;
#endif
                    panel_items.push_back(std::move(item));

                    if (item_expanded)
                        go_tree(node->children, depth + 1);
                }
                else if (!node->filtered)
                {
                    PanelListItem item;
                    item.depth = depth;
                    item.selected = inspector->is_node_inspecting(node->node);
                    item.name = node->name_or_category;
                    item.icon = ICON_LC_STAR;
                    item.icon_can_click = true;
                    item.icon_active = node->node->starred;
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
            if (nodes->is_node && nodes->node->starred && !nodes->filtered)
            {
                PanelListItem item;
                item.name = nodes->name_or_category;
                item.icon_can_click = item.icon_active = true;
                item.icon = ICON_LC_STAR;
                item.can_lclick = item.can_rclick = true;
                item.selected = m_inspector->is_node_inspecting(nodes->node);
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
            if (nodes->is_node && !nodes->node->starred && !nodes->filtered)
            {
                PanelListItem item;
                item.name = nodes->name_or_category;
                item.icon_can_click = item.can_lclick = item.can_rclick = true;
                item.icon = ICON_LC_STAR;
                item.selected = m_inspector->is_node_inspecting(nodes->node);
                item.ptr = nodes;
                m_panel_items.push_back(std::move(item));
            }
        }
    }
}

void PanelNodes::handle_dirty(const PanelItemState& dirty_item)
{
    Log("Dirty: idx=" << dirty_item.index << ", rc=" << dirty_item.rclicked << " lc=" << dirty_item.lclicked << ", ico=" << dirty_item.icon_clicked << ", xp=" << dirty_item.expanded);

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

#ifndef TEST
    if (nullptr == nodes || !nodes->is_node)
    {
        LogError(ExcepCategory::Logic, "Null dirty item or dirty item is folder");
    }
#endif

    if (dirty_item.icon_clicked)
    {
        nodes->node->starred = panel_item.icon_active;
        g::pOptsManager->node_starred_save(nodes->node->get_id(), nodes->node->starred);
        rebuild_items();
    }
    else if (dirty_item.rclicked)
    {
#ifdef TEST
        if (panel_item.can_expand && !nodes->is_node)
        {
            m_inspector->reset_inspecting_nodes();
            for (auto& unsel_panel_item : m_panel_items)
            {
                unsel_panel_item.selected = false;
            }
            for (auto child : nodes->children)
            {
                if (child->is_node)
                {
                    m_inspector->add_inspecting_node(child->node);
                    for (auto& sel_panel_item : m_panel_items)
                    {
                        if (sel_panel_item.ptr == child)
                        {
                            sel_panel_item.selected = true;
                            break;
                        }
                    }
                }
            }
            return;
        }
#endif

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

        if (panel_item.icon_active && g::pOptsManager->ui_nodes_pane_tree_view_load())
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

void PanelNodes::on_expand_all(PanelToolbar::Button& /*button*/)
{
    rebuild_items(true);
}

void PanelNodes::on_collapse_all(PanelToolbar::Button& /*button*/)
{
    for (auto& item : m_panel_items)
    {
        item.expanded = false;
    }
    rebuild_items();
}

void PanelNodes::on_pre_expand_collapse(PanelToolbar::Button& button)
{
    button.enabled = g::pOptsManager->ui_nodes_pane_tree_view_load();
}

void PanelNodes::on_view(PanelToolbar::Button& button)
{
    auto tree_view = g::pOptsManager->ui_nodes_pane_tree_view_load();
    tree_view = !tree_view;
    g::pOptsManager->ui_nodes_pane_tree_view_save(tree_view);

    rebuild_items();

    if (tree_view)
        button.icon = ICON_LC_FOLDER_TREE;
    else
        button.icon = ICON_LC_LIST_TREE;
}

void PanelNodes::on_sync(PanelToolbar::Button& /*button*/)
{
    g::pFlow->reconnect();
}

void PanelNodes::on_pre_sync(PanelToolbar::Button& button)
{
    button.enabled = g::pFlow->get_online();
}

} // namespace ui
