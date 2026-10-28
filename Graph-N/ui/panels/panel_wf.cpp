/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "ui/panels/panel_wf.h"
#include "ui/graph-n_flow.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/history_manager.h"
#include "core/managers/queue_manager.h"
#include "core/managers/hotkeys_manager.h"
#include "core/workflow/workflow.h"
#include "core/workflow/workflow_action.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/workflow/custom_nodes/cni_reroute.h"

namespace ui
{

PanelWorkflow::PanelWorkflow(wid workflow_id, const std::string& name, const std::string& panel_id, ImGuiID dock)
    : PanelBase(DlgObj::PanelWf, name, panel_id)
    , m_wf(g::pWfsManager->access(workflow_id))
    , m_dock(dock)
    , m_action(m_wf->action())
    , m_drawer(m_wf)
    , m_context_menuid(cast::to_u32(base::GlobalID::random_widget(0xff, 0xfffffff)))
{
    m_action->set_drawer(&m_drawer);
    m_wf_valid = m_wf->get_data().valid;
}

PanelWorkflow::~PanelWorkflow() noexcept = default;

void PanelWorkflow::stop_and_cleanup() noexcept
{
    dead();

    if (g::pGraphN != nullptr)
        g::pGraphN->unsubscribe_comfyui_validity_state(this);

    if (m_wf != nullptr)
    {
        m_wf->get_data().activated = false;        
        m_wf = nullptr;
        if (m_action != nullptr)
            m_action->set_drawer(nullptr);
        m_drawer.stop_and_cleanup();
    }
}

void PanelWorkflow::init()
{
    PanelBase::init();

    blob::Blob drawer_state;
    g::pWfsManager->load(m_wf->get_id(), drawer_state);
    m_drawer.init(std::move(drawer_state));

    g::pGraphN->subscribe_comfyui_validity_state(listener::ComfyUiObjectValidityListener::Step::Invalidate, this);
    g::pGraphN->subscribe_comfyui_validity_state(listener::ComfyUiObjectValidityListener::Step::ResolvePanels, this);
    g::pHotkeysManager->add_listener(this);

    m_wf->get_data().activated = true;
    m_wf_valid = m_wf->get_data().valid;
    create_toolbar();
    alive();
}

void PanelWorkflow::cleanup_frame()
{
    if (!m_pending_cleanups.empty())
    {
        delete_nodes(std::move(m_pending_cleanups), true);
        m_pending_cleanups = {};
    }
}

void PanelWorkflow::invalidate_comfyui_objects() noexcept
{
    m_invalidation_state = true;
    dead();
}

void PanelWorkflow::resolve_comfyui_objects(listener::ComfyUiObjectValidityListener::Step /*step*/)
{
    m_invalidation_state = false;
    create_toolbar();
    alive();
}

DlgCode PanelWorkflow::show(bool& focused)
{
    bool opened = true;
    ImGui::SetNextWindowDockID(m_dock, m_restore_pos ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
    m_restore_pos = false;

    const bool active = (g::pFlow->wf_get_active() == get_workflow()->get_id());
    if (active)
    {
        ImGui::PushStyleColor(ImGuiCol_TabHovered, dlg_data()->clr_active_hov_wf);
        ImGui::PushStyleColor(ImGuiCol_TabActive, dlg_data()->clr_active_wf);
        ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, dlg_data()->clr_active_wf);
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, dlg_data()->clr_active_wf);
        ImGui::PushStyleColor(ImGuiCol_TitleBg, dlg_data()->clr_active_wf);
    }

    if (begin_panel(opened, focused) && opened)
    {
        handle_panel_context_menu();

        const auto& data = m_wf->get_data();
        if (m_wf_valid != m_wf->get_data().valid)
        {
            m_wf_valid = m_wf->get_data().valid;
            create_toolbar();
        }

        if (data.critical_failure) [[unlikely]]
        {
            draw_toolbar();
        }
        else
        {
            mark_dirty(data.dirty);
            check_history();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 2);
            draw_toolbar();
            draw_wf();
        }
    }
    else dead();
    if (!opened) dead();

    const auto code = end_panel(opened);

    if (active)
        ImGui::PopStyleColor(5);
    
    return code;
}

