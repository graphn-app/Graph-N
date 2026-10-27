/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "ui/panels/panel_nodes.h"
#include "ui/panels/panel_node_inspector.h"
#include "ui/graph-n_flow.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/workflows_manager.h"
#include "core/workflow/node.h"
#include "core/workflow/node_instance_ro.h"
#include "core/workflow/workflow.h"
#include "main.h"

#define TEST

namespace ui
{

PanelNodes::PanelNodes(PanelNodeInspector* inspector)
    : PanelBase(DlgObj::PanelNodes, "Node Library")
    , m_inspector(inspector)
    , m_tt_node_drawer(m_wf)
{
    g::pGraphN->subscribe_comfyui_validity_state(listener::ComfyUiObjectValidityListener::Step::Invalidate, this);
    g::pGraphN->subscribe_comfyui_validity_state(listener::ComfyUiObjectValidityListener::Step::ResolvePanels, this);
}

PanelNodes::~PanelNodes() noexcept
{
    if (g::pGraphN != nullptr)
        g::pGraphN->unsubscribe_comfyui_validity_state(this);
}

void PanelNodes::init()
{
    PanelBase::init();
    dlg_data()->buffer.resize(PanelNodesData::_buf_count);

    const auto wf_id = g::pWfsManager->create(false /*generate_name*/, false /*dump_to_db*/, true /*ro*/).id;
    m_wf = g::pWfsManager->access(wf_id);

    m_tt_callbacks.on_alloc_tt_data = [this](int32_t idx) { on_alloc_tt_data(idx); };
    m_tt_callbacks.on_release_tt_data = [this](int32_t idx) { on_release_tt_data(idx); };
    m_tt_callbacks.on_show_tt = [this](int32_t idx) { on_show_tt(idx); };
    m_tt_callbacks.on_pre_tt = [this](int32_t idx) noexcept { return on_pre_tt(idx); };
    m_tt_callbacks.on_post_tt = [this](int32_t idx) noexcept { on_post_tt(idx); };

    m_dnd_callbacks.on_dnd_allow = [this](int32_t idx) noexcept { return on_dnd_allow(idx); };
    m_dnd_callbacks.on_dnd_show = [this](int32_t idx) noexcept { on_dnd_show(idx); };
    m_dnd_callbacks.dnd_id = DND_FROM_NODES_LIBRARY;

    create_toolbar();
    rebuild_items();
}

void PanelNodes::create_toolbar()
{
    clear_toolbar();

    if (m_labels_mode)
    {
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Button;
            el.icon = ICON_LC_MOVE_LEFT;
            el.tooltip = "Back to Nodes library";
            el.on_click = [this](PanelToolbar::Element& el) { on_labels_mode(el); };
            add_toolbar_element(std::move(el), false);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Empty;
            add_toolbar_element(std::move(el), false);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Button;
            el.icon = ICON_LC_PLUS;
            el.tooltip = "Add new label";
            el.on_click = [this](PanelToolbar::Element& /*el*/)
            {
                m_new_label = g::pNodesManager->create_label();
                rebuild_items();
            };
            add_toolbar_element(std::move(el), false);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Empty;
            add_toolbar_element(std::move(el), false);
        }
    }
    else
    {
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Button;
            el.icon = ICON_LC_TAGS;
            el.tooltip = "Manage labels";
            el.on_click = [this](PanelToolbar::Element& el) { on_labels_mode(el); };
            add_toolbar_element(std::move(el), false);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Empty;
            add_toolbar_element(std::move(el), false);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Button;
            el.icon = ICON_LC_LIST_CHEVRONS_UP_DOWN;
            el.tooltip = "Expand all";
            el.on_click = [this](PanelToolbar::Element& /*el*/) { rebuild_items(true); };
            el.on_preview = [](PanelToolbar::Element& el) noexcept { el.enabled = (g::pOptsManager->ui_nodes_pane_tree_view_load() != mngr::SettingsManager::NodeViewType::plain); };
            add_toolbar_element(std::move(el), false);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Button;
            el.icon = ICON_LC_LIST_CHEVRONS_DOWN_UP;
            el.tooltip = "Collapse all";
            el.on_click = [this](PanelToolbar::Element& /*el*/)
            {
                for (auto& item : m_panel_items) { item.expanded = false; }
                rebuild_items();
            };
            el.on_preview = [](PanelToolbar::Element& el) noexcept { el.enabled = (g::pOptsManager->ui_nodes_pane_tree_view_load() != mngr::SettingsManager::NodeViewType::plain); };
            add_toolbar_element(std::move(el), false);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Button;
            const auto view = g::pOptsManager->ui_nodes_pane_tree_view_load();
            el.icon = (mngr::SettingsManager::NodeViewType::plain == view ? ICON_LC_LIST_TREE : (mngr::SettingsManager::NodeViewType::categories == view ? ICON_LC_FOLDER_TREE : ICON_LC_PACKAGE));
            el.tooltip = "Toggle Flat/Category/Module view";
            el.on_click = [this](PanelToolbar::Element& el) { on_view(el); };
            add_toolbar_element(std::move(el), false);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Empty;
            add_toolbar_element(std::move(el), false);
        }  
    }
    finalize_toolbar(false /*separator*/);
}

