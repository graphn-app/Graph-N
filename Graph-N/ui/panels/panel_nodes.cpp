
#include "ui/panels/panel_nodes.h"
#include "ui/panels/panel_node_inspector.h"
#include "ui/graph-n_flow.h"
#include "main.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/workflows_manager.h"
#include "core/data/node.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/workflow.h"

#define TEST

namespace ui
{

PanelNodes::PanelNodes(PanelNodeInspector* inspector)
    : PanelBase(DlgObj::PanelNodes, ICON_LC_BOOK_OPEN " Node Library", ICON_LC_BOOK_OPEN, DlgObjID[cast::to_szt(DlgObj::PanelNodes)])
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

    auto wf_id = g::pWfsManager->create(true /*skip_name*/).id;
    m_wf = g::pWfsManager->access(wf_id);
    g::pWfsManager->set_read_only(wf_id);

    g::pFlow->subscribe_comfyui_validity_state(this);

    m_tt_callbacks.on_alloc_tt_data = std::bind(&PanelNodes::on_alloc_tt_data, this, std::placeholders::_1);
    m_tt_callbacks.on_release_tt_data = std::bind(&PanelNodes::on_release_tt_data, this, std::placeholders::_1);
    m_tt_callbacks.on_show_tt = std::bind(&PanelNodes::on_show_tt, this, std::placeholders::_1);
    m_tt_callbacks.on_pre_tt = std::bind(&PanelNodes::on_pre_tt, this, std::placeholders::_1);
    m_tt_callbacks.on_post_tt = std::bind(&PanelNodes::on_post_tt, this, std::placeholders::_1);

    create_toolbar();
    rebuild_items();
}