void PanelWorkflow::handle_panel_context_menu() noexcept
{
    //TODO

    //ImGuiContext& g = *GImGui;
    //ImGuiWindow* window = ImGui::GetCurrentWindow();
    //const bool dock_tab_hovered = (g.HoveredId == window->TabId);
    //const bool title_bar_hovered = (g.HoveredId == window->MoveId);

    //if ((dock_tab_hovered || title_bar_hovered) && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    //    ImGui::OpenPopup("StrictTitleContext");

    //if (title_bar_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    //    ImGui::OpenPopup("WfpanelContextMenu");

    //if (ImGui::BeginPopup("WfpanelContextMenu"))
    //{
    //    if (ImGui::MenuItem("Menu item"))
    //    {        
    //    }
    //    ImGui::EndPopup();
    //}
}

void PanelWorkflow::set_wf_view(std::optional<ImRect> wf_rc, std::optional<float> /*wf_zoom*/)
{
    if (wf_rc.has_value())
        m_drawer.set_viewport_rect(wf_rc.value());
    //if (wf_zoom.has_value())
    //    m_drawer.set_zoom(wf_zoom.value());
}

bool PanelWorkflow::can_run() const noexcept
{
    return g::pFlow->get_nodes_ready() && !m_wf->get_data().critical_failure && m_wf->get_data().valid && !m_invalidation_state;
}

void PanelWorkflow::run(bool add_front)
{
    g::pQueueManager->add_to_queue(m_wf, m_tasks_num, add_front);
}

void PanelWorkflow::set_num_of_runs(int32_t num) noexcept
{
    m_tasks_num = (0 == num ? -1 : num);
}

bool PanelWorkflow::get_node_selected(data::NodeInstance* node) const noexcept
{
    LogRAssert(node, "Zero ptr", false);
    return m_drawer.is_node_selected(node->ed());
}

void PanelWorkflow::group_nodes(const std::set<data::NodeInstance*>& nodes)
{
    m_action->multi_action_on("Add group");
    auto group = m_action->add_group(std::nullopt);
    m_action->set_node_rect(group, m_drawer.get_group_rect(nodes));
    m_action->multi_action_off();
    m_wf->handle_rgthree_switchers(nullptr);
}

std::set<data::NodeInstance*> PanelWorkflow::get_nodes_in_group(data::NodeInstance* group)
{
    LogRAssert(group, "Zero ptr", {});
    LogRAssert(group->flags().has(NodeInstFlags::Group), "Not a group", {});
    return m_drawer.get_nodes_in_group(group, m_wf->get_nodes_all());
}

void PanelWorkflow::delete_nodes(std::set<data::NodeInstance*>&& nodes, bool process_pending_deletion)
{
    if (!process_pending_deletion)
    {
        m_drawer.clear_selection();
        m_pending_cleanups.merge(std::move(nodes));
        return;
    }

    m_action->multi_action_on("Remove");
    std::set<data::NodeInstance*> nodes_upd;
    for (auto node : nodes)
    {
        if (node->has_object() && node->obj_flags().has(NodeObjFlags::SpecReroute))
        {
            if (node->flags().has(NodeInstFlags::RerouteSplitted_R))
                node = cast::strict<data::CNI_Reroute>(node)->get_splitted_base();
            m_wf->collapse_reroute(node);
        }
        nodes_upd.insert(node);
    }
    for (auto node : nodes_upd)
    {
        m_action->delete_node(node);
    }
    m_action->multi_action_off();
}

void PanelWorkflow::copy_nodes(const std::set<data::NodeInstance*>& nodes, std::optional<ImVec2> src_point)
{
    g::pWfsManager->copy_begin(m_wf, src_point);
    for (auto node : nodes)
    {
        if (node->flags().has(NodeInstFlags::Group) || node->obj_flags().has_not(NodeObjFlags::Dummy))
        {
            if (node->flags().has(NodeInstFlags::RerouteSplitted_R))
                g::pWfsManager->copy_add(cast::strict<data::CNI_Reroute>(node)->get_splitted_base());
            else
                g::pWfsManager->copy_add(node);
        }
    }
}

void PanelWorkflow::copy_selected_nodes(std::optional<ImVec2> src_point)
{    
    const std::set<data::NodeInstance*> nodes = get_selected_nodes(true);
    std::set<data::NodeInstance*> to_copy = nodes;
    for (auto node : nodes)
    {
        if (node->flags().has(NodeInstFlags::Group))
        {
            auto inside = m_drawer.get_nodes_in_group(node, m_wf->get_nodes_all());
            to_copy.merge(inside);
        }
    }
    copy_nodes(to_copy, src_point);
}