void PanelNodes::invalidate_comfyui_objects() noexcept
{
    m_panel_items.clear();
}

void PanelNodes::resolve_comfyui_objects(listener::ComfyUiObjectValidityListener::Step /*step*/)
{
    rebuild_items();
}

DlgCode PanelNodes::show(bool& focused)
{
    bool opened = true;
    if (begin_panel(opened, focused) && opened)
    {
        if (!m_labels_mode && g::pNodesManager->get_nodes_by_category().empty()) [[unlikely]]
        {
            show_empty_panel_descr(
                ICON_LC_INFO " Node Library: Browse, search, and add nodes to your workflow.",
                {
                    ICON_LC_UNPLUG,
                    "Waiting for connection.",
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
        if (m_labels_mode && g::pNodesManager->get_labels().empty()) [[unlikely]]
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

        if (!m_labels_mode) [[likely]]
        {
            ImGui::Spacing();

            if (m_appeared)
                ImGui::SetKeyboardFocusHere();

            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGuiEx::InputTextWithHint("##filternodes", "filter nodes", &dlg_data()->buffer[PanelNodesData::BuffFilter],
                ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoUndoRedo | ImGuiInputTextFlags_EscapeClearsAll))
            {
                dlg_data()->buffer[PanelNodesData::BuffFilterLowered] = tools::str::to_lower_ascii(dlg_data()->buffer[PanelNodesData::BuffFilter]);
                Log("Filter by: " << dlg_data()->buffer[PanelNodesData::BuffFilterLowered]);
                g::pNodesManager->calc_number_inside(tools::str::trim_this(dlg_data()->buffer[PanelNodesData::BuffFilterLowered]), {}, {}, true);
                rebuild_items(true);
            }
        }

        ImGuiEx::Spacing(3);
        if (ImGui::BeginChild(DlgObj_IID[cast::to_szt(DlgObj::PanelNodes_ItemsList)], ImVec2(ImGui::GetContentRegionAvail().x, 0))) [[likely]]
        {
            PanelItemEvent ev;
            if (draw_list(m_panel_items, ev, &m_tt_callbacks, &m_dnd_callbacks))
                handle_dirty(ev);
            draw_popups();
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

    const auto view = g::pOptsManager->ui_nodes_pane_tree_view_load();

    if (view != mngr::SettingsManager::NodeViewType::plain)
        g::pNodesManager->calc_number_inside(tools::str::trim_this(dlg_data()->buffer[PanelNodesData::BuffFilterLowered]), {}, {}, mngr::SettingsManager::NodeViewType::categories == view);

    std::set<data::NodeItem*> expanded_nodes;
    std::set<uint32_t> expanded_labels;
    for (auto& item : m_panel_items)
    {
        if (item.expanded)
        {
            if (item.ext_data.label != 0)
                expanded_labels.insert(item.ext_data.label);
            else
                expanded_nodes.insert(cast::ptrs<data::NodeItem>(item.ext_data.ptr));
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
        const auto nodes_in_label = g::pNodesManager->get_nodes_in_label(label.id);
        for (auto node_item : nodes_in_label)
        {
            if (!node_item->filtered)
                nodes_count++;
        }

        PanelListItem label_item;
        label_item.name = label.name;
        label_item.name_label = std::to_string(nodes_count);
        label_item.can_expand = true;
        label_item.expanded = (force_expand_all || expanded_labels.contains(label.id));
        label_item.icon = label_item.expanded ? ICON_LC_FOLDER_OPEN : ICON_LC_FOLDER;
        label_item.ext_data.label = label.id;
        m_panel_items.push_back(std::move(label_item));

        if (label_item.expanded)
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
                    item.icon_can_click = true;
                    item.selected = m_inspector->is_node_inspecting(node_item->node);
                    item.ext_data.ptr = node_item;
                    item.ext_data.dnd_data = node_item->node->get_hash();
                    m_panel_items.push_back(std::move(item));
                }
            }
        }
    }

    if (view != mngr::SettingsManager::NodeViewType::plain)
    {
        if (!labels.empty())
            add_separator(mngr::SettingsManager::NodeViewType::categories == view ? "Categories" : "Modules");

        std::function<void(const std::vector<data::NodeItem*>&, int32_t)> go_tree;
        go_tree = [&go_tree, &panel_items = m_panel_items, &expanded_nodes, force_expand_all, &inspector = m_inspector,
            is_filter_empty = dlg_data()->buffer[PanelNodesData::BuffFilter].empty()]
            (const std::vector<data::NodeItem*>& nodes, int32_t depth)
        {
            for (auto node : nodes)
            {
                bool show_node = !node->filtered;
                if (show_node && !node->is_node)
                    show_node = is_filter_empty || (!is_filter_empty && node->nodes_inside > 0);

                if (!show_node)
                    continue;

                PanelListItem item;
                item.depth = depth;
                item.name = node->name_or_category;
                item.ext_data.ptr = node;

                if (!node->is_node)
                {
                    item.name_label = std::to_string(node->nodes_inside);
                    item.can_expand = true;
                    item.expanded = (force_expand_all || expanded_nodes.contains(node));
                    item.icon = item.expanded ? ICON_LC_FOLDER_OPEN : ICON_LC_FOLDER;
                    panel_items.push_back(item);

                    if (item.expanded)
                        go_tree(node->children, depth + 1);
                }
                else
                {
                    item.selected = inspector->is_node_inspecting(node->node);
                    item.icon = ICON_LC_TAG;
                    item.icon_tooltip = "Manage node labels";
                    item.icon_can_click = true;
                    item.ext_data.dnd_data = node->node->get_hash();
                    panel_items.push_back(std::move(item));
                }
            }
        };
        if (mngr::SettingsManager::NodeViewType::categories == view)
            go_tree(g::pNodesManager->get_nodes_by_category(), 0);
        else
            go_tree(g::pNodesManager->get_nodes_by_module(), 0);
    }
    else
    {
        if (!labels.empty())
            add_separator("Plain view");

        for (auto node_item : g::pNodesManager->get_nodes_plain())
        {
            if (node_item->is_node && !node_item->filtered)
            {
                PanelListItem item;
                item.name = node_item->name_or_category;
                item.icon_can_click = true;
                item.icon = ICON_LC_TAG;
                item.icon_tooltip = "Manage node labels";
                item.selected = m_inspector->is_node_inspecting(node_item->node);
                item.ext_data.ptr = node_item;
                item.ext_data.dnd_data = node_item->node->get_hash();
                m_panel_items.push_back(std::move(item));
            }
        }
    }
}

void PanelNodes::handle_dirty(const PanelItemEvent& event)
{
    LogRAssert(event.index < m_panel_items.size(), "Index out of bounds", );

    if (m_labels_mode)
    {
        handle_dirty_labels(event);
        return;
    }

    auto& panel_item = m_panel_items[event.index];

    if (event.expand_toggled)
    {
        panel_item.expanded = !panel_item.expanded;
        rebuild_items();
        return;
    }

    data::NodeItem* node_item = cast::ptrs<data::NodeItem>(panel_item.ext_data.ptr);
    if (nullptr == node_item || !node_item->is_node)
        return;

    if (event.icon_clicked && event.l_released)
    {
        m_active_popup_ptr = panel_item.ext_data.ptr;
        m_active_popup_labels.clear();
        *m_new_label_buff = 0;

        auto& labels = g::pNodesManager->get_labels();
        for (auto& label : labels)
        {
            if (g::pNodesManager->get_is_node_in_label(node_item, label.id))
                m_active_popup_labels.insert(label.id);
        }

        ImGui::OpenPopup(DlgObj_IID[cast::to_szt(DlgObj::PanelNodes_ItemInLabels)]);
        return;
    }

    if (event.r_released)
    {
        if (ImGui::IsKeyDown(ImGuiMod_Ctrl) || ImGui::IsKeyDown(ImGuiMod_Shift))
        {
            m_inspector->add_inspecting_node(node_item->node);
            panel_item.selected = true;
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
        rebuild_items();
    }
    else if (event.l_released)
    {
        add_node_to_wf(node_item);
    }
}

void PanelNodes::rebuild_labels()
{
    auto& labels = g::pNodesManager->get_labels();

    m_panel_items.clear();
    m_panel_items.reserve(labels.size());

    for (auto& label : labels)
    {
        PanelListItem item;
        item.name = label.name;
        item.ext_data.label = label.id;

        if (m_new_label == label.id)
        {
            begin_rename(cast::to_i32(m_panel_items.size()), item.name);
            m_new_label = 0;
        }
        m_panel_items.push_back(std::move(item));
    }
}

void PanelNodes::handle_dirty_labels(const PanelItemEvent& event)
{
    auto& panel_item = m_panel_items[event.index];

    if (event.rename_completed)
    {
        if (g::pNodesManager->rename_label(panel_item.ext_data.label, event.new_name))
            panel_item.name = event.new_name;
        rebuild_items();
    }
    else if (event.r_released)
    {
        m_active_popup_label_id = panel_item.ext_data.label; 
        ImGui::OpenPopup(DlgObj_IID[cast::to_szt(DlgObj::PanelNodes_LabelMenu)]);
    }
    else if (event.l_released)
    {
        begin_rename(event.index, panel_item.name);
    }
}

void PanelNodes::draw_popups()
{
    ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(300, 400));
    if (m_labels_mode && ImGui::BeginPopup(DlgObj_IID[cast::to_szt(DlgObj::PanelNodes_LabelMenu)], ImGuiWindowFlags_NoMove))
    {
        bool close = false;
        const uint32_t current_label_id = m_active_popup_label_id;

        if (ImGui::MenuItem("Add new label"))
        {
            m_new_label = g::pNodesManager->create_label();
            close = true;
            rebuild_items();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Rename label"))
        {
            for (int i = 0; i < m_panel_items.size(); i++)
            {
                if (m_panel_items[i].ext_data.label == current_label_id)
                {
                    begin_rename(i, m_panel_items[i].name);
                    break;
                }
            }
            close = true;
        }
        if (ImGui::MenuItem("Delete label"))
        {
            g::pNodesManager->remove_label(current_label_id);
            close = true;
            rebuild_items();
        }

        if (close)
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    else if (!m_labels_mode && ImGui::BeginPopup(DlgObj_IID[cast::to_szt(DlgObj::PanelNodes_ItemInLabels)], ImGuiWindowFlags_NoMove))
    {
        ImGui::TextDisabled("Assign label(s):");

        bool changed = false;
        auto& labels = g::pNodesManager->get_labels();

        for (auto& label : labels)
        {
            bool in_label = m_active_popup_labels.contains(label.id);
            ImGui::PushID(label.id);
            if (ImGui::Checkbox(label.name.c_str(), &in_label))
            {
                changed = true;
                if (in_label) m_active_popup_labels.insert(label.id);
                else          m_active_popup_labels.erase(label.id);

                g::pNodesManager->set_node_labels(cast::ptrs<data::NodeItem>(m_active_popup_ptr), m_active_popup_labels);
                rebuild_items();
            }
            ImGui::PopID();
        }

        ImGui::Spacing();
        ImGui::TextDisabled("New label:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::InputText("##newlabel", m_new_label_buff, 30, ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::IsItemDeactivatedAfterEdit())
        {
            if (*m_new_label_buff != 0 && g::pNodesManager->find_label(m_new_label_buff) == 0)
            {
                const uint32_t new_id = g::pNodesManager->create_label(m_new_label_buff);
                m_active_popup_labels.insert(new_id);
                g::pNodesManager->set_node_labels(cast::ptrs<data::NodeItem>(m_active_popup_ptr), m_active_popup_labels);
                *m_new_label_buff = 0;
                rebuild_items();
            }
        }
        ImGui::EndPopup();
    }
}

void PanelNodes::on_alloc_tt_data(int32_t idx)
{
    if (m_panel_items[idx].can_expand || nullptr == m_panel_items[idx].ext_data.ptr)
        return;

    if (m_tt_node != nullptr)
        m_wf->remove_node(m_tt_node);
    m_tt_idx = idx;
    m_tt_node = m_wf->add_node_ro(cast::ptrs<data::NodeItem>(m_panel_items[idx].ext_data.ptr)->node);
    m_tt_width = m_tt_node->calc_node_width(true, false);
}

void PanelNodes::on_release_tt_data(int32_t /*idx*/)
{
    if (m_tt_node != nullptr)
        m_wf->remove_node(m_tt_node);
    m_tt_node = nullptr;
    m_tt_idx = -1;
}

bool PanelNodes::on_pre_tt(int32_t /*idx*/) noexcept
{
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
    return true;
}

void PanelNodes::on_show_tt(int32_t idx)
{
    if (m_tt_node && idx == m_tt_idx)
        m_tt_node_drawer.draw(m_tt_node, true, m_tt_width);
}

void PanelNodes::on_post_tt(int32_t /*idx*/) noexcept
{
    ImGui::PopStyleColor(2);
}

bool PanelNodes::on_dnd_allow(int32_t idx) noexcept
{
    if (m_panel_items[idx].can_expand || nullptr == m_panel_items[idx].ext_data.ptr)
        return false;
    return true;
}

void PanelNodes::on_dnd_show(int32_t idx) noexcept
{
    if (m_panel_items[idx].can_expand || nullptr == m_panel_items[idx].ext_data.ptr)
        return;

    const auto node_item = cast::ptrs<data::NodeItem>(m_panel_items[idx].ext_data.ptr);
    ImGui::TextUnformatted(node_item->name_or_category.c_str(), node_item->name_or_category.c_str() + node_item->name_or_category.size());
}

void PanelNodes::add_node_to_wf(data::NodeItem* node_item)
{
    g::pFlow->wf_add_node_to_active_panel(node_item->node);
}

void PanelNodes::on_labels_mode(PanelToolbar::Element& /*el*/)
{
    m_labels_mode = !m_labels_mode;
    create_toolbar();
    rebuild_items();
    reset_toolbar_state();
}

void PanelNodes::on_view(PanelToolbar::Element& el)
{
    uint8_t view = cast::to_u8(g::pOptsManager->ui_nodes_pane_tree_view_load());
    if (++view > mngr::SettingsManager::NodeViewType::_last)
        view = mngr::SettingsManager::NodeViewType::_first;

    el.icon = (mngr::SettingsManager::NodeViewType::plain == view ? ICON_LC_LIST_TREE : (mngr::SettingsManager::NodeViewType::categories == view ? ICON_LC_FOLDER_TREE : ICON_LC_PACKAGE));
    g::pOptsManager->ui_nodes_pane_tree_view_save(cast::to<mngr::SettingsManager::NodeViewType>(view));

    rebuild_items();
}

} // namespace ui