void PanelNodes::create_toolbar()
{
    clear_toolbar();

    if (m_labels_mode)
    {
        {
            PanelToolbar::Button button;
            button.icon = ICON_LC_MOVE_LEFT;
            button.tooltip = "Back to Nodes library";
            button.on_click = std::bind(&PanelNodes::on_labels_mode, this, std::placeholders::_1);
            add_button(std::move(button));
        }
        {
            PanelToolbar::Button button;
            button.empty_indent = true;
            add_button(std::move(button));
        }
        {
            PanelToolbar::Button button;
            button.icon = ICON_LC_PLUS;
            button.tooltip = "Add new label";
            button.on_click = std::bind(&PanelNodes::on_new_label, this, std::placeholders::_1);
            add_button(std::move(button));
        }
    }
    else
    {
        {
            PanelToolbar::Button button;
            button.icon = ICON_LC_TAGS;
            button.tooltip = "Manage Labels";
            button.on_click = std::bind(&PanelNodes::on_labels_mode, this, std::placeholders::_1);
            add_button(std::move(button));
        }
        {
            PanelToolbar::Button button;
            button.empty_indent = true;
            add_button(std::move(button));
        }
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
            add_button(std::move(button));
        }
        {
            PanelToolbar::Button button;
            button.empty_indent = true;
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
    }
    finalize_toolbar();
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
        if (!m_labels_mode && g::pNodesManager->get_nodes_tree().empty())
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
                    g::pFlow->open_settings_dlg(OptPane::Connection);
                }
            );
            return end_panel(true);
        }
        if (m_labels_mode && g::pNodesManager->get_labels().empty())
        {
            draw_toolbar();
            show_empty_panel_descr(
                "",
                {
                    ICON_LC_TAGS,
                    "Organize your nodes using labels.",
                    "A single node can belong",
                    "to several labels at once.",
                    "",
                    "Use the context menu, double-click",
                    "or press Del, F2",
                    "to manage them.",
                },
                {
                    "To get started,",
                },
                "click here",
                [this]()
                {
                    m_new_label = g::pNodesManager->create_label();
                    rebuild_items();
                }
            );
            return end_panel(true);
        }

        draw_toolbar();
        ImGui::Spacing();

        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGuiEx::InputTextWithHint("##filternodes", "filter nodes", &data()->buffer[PanelNodesData::BuffFilter],
            ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoUndoRedo | ImGuiInputTextFlags_EscapeClearsAll))
        {
            data()->buffer[PanelNodesData::BuffFilterLowered] = tools::str::to_lower_ascii(data()->buffer[PanelNodesData::BuffFilter]);
            Log("Filter by: " << data()->buffer[PanelNodesData::BuffFilterLowered]);
            g::pNodesManager->calc_number_inside(tools::str::trim_this(data()->buffer[PanelNodesData::BuffFilterLowered]));
            rebuild_items(true);
        }

        ImGuiEx::Spacing(3);
        if (ImGui::BeginChild(DlgObjID[cast::to_szt(DlgObj::PanelNodes_ItemsList)], ImVec2(ImGui::GetContentRegionAvail().x, 0)))
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
    if (m_labels_mode)
    {
        rebuild_labels();
        return;
    }

    LogF;

    struct NodeLabelState : public PanelListItem::State
    {
        std::set<uint32_t> active_labels;
    };

    const auto on_icon_handler = [&labels_mode = m_labels_mode]
        (PanelListItem& item, bool /*l_clicked*/, bool /*l_dbl_clicked*/, bool l_released, bool /*r_clicked*/, bool r_released, bool& handler_processed)
    {        
        bool dirty = false;
        auto& labels = g::pNodesManager->get_labels();
        if (!item.continue_processing_ico_handler && (l_released || r_released))
        {
            if (labels.empty())
            {
                item.continue_processing_ico_handler = false;
                labels_mode = true;
                return true;
            }

            handler_processed = true;
            item.state = std::make_unique<NodeLabelState>();
            item.state->ptr = item.ptr;
            NodeLabelState* state = cast::ptrs<NodeLabelState>(item.state.get());
            for (auto& label : labels)
            {
                if (g::pNodesManager->get_is_node_in_label(cast::ptrs<data::NodeItem>(item.ptr), label.second))
                {
                    state->active_labels.insert(label.second);
                }
            }

            ImGui::OpenPopup(DlgObjID[cast::to_szt(DlgObj::PanelNodes_LabelsMenu)]);
        }

        ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(ImGui::GetContentRegionAvail().x, 200));
        if (ImGui::BeginPopup(DlgObjID[cast::to_szt(DlgObj::PanelNodes_LabelsMenu)]))
        {
            handler_processed = true;
            item.continue_processing_ico_handler = true;

            ImGui::TextDisabled("Add/remove labels:");
            ImGui::Separator();

            for (auto& label : labels)
            {
                NodeLabelState* state = ((NodeLabelState*)item.state.get());
                bool node_in_label = (state->active_labels.count(label.second) > 0);
                    
                ImGui::PushID(label.second);
                if (ImGui::Checkbox(label.first.c_str(), &node_in_label))
                {
                    item.state->changed = true;
                    if (node_in_label)
                        state->active_labels.insert(label.second);
                    else
                        state->active_labels.erase(label.second);
                }
                ImGui::PopID();
            }

            ImGui::Separator();
            if (!item.state->changed)
                ImGui::BeginDisabled();
            if (ImGui::Button("Apply", ImVec2(-FLT_MIN, 0)))
            {
                item.continue_processing_ico_handler = false;
                ImGui::CloseCurrentPopup();

                if (item.state->changed && item.state->ptr == item.ptr)
                {
                    dirty = true;
                    g::pNodesManager->set_node_labels(cast::ptrs<data::NodeItem>(item.ptr), cast::ptrs<NodeLabelState>(item.state.get())->active_labels);
                }
            }
            if (!item.state->changed)
                ImGui::EndDisabled();

            ImGui::EndPopup();
        }
        else
            item.continue_processing_ico_handler = false;

        if (!item.continue_processing_ico_handler)
            item.state.reset();
        return dirty;
    };

    std::set<data::NodeItem*> expanded_nodes;
    std::set<uint32_t> expanded_labels;
    for (auto& item : m_panel_items)
    {
        if (item.expanded)
        {
            if (item.label != 0)
                expanded_labels.insert(item.label);
            else
                expanded_nodes.insert(cast::ptrs<data::NodeItem>(item.ptr));
        }
    }

    auto& labels = g::pNodesManager->get_labels();

    m_panel_items.clear();
    m_panel_items.reserve(labels.size() + g::pNodesManager->get_node_items_total());

    const auto add_separator = [&items = m_panel_items](const char* sep_name)
    {
        PanelListItem item;
        item.is_separator = true;
        item.name = sep_name;
        items.push_back(std::move(item));
    };

    if (!labels.empty())
        add_separator("Labels");

    for (auto& label : labels)
    {
        size_t nodes_count = 0;
        auto& nodes_in_label = g::pNodesManager->get_nodes_in_label(label.second);
        for (auto node_item : nodes_in_label)
        {
            if (!node_item->filtered)
                nodes_count++;
        }

        PanelListItem label_item;
        label_item.name = label.first;
        label_item.name_p2 = std::to_string(nodes_count);
        label_item.can_expand = true;
        label_item.expanded = (force_expand_all || expanded_labels.count(label.second) > 0);
        const bool label_expanded = label_item.expanded;
        label_item.icon = label_item.expanded ? ICON_LC_FOLDER_OPEN : ICON_LC_FOLDER;
        label_item.label = label.second;
        m_panel_items.push_back(std::move(label_item));

        if (label_expanded)
        {
            for (auto node_item : nodes_in_label)
            {
                if (!node_item->filtered)
                {
                    PanelListItem item;
                    item.depth = 1;
                    item.name = node_item->name_or_category;
                    item.icon = ICON_LC_TAG;
                    item.icon_tooltip = "Manage node labels";
                    item.on_icon_handler = on_icon_handler;
                    item.can_lclick = item.can_rclick = item.icon_can_click = true;
                    item.selected = m_inspector->is_node_inspecting(node_item->node);
                    item.ptr = node_item;
                    m_panel_items.push_back(std::move(item));
                }
            }
        }
    }

    if (g::pOptsManager->ui_nodes_pane_tree_view_load())
    {
        if (!labels.empty())
            add_separator("Folders");

        std::function<void(const std::vector<data::NodeItem*>&, int32_t)> go_tree;
        go_tree = [&go_tree, &on_icon_handler, &labels, &panel_items = m_panel_items, &expanded_nodes, force_expand_all, &inspector = m_inspector,
            is_filter_empty = data()->buffer[PanelNodesData::BuffFilter].empty()]
            (const std::vector<data::NodeItem*>& nodes, int32_t depth)
        {
            for (auto node : nodes)
            {
                bool show_node = !node->filtered;
                if (show_node && !node->is_node)
                    show_node = is_filter_empty || !is_filter_empty && node->nodes_inside > 0;

                if (!show_node)
                    continue;

                if (!node->is_node)
                {
                    PanelListItem item;
                    item.depth = depth;
                    item.name = node->name_or_category;
                    item.name_p2 = std::to_string(node->nodes_inside);
                    item.can_expand = true;
                    item.expanded = force_expand_all || expanded_nodes.count(node) > 0;
                    item.icon = item.expanded ? ICON_LC_FOLDER_OPEN : ICON_LC_FOLDER;
                    const bool item_expanded = item.expanded;
                    item.ptr = node;
#ifdef TEST
                    item.can_rclick = true;
#endif
                    panel_items.push_back(std::move(item));

                    if (item_expanded)
                        go_tree(node->children, depth + 1);
                }
                else
                {
                    PanelListItem item;
                    item.depth = depth;
                    item.selected = inspector->is_node_inspecting(node->node);
                    item.name = node->name_or_category;
                    item.icon = ICON_LC_TAG;
                    item.icon_tooltip = "Manage node labels";
                    item.on_icon_handler = on_icon_handler;
                    item.can_lclick = item.can_rclick = item.icon_can_click = true;
                    item.ptr = node;
                    panel_items.push_back(std::move(item));
                }
            }
        };

        go_tree(g::pNodesManager->get_nodes_tree(), 0);
    }
    else
    {
        if (!labels.empty())
            add_separator("Plain view");

        auto& plain = g::pNodesManager->get_nodes_plain();

        for (auto node_item : plain)
        {
            if (node_item->is_node && !node_item->filtered)
            {
                PanelListItem item;
                item.name = node_item->name_or_category;
                item.can_lclick = item.can_rclick = item.icon_can_click = true;
                item.on_icon_handler = on_icon_handler;
                item.icon = ICON_LC_TAG;
                item.icon_tooltip = "Manage node labels";
                item.selected = m_inspector->is_node_inspecting(node_item->node);
                item.ptr = node_item;
                m_panel_items.push_back(std::move(item));
            }
        }
    }
}