void PanelWorkflow::paste_copied(std::optional<ImVec2> paste_at)
{
    bool center_point = false;
    if (!paste_at.has_value())
    {
        ImVec2 point;
        const ImVec2 mouse_screen_pos = ImGui::GetMousePos();
        if (m_drawer.get_canvas_hovered())
        {
            point = m_drawer.screen_to_canvas(mouse_screen_pos);
            Log("Copy dest point autoset to mouse pos: " << point.x << "/" << point.y);
        }
        else
        {
            center_point = true;
            const ImRect& view_rect = m_drawer.get_viewport_rect();
            point.x = view_rect.Min.x + (view_rect.Max.x - view_rect.Min.x) * 0.5f;
            point.y = view_rect.Min.y + (view_rect.Max.y - view_rect.Min.y) * 0.5f;
            Log("Copy dest point autoset to point: " << point.x << "/" << point.y);
        }

        paste_at = point;
    }

    g::pWfsManager->copy_end(m_wf, paste_at.value(), center_point);
}

void PanelWorkflow::save() const
{
    if (can_save())
        g::pWfsManager->save(m_wf->get_id());
}

void PanelWorkflow::undo() const
{
    if (can_undo())
        g::pHistoryManager->undo(m_wf->get_id());
}

void PanelWorkflow::redo() const
{
    if (can_redo())
        g::pHistoryManager->redo(m_wf->get_id());
}

void PanelWorkflow::separate_reroutes(const std::set<data::NodeInstance*>& nodes)
{
    if (nodes.empty())
        return;

    m_wf->separate_reroutes(nodes);
}

void PanelWorkflow::bundle_reroutes(const std::set<data::NodeInstance*>& nodes)
{
    if (nodes.size() <= 1)
        return;

    m_wf->bundle_reroutes(nodes);
}

void PanelWorkflow::insert_reroute(data::Link* link, ImVec2 pos)
{
    if (nullptr == link)
    {
        const auto links = m_drawer.get_selected_links();
        if (links.empty())
            return;
        link = *links.begin();
        LogRAssert(link, "Zero ptr", );

        const auto* n1 = m_wf->get_node_by_instance(link->get_input()->get_parent_node());
        const auto* n2 = m_wf->get_node_by_instance(link->get_output()->get_parent_node());
        LogRAssert(n1 && n2, "Node not found, n1:" << link->get_input()->get_parent_node() << ", n2:" << link->get_output()->get_parent_node(), );

        const auto p1_x = n1->m_rc.Min.x + n1->m_rc.Max.x;
        const auto p1_y = n1->m_rc.Min.y;
        const auto p2_x = n2->m_rc.Min.x;
        const auto p2_y = n2->m_rc.Min.y;

        pos.x = p1_x + (p2_x - p1_x) / 2;
        pos.y = p1_y + (p2_y - p1_y) / 2;
    }
    LogRAssert(link != nullptr, "Zero ptr", );
    m_wf->insert_reroute(link, pos);
}

void PanelWorkflow::split_reroute_to_parts(data::NodeInstance* reroute)
{
    LogRAssert(reroute != nullptr, "Zero ptr", );
    m_wf->split_reroute_to_parts(reroute);
}

void PanelWorkflow::merge_reroute_parts(data::NodeInstance* reroute, bool to_left)
{
    LogRAssert(reroute != nullptr, "Zero ptr", );
    m_wf->merge_reroute_parts(reroute, to_left);
}

void PanelWorkflow::set_group_mode(data::NodeInstance* group, NodeInstFlags mode)
{
    LogRAssert(group, "Zero ptr", );
    LogRAssert(group->flags().has(NodeInstFlags::Group), "Not a group", );
    LogRAssert(NodeInstFlags::None == mode || NodeInstFlags::ModeBypass == mode || NodeInstFlags::ModeMute == mode, "Unknown mode", );

    if (NodeInstFlags::None == mode)
        m_action->multi_action_on("Always mode");
    else if (NodeInstFlags::ModeBypass == mode)
        m_action->multi_action_on("Bypass");
    else if (NodeInstFlags::ModeMute == mode)
        m_action->multi_action_on("Mute");

    const auto nodes = m_drawer.get_nodes_in_group(group, m_wf->get_nodes_all());
    for (auto node : nodes)
    {
        if (node->obj_flags().has_not(NodeObjFlags::Dummy))
            m_action->set_node_mode(node, mode);
    }

    m_action->multi_action_off();
}

