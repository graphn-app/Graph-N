
#include "ui/panels/panel_wf.h"
#include "ui/graph-n_flow.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/history_manager.h"
#include "core/workflow/workflow.h"
#include "core/workflow/workflow_action.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"

namespace ui
{

PanelWorkflow::PanelWorkflow(wid workflow_id, const std::string& name, const std::string& panel_id, ImGuiID dock)
    : PanelBase(DlgObj::PanelWf, name, panel_id, true)
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
}

void PanelWorkflow::free()
{
    LogF;

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
}

void PanelWorkflow::invalidate_comfyui_objects()
{
    m_invalidation_state = true;
}

void PanelWorkflow::resolve_comfyui_objects(listener::ComfyUiObjectValidityListener::Step /*step*/)
{
    m_invalidation_state = false;
    create_toolbar();
}

DlgCode PanelWorkflow::show(bool& focused)
{
    bool opened = true;
    ImGui::SetNextWindowDockID(m_dock, m_restore_pos ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
    m_restore_pos = false;
    if (begin_panel(opened, focused, false, m_active) && opened)
    {
        //if (m_nodes.empty())
        //{
        //    show_empty_panel_descr(
        //        "",
        //        {},
        //            {
        //                "Empty Canvas.",
        //                "Right Click or load from " ICON_LC_BOOK_OPEN " Nodes Library to add the first node."
        //            }
        //        );
        //}
        //else

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

    return end_panel(opened);
}

void PanelWorkflow::set_wf_view(std::optional<ImRect> wf_rc, std::optional<float> wf_zoom)
{
    if (wf_rc.has_value())
        m_drawer.set_viewport_rect(wf_rc.value());
    if (wf_zoom.has_value())
        m_drawer.set_zoom(wf_zoom.value());
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

        const bool is_new = !node->ed();
        if (is_new)
        {
            node->reset_state();
            if (!node->m_group)
                node->calc_node_width(false, false);
        }

        ImGui::PushID(node);
        m_drawer.pre_draw_node(node);
        if (node->m_group)
            m_drawer.draw_group(node, node_idx);
        else
            m_drawer.draw_node(node, node->get_object()->w_fit, node_idx);
        m_drawer.post_draw_node(node);
        ImGui::PopID();

        if (is_new && m_select_new_nodes)
            m_drawer.select_node(node->ed(), true);
    }

    m_select_new_nodes = false;

    if (m_locate != nullptr)
    {
        m_drawer.locate_node(m_locate);
        m_locate = nullptr;
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

    m_drawer.post_draw_all(
        [this]() { return context_menu_handler(); },
        [this]() { return show_context_menu(); },
        [this]() { return handle_moves(); });

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
                if (m_action->set_group_rect(node, m_drawer.fit_group_rect(node, m_drawer.get_nodes_in_group(node, m_wf->get_nodes()))))
                    add_y.insert(it->first);
                items.erase(node);
            }
        }
    }

    for (auto& [group, nodes] : items)
    {
        m_action->set_group_rect(group, m_drawer.fit_group_rect(group, nodes));
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

        // right

        if (m_wf->get_data().valid)
        {
            {
                PanelToolbar::Element el;
                el.type = PanelToolbar::Element::Type::Text;
                el.text = "Idle";
                el.on_preview = std::bind(&PanelWorkflow::on_pre_queue_status, this, std::placeholders::_1);
                add_toolbar_element(std::move(el), false);
            }
            {
                PanelToolbar::Element el;
                el.type = PanelToolbar::Element::Type::Button;
                el.icon = ICON_LC_PLAY;
                el.tooltip = "Run tasks";
                el.on_preview = std::bind(&PanelWorkflow::on_pre_queue, this, std::placeholders::_1);
                el.on_click = std::bind(&PanelWorkflow::on_queue, this, std::placeholders::_1);
                add_toolbar_element(std::move(el), false);
            }
            {
                PanelToolbar::Element el;
                el.type = PanelToolbar::Element::Type::Button;
                el.icon = ICON_LC_SQUARE;
                el.tooltip = "Stop queue";
                el.on_preview = std::bind(&PanelWorkflow::on_pre_queue_stop, this, std::placeholders::_1);
                el.on_click = std::bind(&PanelWorkflow::on_queue_stop, this, std::placeholders::_1);
                add_toolbar_element(std::move(el), false);
            }
            {
                PanelToolbar::Element el;
                el.type = PanelToolbar::Element::Type::InputInt;
                el.tooltip = "Number of tasks. Zero or less to infinite";
                m_queue_num_input_id = "##num_" + m_wf->get_uuid();
                el.id = m_queue_num_input_id.c_str();
                el.input_w = 100.0f;
                el.buffer_int = &m_tasks_num;
                el.int_min = -1;
                el.int_max = 100;
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
    el.enabled = m_can_save && !m_invalidation_state;
}

void PanelWorkflow::on_save(PanelToolbar::Element& /*el*/)
{
    g::pWfsManager->save(m_wf->get_id());
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
    el.enabled = *m_undo_name != 0 && !m_invalidation_state;
    el.tooltip = m_undo_name;
}

void PanelWorkflow::on_undo(PanelToolbar::Element& /*el*/)
{
    g::pHistoryManager->undo(m_wf->get_id());
}

void PanelWorkflow::on_pre_redo(PanelToolbar::Element& el)
{
    el.enabled = *m_redo_name != 0 && !m_invalidation_state;
    el.tooltip = m_redo_name;
}

void PanelWorkflow::on_redo(PanelToolbar::Element& /*el*/)
{
    g::pHistoryManager->redo(m_wf->get_id());
}

void PanelWorkflow::on_pre_project_status(PanelToolbar::Element& el)
{
    el.text = m_history_status;
    el.tooltip = m_tt_history_status;
}

void PanelWorkflow::on_pre_queue_status(PanelToolbar::Element& /*el*/) {}

void PanelWorkflow::on_pre_queue(PanelToolbar::Element& el)
{
    el.enabled = !m_invalidation_state;
}

void PanelWorkflow::on_queue(PanelToolbar::Element& /*el*/)
{
}

void PanelWorkflow::on_pre_queue_stop(PanelToolbar::Element& el)
{
    el.enabled = !m_invalidation_state;
}

void PanelWorkflow::on_queue_stop(PanelToolbar::Element& /*el*/) {}

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