void PanelNodes::handle_dirty(const PanelItemState& dirty_item)
{
    if (dirty_item.index >= m_panel_items.size())
    {
        LogError(ExcepCategory::Logic, "Index out of bounds");
    }

    if (dirty_item.expanded)
    {
        rebuild_items();
        return;
    }

    if (m_labels_mode)
    {
        handle_dirty_labels(dirty_item);
        return;
    }

    Log("Dirty: idx=" << dirty_item.index << ", rc=" << dirty_item.rclicked << " lc=" << dirty_item.lclicked << ", ico=" << dirty_item.icon_clicked << ", xp=" << dirty_item.expanded);

    auto& panel_item = m_panel_items[dirty_item.index];
    data::NodeItem* node_item = cast::ptrs<data::NodeItem>(panel_item.ptr);

#ifndef TEST
    if (nullptr == node_item || !node_item->is_node)
    {
        LogError(ExcepCategory::Logic, "Null dirty item or dirty item is folder");
    }
#endif

    if (dirty_item.icon_clicked)
    {
        rebuild_items();
    }
    else if (dirty_item.rclicked)
    {
#ifdef TEST
        if (panel_item.can_expand && !node_item->is_node)
        {
            m_inspector->reset_inspecting_nodes();
            for (auto& unsel_panel_item : m_panel_items)
            {
                unsel_panel_item.selected = false;
            }
            for (auto child : node_item->children)
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
                m_inspector->remove_inspecting_node(node_item->node);
            else
                m_inspector->add_inspecting_node(node_item->node);
            panel_item.selected = !panel_item.selected;
        }
        else
        {
            for (auto& item : m_panel_items)
            {
                item.selected = false;
            }
            m_inspector->reset_inspecting_nodes();
            m_inspector->add_inspecting_node(node_item->node);
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
        add_node_to_wf(node_item);
    }
    else
    {
        LogError(ExcepCategory::Logic, "Unhandled dirty state");
    }
}

void PanelNodes::rebuild_labels()
{
    LogF;

    const auto on_hover_handler = []
        (PanelListItem& item, bool& handler_processed)
    {
        bool dirty = false;
        if (ImGui::IsKeyPressed(ImGuiKey_Delete))
        {
            g::pNodesManager->remove_label(item.label);
            dirty = true;
            handler_processed = true;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_F2))
        {
            item.name_backup = item.name;
            item.renaming = true;
            item.need_focus = true;
            handler_processed = true;
        }

        return dirty;
    };

    const auto on_item_handler = [&new_label_id = m_new_label]
        (PanelListItem& item, bool /*l_clicked*/, bool /*l_dbl_clicked*/, bool /*l_released*/, bool /*r_clicked*/, bool r_released, bool& handler_processed)
    {
        bool dirty = false, close = false;
        if (!item.continue_processing_item_handler && r_released)
        {
            ImGui::OpenPopup(DlgObjID[cast::to_szt(DlgObj::PanelNodes_LabelsMenu)]);
            handler_processed = true;
        }

        ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(ImGui::GetContentRegionAvail().x, 200));
        if (ImGui::BeginPopup(DlgObjID[cast::to_szt(DlgObj::PanelNodes_LabelsMenu)]))
        {
            handler_processed = true;
            if (ImGui::MenuItem("Add new label"))
            {
                new_label_id = g::pNodesManager->create_label();
                close = dirty = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Rename label"))
            {
                item.name_backup = item.name;
                item.renaming = true;
                item.need_focus = true;
                close = true;
            }
            if (ImGui::MenuItem("Delete label"))
            {
                g::pNodesManager->remove_label(item.label);
                close = dirty = true;
            }

            item.continue_processing_item_handler = !close;
            if (close)
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return dirty;
        }
        else
            item.continue_processing_item_handler = false;

        return dirty;
    };

    auto& labels = g::pNodesManager->get_labels();

    m_panel_items.clear();
    m_panel_items.reserve(labels.size());

    for (auto& label : labels)
    {
        PanelListItem item;
        item.can_dblclick = true;
        item.name = label.first;
        item.label = label.second;
        item.on_item_handler = on_item_handler;
        item.on_hover_handler = on_hover_handler;
        if (m_new_label == label.second)
        {
            item.renaming = true;
            item.need_focus = true;
            item.name_backup = item.name;
            m_new_label = 0;
        }
        m_panel_items.push_back(std::move(item));
    }
}