void PanelWorkflow::draw_wf()
{
    if (m_invalidation_state)
        return;

    ImGuiIO& io = ImGui::GetIO();
    const ImGuiConfigFlags old_flags = io.ConfigFlags;
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

    auto& nodes = m_wf->get_nodes_all();
    m_drawer.pre_draw_all(get_panel_id(), nodes.size(), m_first_show);

    if (m_select_new_nodes)
        m_drawer.clear_selection();

    for (size_t node_idx = 0; node_idx < nodes.size(); node_idx++)
    {
        auto node = nodes[node_idx].get();
        if (node->flags().has(NodeInstFlags::Hidden))
            continue;

        bool failed_node = false;
        const std::set<std::string>* failed_pins = nullptr;
        if (m_failed_nodes != nullptr) [[unlikely]]
        {
            const auto it = m_failed_nodes->find(node->get_instance_id());
            if (it != m_failed_nodes->end())
            {
                failed_node = true;
                failed_pins = &it->second;
            }
        }

        const bool is_new = !node->ed().has_object();
        if (is_new) [[unlikely]]
        {
            node->reset_state();
            if (node->flags().has_not(NodeInstFlags::Group))
                node->reset_width();
        }

        ImGui::PushID(node);        

        m_drawer.pre_draw_node(node, failed_node);
        if (node->flags().has(NodeInstFlags::Group))
            m_drawer.draw_group(node, node_idx);
        else
            m_drawer.draw_node(node, node->get_object()->w_fit, node_idx, failed_pins);
        m_drawer.post_draw_node(node, failed_node);

        ImGui::PopID();

        if (is_new && m_select_new_nodes || m_select_all_nodes) [[unlikely]]
            m_drawer.select_node(node->ed(), true);
    }

    m_select_new_nodes = false;
    m_select_all_nodes = false;

    if (m_locate != nullptr) [[unlikely]]
    {
        m_drawer.locate_node(m_locate);
        m_locate = nullptr;
    }
    else if (m_locate_selected) [[unlikely]]
    {
        m_drawer.locate_selected();
        m_locate_selected = false;
    }

    m_drawer.draw_links(m_wf->get_links_all());
    m_drawer.query_new_item(
        [this](data::PinInstance* pin) { m_action->unlink(pin); },
        [this](data::PinInstance* pin, const ImVec2& pt)
            {
                const auto parent = m_wf->get_node_by_instance(pin->get_parent_node());
                if (parent != nullptr)
                {
                    m_menu_objs = {};
                    m_menu_objs.click_pos = pt;
                    m_menu_objs.pins.insert(pin);
                    m_menu_objs.nodes.insert(parent);
                    m_context_menu_type = MenuType::SpawnNode;
                    m_handle_context_menu = true;
                }
            }
    );

    //if (m_first_show) [[unlikely]]
    //{
    //    g::pHistoryManager->set_ignore_actions_on(m_wf->get_id());
    //    std::map<data::NodeInstance*, std::set<data::NodeInstance*>> items;
    //    fit(items);
    //    g::pHistoryManager->set_ignore_actions_off(m_wf->get_id());
    //}

    m_drawer.post_draw_all(
        [this]() { return context_menu_handler(); },
        [this]() { return show_context_menu(); },
        [this]() { return handle_moves(); },
        [this]() { return g::pHotkeysManager->process_node_hotkey(this); }
    );

    ImGui::GetIO().ConfigFlags = old_flags;
    m_first_show = false;

    handle_dnd();
}

