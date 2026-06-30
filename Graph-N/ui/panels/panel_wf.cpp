
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

namespace ui
{

PanelWorkflow::PanelWorkflow(wid workflow_id, const std::string& name, const std::string& panel_id, ImGuiID dock)
    : PanelBase(DlgObj::PanelWf, name, panel_id)
    , m_wf(g::pWfsManager->access(workflow_id))
    , m_dock(dock)
    , m_action(m_wf->action())
    , m_drawer(m_wf)
    , m_context_menuid(panel_id + "_ContextMenu")
{
    m_action->set_drawer(&m_drawer);

    g::pGraphN->subscribe_comfyui_validity_state(listener::ComfyUiObjectValidityListener::Step::Invalidate, this);
    g::pGraphN->subscribe_comfyui_validity_state(listener::ComfyUiObjectValidityListener::Step::ResolvePanels, this);
}

PanelWorkflow::~PanelWorkflow()
{
    LogF;
    if (g::pGraphN != nullptr)
        g::pGraphN->unsubscribe_comfyui_validity_state(this);
}

void PanelWorkflow::stop_all_threads()
{
    LogF;
    dead();
}

void PanelWorkflow::free()
{
    LogF;

    dead();

    if (g::pGraphN != nullptr)
        g::pGraphN->unsubscribe_comfyui_validity_state(this);

    m_wf->get_data().activated = false;
    m_wf->clear();
    m_wf = nullptr;
    m_action->set_drawer(nullptr);
    m_drawer.free();
}

void PanelWorkflow::init()
{
    LogF;
    PanelBase::init();

    blob::Blob drawer_state;
    g::pWfsManager->load(m_wf->get_id(), drawer_state);
    m_drawer.init(std::move(drawer_state));

    m_wf->reset();
    m_wf->get_data().activated = true;
    create_toolbar();
    alive();
}

void PanelWorkflow::invalidate_comfyui_objects()
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
        ImGui::PushStyleColor(ImGuiCol_TabHovered, data()->clr_active_hov_wf);
        ImGui::PushStyleColor(ImGuiCol_TabActive, data()->clr_active_wf);
        ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, data()->clr_active_wf);
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, data()->clr_active_wf);
        ImGui::PushStyleColor(ImGuiCol_TitleBg, data()->clr_active_wf);
    }

    if (begin_panel(opened, focused) && opened)
    {
        handle_panel_context_menu();

        auto& data = m_wf->get_data();
        if (data.critical_failure)
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

void PanelWorkflow::handle_panel_context_menu()
{
    //TODO

    //ImGuiContext& g = *GImGui;
    //ImGuiWindow* window = ImGui::GetCurrentWindow();
    //const bool dock_tab_hovered = (g.HoveredId == window->TabId);
    //const bool title_bar_hovered = (g.HoveredId == window->MoveId);
    //LogV(dock_tab_hovered, title_bar_hovered);

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

void PanelWorkflow::set_wf_view(std::optional<ImRect> wf_rc, std::optional<float> wf_zoom)
{
    if (wf_rc.has_value())
        m_drawer.set_viewport_rect(wf_rc.value());
    if (wf_zoom.has_value())
        m_drawer.set_zoom(wf_zoom.value());
}

bool PanelWorkflow::can_run() const
{
    return !m_wf->get_data().critical_failure && m_wf->get_data().valid && !m_invalidation_state;
}

void PanelWorkflow::run(bool add_front)
{
    g::pQueueManager->add_to_queue(m_wf, m_tasks_num, add_front);
}

void PanelWorkflow::set_num_of_runs(int32_t num)
{
    m_tasks_num = (0 == num ? -1 : num);
}

bool PanelWorkflow::get_node_selected(data::NodeInstance* node) const
{
    return m_drawer.is_node_selected(node->ed());
}

void PanelWorkflow::group_nodes(const std::set<data::NodeInstance*>& nodes)
{
    m_action->multi_action_on("Add group");
    auto group = m_action->add_group(std::nullopt);
    m_action->set_node_rect(group, m_drawer.fit_group_rect(group, nodes));
    m_action->multi_action_off();
}

void PanelWorkflow::delete_nodes(const std::set<data::NodeInstance*>& nodes)
{
    m_action->multi_action_on("Remove");
    for (auto node : nodes)
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
        if (!node->get_object()->is_dummy)
            g::pWfsManager->copy_add(node);
    }
}

void PanelWorkflow::paste_copied(std::optional<ImVec2> paste_at)
{
    g::pWfsManager->copy_end(m_wf, paste_at);
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

void PanelWorkflow::draw_wf()
{
    if (m_invalidation_state)
        return;

    ImGuiIO& io = ImGui::GetIO();
    ImGuiConfigFlags old_flags = io.ConfigFlags;
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

    auto& nodes = m_wf->get_nodes();
    m_drawer.pre_draw_all(get_panel_id(), nodes.size());

    if (m_select_new_nodes)
        m_drawer.select_node({}, false);

    for (size_t node_idx = 0; node_idx < nodes.size(); node_idx++)
    {
        auto node = nodes[node_idx].get();

        bool failed_node = false;
        const std::set<std::string>* failed_pins = nullptr;
        if (m_failed_nodes != nullptr)
        {
            const auto it = m_failed_nodes->find(node->get_instance_id());
            if (it != m_failed_nodes->end())
            {
                failed_node = true;
                failed_pins = &it->second;
            }
        }

        const bool is_new = !node->ed();
        if (is_new)
        {
            node->reset_state();
            if (!node->m_group)
                node->calc_node_width(false, false);
        }

        ImGui::PushID(node);
        m_drawer.pre_draw_node(node, failed_node);
        if (node->m_group)
            m_drawer.draw_group(node, node_idx);
        else
            m_drawer.draw_node(node, node->get_object()->w_fit, node_idx, failed_pins);
        m_drawer.post_draw_node(node, failed_node);
        ImGui::PopID();

        if (is_new && m_select_new_nodes || m_select_all_nodes)
            m_drawer.select_node(node->ed(), true);
    }

    m_select_new_nodes = false;
    m_select_all_nodes = false;

    if (m_locate != nullptr)
    {
        m_drawer.locate_node(m_locate);
        m_locate = nullptr;
    }
    else if (m_locate_selected)
    {
        m_drawer.locate_selected();
        m_locate_selected = false;
    }

    m_drawer.draw_links(m_wf->get_links());
    m_drawer.query_new_item(
        [this](data::PinInstance* pin) { m_action->unlink(pin); },
        [this](data::PinInstance* pin, const ImVec2& pt)
            {
                auto node = m_wf->get_node_by_instance(pin->get_parent_node());
                m_menu_objs = {};
                m_menu_objs.click_pos = pt;
                m_menu_objs.pins.insert(pin);
                m_menu_objs.nodes.insert(node);
                m_context_menu_type = MenuType::SpawnNode;
                m_handle_context_menu = true;
            }
    );

    if (m_first_show)
    {
        g::pHistoryManager->set_ignore_actions_on(m_wf->get_id());
        std::map<data::NodeInstance*, std::set<data::NodeInstance*>> items;
        fit(items);
        m_first_show = false;
        g::pHistoryManager->set_ignore_actions_off(m_wf->get_id());
    }

    m_drawer.post_draw_all(
        [this]() { return context_menu_handler(); },
        [this]() { return show_context_menu(); },
        [this]() { return handle_moves(); },
        [this]() { return g::pHotkeysManager->process_node_hotkey(this); }
    );

    ImGui::GetIO().ConfigFlags = old_flags;

    handle_dnd();
}

void PanelWorkflow::fit(std::map<data::NodeInstance*, std::set<data::NodeInstance*>>& items)
{
    m_action->multi_action_on("Fit");
    if (items.empty())
    {
        for (auto& node : m_wf->get_nodes())
        {
            if (node->m_group)
            {
                items.insert({ node.get(), m_drawer.get_nodes_in_group(node.get(), m_wf->get_nodes()) });
            }
            else
            {
                items.insert({ node.get(), {} });
            }
        }
    }

    for (auto it = items.begin(); it != items.end(); )
    {
        if (!it->first->m_group)
        {
            m_action->reset_node_size(it->first);
            it = items.erase(it);
        }
        else if (it->second.empty())
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
            if (node != it->first && node->m_group && items.find(node) != items.end())
            {
                if (m_action->set_node_rect(node, m_drawer.fit_group_rect(node, m_drawer.get_nodes_in_group(node, m_wf->get_nodes()))))
                    add_y.insert(it->first);
                items.erase(node);
            }
        }
    }

    for (auto& [group, nodes] : items)
    {
        m_action->set_node_rect(group, m_drawer.fit_group_rect(group, nodes));
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
            const hid node_id = *(const hid*)payload->Data;
            auto node = g::pNodesManager->find_node(node_id);
            auto inst = m_action->add_node(node, "", ImRect(m_drawer.get_mouse_pos(), ImVec2(-1, -1)));
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
            auto selected = m_drawer.get_selected_nodes();
            if (!m_drawer.is_node_selected(node->ed()))
            {
                selected.clear();
                selected.insert(node);
            }

            std::set<data::NodeInstance*> add_subnodes;
            for (auto sel : selected)
            {
                if (sel->m_group)
                {
                    auto subnodes = m_drawer.get_nodes_in_group(sel, m_wf->get_nodes());
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
            el.on_preview = std::bind(&PanelWorkflow::on_pre_save, this, std::placeholders::_1);
            el.on_click = std::bind(&PanelWorkflow::on_save, this, std::placeholders::_1);
            add_toolbar_element(std::move(el), true);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Button;
            el.icon = ICON_LC_HISTORY;
            el.tooltip = "Revert to saved";
            el.on_preview = std::bind(&PanelWorkflow::on_pre_revert, this, std::placeholders::_1);
            el.on_click = std::bind(&PanelWorkflow::on_revert, this, std::placeholders::_1);
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
            el.on_preview = std::bind(&PanelWorkflow::on_pre_undo, this, std::placeholders::_1);
            el.on_click = std::bind(&PanelWorkflow::on_undo, this, std::placeholders::_1);
            add_toolbar_element(std::move(el), true);
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Button;
            el.icon = ICON_LC_REDO_2;
            el.tooltip = "Redo";
            el.on_preview = std::bind(&PanelWorkflow::on_pre_redo, this, std::placeholders::_1);
            el.on_click = std::bind(&PanelWorkflow::on_redo, this, std::placeholders::_1);
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
            el.on_preview = std::bind(&PanelWorkflow::on_pre_project_status, this, std::placeholders::_1);
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
                el.on_preview = std::bind(&PanelWorkflow::on_pre_queue_status, this, std::placeholders::_1);
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
                el.on_preview = std::bind(&PanelWorkflow::on_pre_queue, this, std::placeholders::_1);
                el.on_click = std::bind(&PanelWorkflow::on_queue, this, std::placeholders::_1);
                add_toolbar_element(std::move(el), false);
            }
            {
                PanelToolbar::Element el;
                el.type = PanelToolbar::Element::Type::Button;
                el.icon = ICON_LC_X;
                el.tooltip = "Terminate current task";
                el.on_preview = std::bind(&PanelWorkflow::on_pre_queue_interrupt, this, std::placeholders::_1);
                el.on_click = std::bind(&PanelWorkflow::on_queue_interrupt, this, std::placeholders::_1);
                add_toolbar_element(std::move(el), false);
            }
            {
                PanelToolbar::Element el;
                el.type = PanelToolbar::Element::Type::Button;
                el.icon = ICON_LC_SQUARE;
                el.tooltip = "Cancel all tasks for this workflow";
                el.on_preview = std::bind(&PanelWorkflow::on_pre_queue_drop_all, this, std::placeholders::_1);
                el.on_click = std::bind(&PanelWorkflow::on_queue_drop_all, this, std::placeholders::_1);
                add_toolbar_element(std::move(el), false);
            }
            {
                PanelToolbar::Element el;
                el.type = PanelToolbar::Element::Type::Combo;
                el.tooltip = "Number of tasks to run";
                m_queue_num_input_id = "##num_" + m_wf->get_uuid();
                el.id = m_queue_num_input_id.c_str();
                el.input_w = 150.0f;
                el.cb_items = { "Infinite", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "15", "20" };
                el.cb_item_sel = 1;
                el.on_preview = std::bind(&PanelWorkflow::on_pre_tasks_num, this, std::placeholders::_1);
                el.on_click = std::bind(&PanelWorkflow::on_tasks_num, this, std::placeholders::_1);
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
                el.on_click = std::bind(&PanelWorkflow::on_show_dummy_node, this, std::placeholders::_1);
                add_toolbar_element(std::move(el), false);
            }
        }
        {
            PanelToolbar::Element el;
            el.type = PanelToolbar::Element::Type::Empty;
            add_toolbar_element(std::move(el), false);
        }
    }
    
    finalize_toolbar();
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

    auto& data = m_wf->get_data();
    m_can_save = data.draft;
    m_can_revert = data.saved_exist && data.draft;

    auto history = g::pHistoryManager->get_state(m_wf->get_id());

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

void PanelWorkflow::on_pre_save(PanelToolbar::Element& el)
{
    el.enabled = can_save();
}

void PanelWorkflow::on_save(PanelToolbar::Element& /*el*/)
{
    save();    
}

void PanelWorkflow::on_pre_revert(PanelToolbar::Element& el)
{
    el.enabled = m_can_revert && !m_invalidation_state;
    el.tooltip = m_tt_revert;
}

void PanelWorkflow::on_revert(PanelToolbar::Element& /*el*/)
{
    blob::Blob drawer_state;
    g::pWfsManager->revert(m_wf->get_id(), drawer_state);
    m_drawer.init(std::move(drawer_state));
    m_wf->reset();
    m_wf->invalidate_current();
}

void PanelWorkflow::on_pre_undo(PanelToolbar::Element& el)
{
    el.enabled = can_undo();
    el.tooltip = m_undo_name;
}

void PanelWorkflow::on_undo(PanelToolbar::Element& /*el*/)
{
    undo();
}

void PanelWorkflow::on_pre_redo(PanelToolbar::Element& el)
{
    el.enabled = can_redo();
    el.tooltip = m_redo_name;
}

void PanelWorkflow::on_redo(PanelToolbar::Element& /*el*/)
{
    redo();
}

void PanelWorkflow::on_pre_project_status(PanelToolbar::Element& el)
{
    el.text = m_history_status;
    el.tooltip = m_tt_history_status;
}

void PanelWorkflow::on_pre_queue_status(PanelToolbar::Element& el)
{
    el.text = "Idle";
    el.tooltip = nullptr;
    m_failed_nodes = nullptr;
    el.blinking = false;
    el.text_clr = 0;

    auto failed = g::pQueueManager->get_is_failed(m_wf->get_id());
    if (failed != nullptr)
    {
        if (TaskResult::Interrupted == failed->result)
        {
            el.text = "Interrupted";
        }
        else
        {
            el.text = failed->err_message.c_str();
            el.tooltip = failed->err_full.c_str();
            m_failed_nodes = &failed->err_nodes;
        }
        el.text_clr = g::pOptsManager->color_load(AppColor::SelectedItem);
        return;
    }

    const auto external = g::pQueueManager->is_external_task();
    if (external > 0)
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
        auto node = group->wf->get_node_by_instance(group->task.current_node);
        if (node != nullptr)
            fmt::format_to(std::back_inserter(m_status_buffer), FMT_COMPILE("Progress: {:.0f}%: {}"), group->task.progress * 100, node->get_name());
        else
            fmt::format_to(std::back_inserter(m_status_buffer), FMT_COMPILE("Progress: {:.0f}%"), group->task.progress * 100);
        m_status_buffer.push_back('\0');
        el.text = m_status_buffer.data();
        return;
    }

    if (g::pQueueManager->is_paused())
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

void PanelWorkflow::on_pre_queue(PanelToolbar::Element& el)
{
    el.enabled = !m_invalidation_state;
}

void PanelWorkflow::on_queue(PanelToolbar::Element& /*el*/)
{
    run(ImGui::IsKeyDown(ImGuiMod_Shift));
}

void PanelWorkflow::on_pre_queue_interrupt(PanelToolbar::Element& el)
{
    auto g = g::pQueueManager->get_current_group();
    el.enabled = g != nullptr && g->src_wf == m_wf->get_id();
}

void PanelWorkflow::on_queue_interrupt(PanelToolbar::Element& /*el*/)
{
    g::pQueueManager->interrupt_task(m_wf->get_id());
}

void PanelWorkflow::on_pre_queue_drop_all(PanelToolbar::Element& el)
{
    auto g = g::pQueueManager->get_current_group();
    el.enabled = (g != nullptr && g->src_wf == m_wf->get_id()) || g::pQueueManager->get_is_queued(m_wf->get_id());
}

void PanelWorkflow::on_queue_drop_all(PanelToolbar::Element& /*el*/)
{
    g::pQueueManager->drop_wf_tasks(m_wf->get_id());
}

void PanelWorkflow::on_tasks_num(PanelToolbar::Element& el)
{
    if (0 == el.cb_item_sel)
        m_tasks_num = -1;
    else
        m_tasks_num = std::strtol(el.cb_items[el.cb_item_sel], nullptr, 10);
}

void PanelWorkflow::on_pre_tasks_num(PanelToolbar::Element& el)
{
    if (-1 == m_tasks_num || 0 == m_tasks_num)
        el.cb_item_sel = 0;
    else
        el.cb_item_sel = m_tasks_num;
}

void PanelWorkflow::on_show_dummy_node(PanelToolbar::Element& el)
{
    if (m_invalidation_state)
        return;

    int32_t idx = 0;
    for (auto& node : m_wf->get_nodes())
    {
        if (node->get_object()->is_dummy)
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