void PanelNodes::handle_dirty_labels(const PanelItemState& dirty_item)
{
    Log("Dirty label: idx=" << dirty_item.index << ", rc=" << dirty_item.rclicked << " lc=" << dirty_item.lclicked << ", ico=" << dirty_item.icon_clicked << ", xp=" << dirty_item.expanded);

    if (dirty_item.dblclicked)
    {
        if (dirty_item.index >= m_panel_items.size())
        {
            LogError(ExcepCategory::Logic, "Index out of bounds");
        }

        auto& panel_item = m_panel_items[dirty_item.index];
        panel_item.renaming = true;
        panel_item.need_focus = true;
        panel_item.name_backup = panel_item.name;
    }
    else if (dirty_item.item_clicked || dirty_item.item_handled)
    {
        rebuild_items();
    }
    else if (dirty_item.renamed)
    {
        auto& panel_item = m_panel_items[dirty_item.index];
        if (!g::pNodesManager->rename_label(panel_item.label, panel_item.name))
            panel_item.name = panel_item.name_backup;
        panel_item.name_backup = {};
        rebuild_items();
    }
}

void PanelNodes::on_alloc_tt_data(int32_t idx)
{
    if (m_panel_items[idx].can_expand || nullptr == m_panel_items[idx].ptr)
        return;

    if (m_tt_node != nullptr)
        m_wf->remove_node(m_tt_node);
    m_tt_idx = idx;
    m_tt_node = m_wf->add_node(cast::ptrs<data::NodeItem>(m_panel_items[idx].ptr)->node);
    m_tt_width = m_tt_node->calc_node_width(true /*ro*/);
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

bool PanelNodes::on_pre_tt(int32_t /*idx*/)
{
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
    return true;
}

void PanelNodes::on_post_tt(int32_t /*idx*/)
{
    ImGui::PopStyleColor(2);
}

void PanelNodes::add_node_to_wf(data::NodeItem* node_item)
{
    g::pFlow->wf_add_node_to_active_panel(node_item->node);
}

void PanelNodes::on_new_label(PanelToolbar::Button& /*button*/)
{
    m_new_label = g::pNodesManager->create_label();
    rebuild_items();
}

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

void PanelNodes::on_labels_mode(PanelToolbar::Button& button)
{
    m_labels_mode = !m_labels_mode;
    create_toolbar();
    rebuild_items();
    button.reset_state = true;
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