void PanelWorkflow::fit(std::map<data::NodeInstance*, std::set<data::NodeInstance*>>& items)
{
    m_action->multi_action_on("Fit");
    if (items.empty())
    {
        for (auto& node : m_wf->get_nodes_all())
        {
            if (node->flags().has(NodeInstFlags::Group))
            {
                items.insert({ node.get(), m_drawer.get_nodes_in_group(node.get(), m_wf->get_nodes_all()) });
            }
            else
            {
                items.insert({ node.get(), {} });
            }
        }
    }

    for (auto it = items.begin(); it != items.end(); )
    {
        if (it->first->flags().has_not(NodeInstFlags::Group))
        {
            m_action->reset_node_size(it->first);
            it = items.erase(it);
        }
        else if (it->second.empty()) // empty group
        {
            it = items.erase(it);
        }
        else
            ++it;
    }

    std::set<data::NodeInstance*> add_y;
    for (auto it = items.begin(); it != items.end(); ++it)
    {
        auto& nodes = it->second;
        for (auto node : nodes)
        {
            if (node != it->first && node->flags().has(NodeInstFlags::Group) && items.find(node) != items.end())
            {
                if (m_action->set_node_rect(node, m_drawer.get_group_rect(m_drawer.get_nodes_in_group(node, m_wf->get_nodes_all()))))
                    add_y.insert(it->first);
                items.erase(node);
            }
        }
    }

    for (auto& [group, nodes] : items)
    {
        m_action->set_node_rect(group, m_drawer.get_group_rect(nodes));
        if (add_y.find(group) != add_y.end())
            group->m_rc.Max.y += ImGui::GetTextLineHeight();
    }
    m_action->multi_action_off();
}

void PanelWorkflow::handle_dnd()
{
    if (m_invalidation_state)
        return;

    if (ImGui::BeginDragDropTarget())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(DND_FROM_NODES_LIBRARY);
        if (payload != nullptr)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
            const hid node_hash = *(const hid*)payload->Data;
            auto node = g::pNodesManager->find_node(node_hash);
            const auto* inst = m_action->add_node(node, "", ImRect(m_drawer.get_mouse_pos(), ImVec2(-1, -1)));
            Log("Node '" << inst->get_name() << "' dropped");
            g::pFlow->wf_activate(m_wf->get_id());
        }
        ImGui::EndDragDropTarget();
    }
}

bool PanelWorkflow::handle_moves()
{
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        auto node = m_drawer.get_hovered_node();
        if (node != nullptr)
        {
            auto selected = m_drawer.get_selected_nodes(false);
            if (!m_drawer.is_node_selected(node->ed()))
            {
                selected.clear();
                selected.insert(node);
            }

            std::set<data::NodeInstance*> add_subnodes;
            for (auto sel : selected)
            {
                if (sel->flags().has(NodeInstFlags::Group))
                {
                    auto subnodes = m_drawer.get_nodes_in_group(sel, m_wf->get_nodes_all());
                    for (auto subnode : subnodes)
                    {
                        add_subnodes.insert(subnode);
                    }
                }
            }
            for (auto subnode : add_subnodes)
            {
                selected.insert(subnode);
            }

            m_action->begin_nodes_move(selected);
            return true;
        }
    }
    else if (m_action->is_moving() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        if (m_invalidation_state)
            m_action->end_nodes_move(true);
        else
            m_action->end_nodes_move(!ImGui::IsMouseDragPastThreshold(ImGuiMouseButton_Left, 2));
        return true;
    }
    return false;
}

void PanelWorkflow::handle_hotkeys_changed()
{
    create_toolbar();
}

void PanelWorkflow::create_toolbar()
{
    clear_toolbar();
    set_use_regular_size_icons();

    if (m_wf->get_data().critical_failure)
    {
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Empty;
            add_toolbar_element(std::move(el), true);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Text;
            el.text = "Something went wrong. The Project couldn't be loaded.";
            add_toolbar_element(std::move(el), true);
        }
    }
    else
    {
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Empty;
            add_toolbar_element(std::move(el), true);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Button;
            el.icon = ICON_LC_SAVE;
            el.tooltip = "Save Project";
            el.tooltip_hotkey = g::pHotkeysManager->get_key_binding_name_str(KeyTarget::Workspace_Save);
            el.on_preview = [this](PanelToolbar::Element& el) noexcept { el.enabled = can_save(); };
            el.on_click = [this](PanelToolbar::Element& /*el*/) { save(); };
            add_toolbar_element(std::move(el), true);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Button;
            el.icon = ICON_LC_HISTORY;
            el.tooltip = "Revert to saved";
            el.on_preview = [this](PanelToolbar::Element& el) noexcept
            {
                el.enabled = m_can_revert && !m_invalidation_state;
                el.tooltip = m_tt_revert;
            };
            el.on_click = [this](PanelToolbar::Element& el) { on_revert(el); };
            add_toolbar_element(std::move(el), true);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Empty;
            add_toolbar_element(std::move(el), true);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Button;
            el.icon = ICON_LC_UNDO_2;
            el.tooltip = "Undo";
            el.tooltip_hotkey = g::pHotkeysManager->get_key_binding_name_str(KeyTarget::Workspace_Undo);
            el.on_preview = [this](PanelToolbar::Element& el) noexcept
            {
                el.enabled = can_undo();
                el.tooltip = m_undo_name;
            };
            el.on_click = [this](PanelToolbar::Element& /*el*/) { undo(); };
            add_toolbar_element(std::move(el), true);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Button;
            el.icon = ICON_LC_REDO_2;
            el.tooltip = "Redo";
            el.tooltip_hotkey = g::pHotkeysManager->get_key_binding_name_str(KeyTarget::Workspace_Redo);
            el.on_preview = [this](PanelToolbar::Element& el) noexcept
            {
                el.enabled = can_redo();
                el.tooltip = m_redo_name;
            };
            el.on_click = [this](PanelToolbar::Element& /*el*/) { redo(); };
            add_toolbar_element(std::move(el), true);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Empty;
            add_toolbar_element(std::move(el), true);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Text;
            el.text = "Saved";
            el.text_clr = g::pOptsManager->color_load(AppColor::ActivityBarInactive);
            el.on_preview = [this](PanelToolbar::Element& el) noexcept
            {
                el.text = m_history_status;
                el.tooltip = m_tt_history_status;
            };
            add_toolbar_element(std::move(el), true);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Empty;
            add_toolbar_element(std::move(el), true);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Empty;
            add_toolbar_element(std::move(el), true);
        }

        // right

        if (m_wf->get_data().valid)
        {
            {
                PanelToolbar::Element el;
                el.type = PanelToolbar::Element::Type::Text;
                el.text_width = ImGui::CalcTextSize("Prompt outputs failed validation").x;
                el.on_preview = [this](PanelToolbar::Element& el) { on_pre_queue_status(el); };
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
                el.icon = ICON_LC_PLAY;
                el.tooltip = "Run workflow (Shift to queue at front)";
                el.tooltip_hotkey = g::pHotkeysManager->get_key_binding_name_str(KeyTarget::Workspace_RunBack)
                                    + " | " + g::pHotkeysManager->get_key_binding_name_str(KeyTarget::Workspace_RunFront);
                el.tooltip_next_line = true;
                el.tooltip_hotkey_separator = nullptr;
                el.on_preview = [this](PanelToolbar::Element& el) noexcept { el.enabled = can_run(); };
                el.on_click = [this](PanelToolbar::Element& /*el*/) { run(ImGui::IsKeyDown(ImGuiMod_Shift)); };
                add_toolbar_element(std::move(el), false);
            }
            {
                PanelToolbar::Element el;
                el.type = PanelToolbar::Element::Type::Button;
                el.icon = ICON_LC_X;
                el.tooltip = "Terminate current task";
                el.tooltip_hotkey = g::pHotkeysManager->get_key_binding_name_str(KeyTarget::Queue_Interrupt);
                el.on_preview = [this](PanelToolbar::Element& el) noexcept
                {
                    const auto g = g::pQueueManager->get_current_group();
                    el.enabled = g != nullptr && g->src_wf == m_wf->get_id();
                };
                el.on_click = [this](PanelToolbar::Element& /*el*/) { g::pQueueManager->interrupt_task(m_wf->get_id()); };
                add_toolbar_element(std::move(el), false);
            }
            {
                PanelToolbar::Element el;
                el.type = PanelToolbar::Element::Type::Button;
                el.icon = ICON_LC_SQUARE;
                el.tooltip = "Cancel all tasks for this workflow";
                el.on_preview = [this](PanelToolbar::Element& el)
                {
                    const auto g = g::pQueueManager->get_current_group();
                    el.enabled = (g != nullptr && g->src_wf == m_wf->get_id()) || g::pQueueManager->get_is_queued(m_wf->get_id());
                };
                el.on_click = [this](PanelToolbar::Element& /*el*/) { g::pQueueManager->drop_wf_tasks(m_wf->get_id()); };
                add_toolbar_element(std::move(el), false);
            }
            {
                PanelToolbar::Element el;
                el.type = PanelToolbar::Element::Type::Combo;
                el.tooltip = "Number of tasks to run";
                el.tooltip_hotkey = g::pHotkeysManager->get_key_binding_name_str(KeyTarget::Workspace_Runs1) +
                                    " ... " + g::pHotkeysManager->get_key_binding_name_str(KeyTarget::Workspace_Runs9) +
                                    " | " + g::pHotkeysManager->get_key_binding_name_str(KeyTarget::Workspace_RunsInf);
                el.tooltip_next_line = true;
                el.tooltip_hotkey_separator = nullptr;
                m_queue_num_input_id = "##num_" + m_wf->get_uuid();
                el.id = m_queue_num_input_id.c_str();
                el.input_w = 150.0f;
                el.cb_items = { "Infinite", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "15", "20" };
                el.cb_item_sel = 1;
                el.on_preview = [this](PanelToolbar::Element& el) noexcept
                {
                    if (-1 == m_tasks_num || 0 == m_tasks_num)  el.cb_item_sel = 0;
                    else                                        el.cb_item_sel = m_tasks_num;
                };
                el.on_click = [this](PanelToolbar::Element& el)
                {
                    if (0 == el.cb_item_sel)    m_tasks_num = -1;
                    else                        m_tasks_num = std::strtol(el.cb_items[el.cb_item_sel], nullptr, 10);
                };
                add_toolbar_element(std::move(el), false);
            }
        }
        else
        {
            {
                PanelToolbar::Element el;
                el.type = PanelToolbar::Element::Type::Text;
                el.text = "The project contains unrecognized nodes. Generation is not possible.";
                add_toolbar_element(std::move(el), false);
            }
            {
                PanelToolbar::Element el;
                el.type = PanelToolbar::Element::Type::Button;
                el.icon = ICON_LC_LOCATE;
                el.tooltip = "Show next unrecognized node";
                el.on_click = [this](PanelToolbar::Element& el) { on_show_dummy_node(el); };
                add_toolbar_element(std::move(el), false);
            }
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Empty;
            add_toolbar_element(std::move(el), false);
        }
    }
    
    finalize_toolbar(false /*separator*/);
}

void PanelWorkflow::check_history()
{
    *m_tt_revert = 0;
    *m_tt_history_status = 0;
    *m_history_status = 0;
    *m_undo_name = 0;
    *m_redo_name = 0;
    m_can_save = m_can_revert = false;

    if (m_invalidation_state)
        return;

    const auto& data = m_wf->get_data();
    m_can_save = data.draft;
    m_can_revert = data.saved_exist && data.draft;

    const auto history = g::pHistoryManager->get_state(m_wf->get_id());

    char* end_ptr = nullptr;
    const int32_t diff = history.pos - (history.saved_pos < -1 ? -1 : history.saved_pos);

    if (history.undo_name != nullptr)
    {
        end_ptr = fmt::format_to_n(m_undo_name, sizeof(m_undo_name), FMT_STRING("Undo: {}"), *history.undo_name).out;
        *end_ptr = 0;
    }
    if (history.redo_name != nullptr)
    {
        end_ptr = fmt::format_to_n(m_redo_name, sizeof(m_redo_name), FMT_STRING("Redo: {}"), *history.redo_name).out;
        *end_ptr = 0;
    }

    if (m_can_revert)
    {
        if (0 == diff)
        {
            end_ptr = fmt::format_to_n(m_tt_revert, sizeof(m_tt_revert),
                FMT_COMPILE("Revert to saved state\n"
                    "Last saved: {:02} {} {}, {:02}:{:02}"),
                data.saved_time.day(), data.saved_time.month_name(3), data.saved_time.year(), data.saved_time.hour(), data.saved_time.minute()
            ).out;
        }
        else
        {
            end_ptr = fmt::format_to_n(m_tt_revert, sizeof(m_tt_revert),
                FMT_COMPILE("Revert to saved state\n"
                    "Last saved: {:02} {} {}, {:02}:{:02}\n"
                    "Will {} {} {}"),
                data.saved_time.day(), data.saved_time.month_name(3), data.saved_time.year(), data.saved_time.hour(), data.saved_time.minute(),
                (diff > 0 ? "discard" : "re-apply"), std::abs(diff), (diff > 0 ? "unsaved actions" : "actions") 
            ).out;
        }
        *end_ptr = 0;
    }

    if (data.draft)
    {
        if (diff != 0 && data.saved_exist)
        {
            end_ptr = fmt::format_to_n(m_history_status, sizeof(m_history_status), FMT_STRING("Draft: {}{}"), (diff > 0 ? "+" : ""), diff).out;
        }
        else
        {
            end_ptr = fmt::format_to_n(m_history_status, sizeof(m_history_status), "Draft").out;
        }
        *end_ptr = 0;

        if (!data.draft_time.null())
        {
            end_ptr = fmt::format_to_n(m_tt_history_status, sizeof(m_tt_history_status),
                FMT_COMPILE("Autosaved as draft at {:02} {} {}, {:02}:{:02}"),
                data.draft_time.day(), data.draft_time.month_name(3), data.draft_time.year(), data.draft_time.hour(), data.draft_time.minute()
            ).out;
            *end_ptr = 0;
        }
    }
    else
    {
        end_ptr = fmt::format_to_n(m_history_status, sizeof(m_history_status), "Saved").out;
        *end_ptr = 0;

        end_ptr = fmt::format_to_n(m_tt_history_status, sizeof(m_tt_history_status),
            FMT_COMPILE("Saved at {:02} {} {}, {:02}:{:02}"),
            data.saved_time.day(), data.saved_time.month_name(3), data.saved_time.year(), data.saved_time.hour(), data.saved_time.minute()
        ).out;
        *end_ptr = 0;
    }
}

void PanelWorkflow::on_revert(PanelToolbar::Element& /*el*/)
{
    blob::Blob drawer_state;
    bool restore_drawer = false;
    g::pWfsManager->revert(m_wf->get_id(), drawer_state, restore_drawer);
    if (restore_drawer)
        m_drawer.init(std::move(drawer_state));
}

void PanelWorkflow::on_pre_queue_status(PanelToolbar::Element& el)
{
    el.text = "Idle";
    el.tooltip = nullptr;
    m_failed_nodes = nullptr;
    el.blinking = false;
    el.text_clr = 0;

    auto failed = g::pQueueManager->get_is_failed(m_wf->get_id());
    if (failed != nullptr) [[unlikely]]
    {
        if (TaskResult::Interrupted == failed->result)
        {
            el.text = "Interrupted";
        }
        else
        {
            el.text = failed->err_message.c_str();
            el.tooltip = failed->err_full.c_str();
            if (failed->err_message.empty())
                el.text = "Prompt execution failed";
            m_failed_nodes = &failed->err_nodes;
        }
        el.text_clr = g::pOptsManager->color_load(AppColor::SelectedItem);
        return;
    }

    const auto external = g::pQueueManager->is_external_task();
    if (external > 0) [[unlikely]]
    {
        el.text = "External task is running";
        m_status_buffer.clear();
        fmt::format_to(std::back_inserter(m_status_buffer), FMT_COMPILE("Queue remaining: {}"), external);
        m_status_buffer.push_back('\0');
        el.tooltip = m_status_buffer.data();
        return;
    }

    auto group = g::pQueueManager->get_current_group();
    if (group != nullptr && group->src_wf == m_wf->get_id())
    {
        m_status_buffer.clear();
        if (0 == group->task.current_node)
        {
            fmt::format_to(std::back_inserter(m_status_buffer), FMT_COMPILE("Progress: {:.0f}%"), group->task.progress * 100);
        }
        else
        {
            const auto* node = group->wf->get_node_by_instance(group->task.current_node);
            if (node != nullptr)
                fmt::format_to(std::back_inserter(m_status_buffer), FMT_COMPILE("Progress: {:.0f}%: {}"), group->task.progress * 100, node->get_name());
            else
                fmt::format_to(std::back_inserter(m_status_buffer), FMT_COMPILE("Progress: {:.0f}%"), group->task.progress * 100);
        }
        m_status_buffer.push_back('\0');
        el.text = m_status_buffer.data();
        return;
    }

    if (g::pQueueManager->is_paused()) [[unlikely]]
    { 
        el.text = "All tasks are paused";
        el.tooltip = "Click 'Unpause' on the Queue panel to resume";
        el.blinking = true;
        return;
    }

    if (g::pQueueManager->get_is_queued(m_wf->get_id()))
    {
        el.text = "Queued";
    }
}

void PanelWorkflow::on_show_dummy_node(PanelToolbar::Element& el)
{
    if (m_invalidation_state)
        return;

    int32_t idx = 0;
    for (auto& node : m_wf->get_nodes_all())
    {
        if (node->has_object() && node->obj_flags().has(NodeObjFlags::Dummy))
        {
            if (m_curr_dummy_node == idx++)
            {
                m_curr_dummy_node++;
                m_drawer.locate_node(node.get());
                return;
            }
        }
    }

    if (idx > 0)
    {
        m_curr_dummy_node = 0;
        on_show_dummy_node(el);
    }
}

} // namespace ui
