
#include "ui/graph-n_flow.h"
#include "ui/panels/panel_wf.h"
#include "ui/panels/panel_wf_list.h"
#include "core/managers/db_manager.h"
#include "core/managers/history_manager.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/hotkeys_manager.h"
#include "core/managers/settings_manager.h"
#include "core/workflow/workflow.h"
#include "core/workflow/workflow_action.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/workflow/workflow_action.h"
#include "core/workflow/pin.h"

namespace ui
{

bool GraphN_Flow::check_wf_panels_initialized()
{
    if (!get_nodes_ready())
        return false;

    auto wf_panels = g::pOptsManager->wf_active_load();
    auto wfs = g::pWfsManager->get_wfs();
    PanelWorkflow* activate = nullptr;

    for (auto& [wf_uuid, active] : wf_panels)
    {
        for (auto& wf_data : wfs)
        {
            if (wf_data->uuid == wf_uuid)
            {
                auto panel = wf_panel_create(*wf_data);
                if (active)
                    activate = panel;
                break;
            }
        }
    }

    if (activate != nullptr)
        wf_panel_activate(activate);

    else if (g::pDbManager->get_is_first_run())
        wf_create();

    return true;
}

// WF public ->
void GraphN_Flow::wf_create()
{
    auto& data = g::pWfsManager->create(true /*generate_name*/, true /*dump_to_db*/, false /*ro*/);
    g::pWfsManager->set_active(data.id);
    auto panel = wf_panel_create(data);
    wf_panel_activate(panel);
    m_panel_wf_list->rebuild_items();
}

void GraphN_Flow::wf_create_copy_from(wid source, std::optional<ImRect> wf_rc, std::optional<float> wf_zoom)
{
    auto info = g::pWfsManager->create_copy(source);
    g::pWfsManager->set_active(info.id);
    auto panel = wf_panel_create(info);
    wf_panel_activate(panel);
    panel->set_wf_view(wf_rc, wf_zoom);
    m_panel_wf_list->rebuild_items();
}

void GraphN_Flow::wf_reload_view(wid id) noexcept
{
    for (auto& panel : m_panels_wf)
    {
        if (panel->get_workflow()->get_id() == id)
        {
            panel->select_new_nodes();
            break;
        }
    }
}

void GraphN_Flow::wf_activate(wid id)
{
    if (0 == id && !m_panels_wf.empty())
        id = m_panels_wf.begin()->get()->get_workflow()->get_id();

    g::pWfsManager->set_active(id);
    auto panel = wf_panel_find(id);
    if (nullptr == panel && id != 0)
    {
        const auto& data = g::pWfsManager->get_data(id);
        panel = wf_panel_create(data);
    }
    wf_panel_activate(panel);

    m_panel_wf_list->rebuild_items();
}

//void GraphN_Flow::wf_locate(wid id)
//{
//    auto panel = wf_panel_find(id);
//    if (nullptr == panel)
//    {
//        auto& data = g::pWfsManager->get_data(id);
//        panel = wf_panel_create(data);
//    }
//    wf_activate(id);
//    panel->restore_pos();
//    m_panel_wf_list->rebuild_items();
//}

void GraphN_Flow::wf_delete(wid id)
{
    auto panel = wf_panel_find(id);
    if (panel != nullptr)
    {
        wf_panel_close(panel);
    }

    g::pWfsManager->delete_wf(id);
    g::pHistoryManager->cleanup(id);
    m_panel_wf_list->rebuild_items();    
}

void GraphN_Flow::wf_restore(wid id)
{
    g::pWfsManager->undelete_wf(id);
    m_panel_wf_list->rebuild_items();
}

void GraphN_Flow::wf_rename(wid id, const std::string& new_name)
{
    g::pWfsManager->update_name(id, new_name);

    auto panel = wf_panel_find(id);
    if (panel != nullptr)
        wf_panel_rename(panel, new_name);

    m_panel_wf_list->rebuild_items();
}

void GraphN_Flow::wf_add_node_to_active_panel(data::Node* node, const std::string& select /*= ""*/)
{
    if (nullptr == m_wf_focused)
        g::pFlow->wf_create();
    if (m_wf_focused != nullptr)
        m_wf_focused->get_action()->add_node(node, select, {});
}

void GraphN_Flow::wf_locate_node(wid id, data::NodeInstance* node)
{
    if (id != 0)
        wf_activate(id);
    if (m_wf_focused != nullptr)
        m_wf_focused->locate_node(node);
}

void GraphN_Flow::wf_reload()
{
    m_panel_wf_list->rebuild_items();
}

void GraphN_Flow::wf_rename() noexcept
{
    m_panel_wf_list->rename_project();
}

wid GraphN_Flow::wf_get_active() const noexcept
{
    return nullptr == m_wf_focused ? 0 : m_wf_focused->get_workflow()->get_id();
}

bool GraphN_Flow::wf_can_run() const noexcept
{
    if (nullptr == m_wf_focused)
        return false;
    return m_wf_focused->can_run();
}

void GraphN_Flow::wf_run(bool add_front) const
{
    if (!wf_can_run())
        return;
    return m_wf_focused->run(add_front);
}

void GraphN_Flow::wf_num_of_runs(int32_t num) const noexcept
{
    if (nullptr == m_wf_focused)
        return;
    m_wf_focused->set_num_of_runs(num);
}

// <- WF public
// WF internal ->

PanelWorkflow* GraphN_Flow::wf_panel_find(wid id) const noexcept
{
    for (const auto& panel : m_panels_wf)
    {
        if (panel->get_workflow()->get_id() == id)
            return panel.get();
    }
    return nullptr;
}

PanelWorkflow* GraphN_Flow::wf_panel_create(const mngr::WfData& wf)
{
    m_panels_wf.emplace_back(std::make_unique<ui::PanelWorkflow>(wf.id, wf.name, wf.uuid, m_main_dock_id));
    auto& panel_wf = m_panels_wf.back();
    panel_wf->init();
    wf_assign_sequential_numbers();
    m_all_dialogs.insert(panel_wf.get());
    m_windows |= ViewPanelWf;
    return panel_wf.get();
}

void GraphN_Flow::wf_panel_activate(PanelWorkflow* wf_panel)
{
    if (m_wf_focused != wf_panel)
    {
        std::list<std::pair<std::string, bool>> active_wfs;

        for (auto& panel : m_panels_wf)
        {
            panel->set_force_focused(false);
            panel->mark_dirty(panel->get_workflow()->get_data().dirty);

            if (wf_panel != panel.get())
                active_wfs.push_back({ panel->get_workflow()->get_uuid(), false });
        }
        if (wf_panel != nullptr)
        {
            wf_panel->set_force_focused(true);
            g::pWfsManager->set_active(wf_panel->get_workflow()->get_id());
            m_panel_wf_list->rebuild_items();
            activate_txt_panels(wf_panel->get_workflow()->get_id());
            active_wfs.push_back({ wf_panel->get_workflow()->get_uuid(), true });
        }

        m_wf_focused = wf_panel;
        g::pOptsManager->wf_active_save(active_wfs);
    }
}

bool GraphN_Flow::wf_panel_activate_next(bool next)
{
    if (nullptr == m_wf_focused || m_panels_wf.size() <= 1)
        return false;

    for (auto it = m_panels_wf.begin(); it != m_panels_wf.end(); ++it)
    {
        if (m_wf_focused->get_panel_id() == (*it)->get_panel_id())
        {
            if (next)
            {
                if (it == m_panels_wf.end()) it = m_panels_wf.begin();
                else ++it;
                if (it == m_panels_wf.end()) it = m_panels_wf.begin();
                wf_panel_activate((*it).get());
            }
            else
            {
                if (it == m_panels_wf.begin()) { it = m_panels_wf.end(); --it; }
                else --it;
                wf_panel_activate((*it).get());
            }
            break;
        }
    }
    return true;
}

void GraphN_Flow::wf_panel_activate_sequential(size_t sequential_number)
{
    size_t number = 0;
    for (auto it = m_panels_wf.begin(); it != m_panels_wf.end(); ++it)
    {
        if (++number == sequential_number)
        {
            wf_panel_activate((*it).get());
            break;
        }
    }
}

void GraphN_Flow::wf_panel_close(PanelWorkflow* wf_panel)
{
    if (nullptr == wf_panel)
        wf_panel = m_wf_focused;

    if (m_wf_focused == wf_panel)
    {
        if (!wf_panel_activate_next(false))
            m_wf_focused = nullptr;
    }

    wf_panel->stop_all_threads();
    m_all_dialogs.erase(wf_panel);

    const wid panel_wf = wf_panel->get_workflow()->get_id();
    txt_panel_clear(panel_wf);
    g::pHistoryManager->cleanup(panel_wf);
    g::pWfsManager->close(panel_wf);

    if (m_panels_wf.empty())
        destroy_txt_panels();
    wf_panel->free();

    std::list<std::pair<std::string, bool>> active_wfs;
    for (auto it = m_panels_wf.begin(); it != m_panels_wf.end();)
    {
        auto& panel = *it;
        if (panel.get() == wf_panel)
        {
            it = m_panels_wf.erase(it);
        }
        else
        {
            active_wfs.push_back({ panel->get_workflow()->get_uuid(), m_wf_focused == panel.get() });
            ++it;
        }
    }

    g::pOptsManager->wf_active_save(active_wfs);
    wf_assign_sequential_numbers();
    on_dlg_closed(DlgObj::PanelWf);
}

void GraphN_Flow::wf_panel_rename(PanelWorkflow* wf_panel, const std::string& new_name)
{
    LogRAssert(wf_panel, "Zero ptr", );
    wf_panel->rename_panel(new_name);
}

void GraphN_Flow::wf_assign_sequential_numbers()
{
    auto& hotkeys = g::pHotkeysManager->get_key_bindings();
    std::set<size_t> numbers;

    for (auto& hk : hotkeys)
    {
        switch (hk.target)
        {
        case KeyTarget::Wf_1:
        case KeyTarget::Wf_2:
        case KeyTarget::Wf_3:
        case KeyTarget::Wf_4:
        case KeyTarget::Wf_5:
        case KeyTarget::Wf_6:
        case KeyTarget::Wf_7:
        case KeyTarget::Wf_8:
        case KeyTarget::Wf_9:
            if (hk.enabled)
            {
                const size_t number = cast::to_szt(hk.target) - cast::to_szt(KeyTarget::Wf_1) + 1;
                numbers.insert(number);
            }
        }
    }

    size_t sequential_number = 1;
    for (auto it = m_panels_wf.begin(); it != m_panels_wf.end(); ++it)
    {
        auto& panel = *it;
        panel->set_sequential_number(numbers.contains(sequential_number) ? sequential_number : 0);
        sequential_number++;
    }
}

bool GraphN_Flow::wf_save(bool check_only)
{
    if (check_only)
        return (m_wf_focused != nullptr && m_wf_focused->can_save());

    if (wf_save(true))
    {
        m_wf_focused->save();
    }
    return true;
}

bool GraphN_Flow::wf_undo(bool check_only)
{
    if (check_only)
        return (m_wf_focused != nullptr && m_wf_focused->can_undo());

    if (wf_undo(true))
    {
        m_wf_focused->undo();
    }
    return true;
}

bool GraphN_Flow::wf_redo(bool check_only)
{
    if (check_only)
        return (m_wf_focused != nullptr && m_wf_focused->can_redo());
    
    if (wf_redo(true))
    {
        m_wf_focused->redo();
    }
    return true;
}

void GraphN_Flow::wf_focus() noexcept
{
    if (m_wf_focused != nullptr)
    {
        m_wf_focused->locate_selected_node();
    }
}

void GraphN_Flow::wf_new_node() noexcept
{
    if (m_wf_focused != nullptr)
    {
        m_wf_focused->display_new_node_menu();
    }
}

void GraphN_Flow::wf_node_get_any_selected(bool& sel_nodes, bool& sel_groups) noexcept
{
    sel_nodes = sel_groups = false;
    if (m_wf_focused != nullptr)
    {
        const auto* wf = m_wf_focused->get_workflow();
        auto& nodes = wf->get_nodes_all();
        for (auto& node : nodes)
        {
            if (m_wf_focused->get_node_selected(node.get()))
            {
                if (node->flags().has(NodeInstFlags::Group)) sel_groups = true;
                else sel_nodes = true;

                if (sel_groups && sel_nodes)
                    return;
            }            
        }
    }
}

void GraphN_Flow::wf_node_select_all() noexcept
{
    if (m_wf_focused != nullptr)
    {
        m_wf_focused->select_all_nodes();
    }
}

void GraphN_Flow::wf_node_action_on_selected(
    const std::function<const char*(bool)>& action_name_cb,
    const std::function<bool(data::NodeInstance*)>& first_cb,
    const std::function<void(data::NodeInstance*, data::WorkflowAction*, bool)>& action_cb)
{
    if (m_wf_focused != nullptr)
    {        
        const auto selected = m_wf_focused->get_selected_nodes(false);
        if (!selected.empty())
        {
            const bool first = first_cb(*selected.begin());
            auto action = m_wf_focused->get_action();

            action->multi_action_on(action_name_cb(first));
            for (auto node : selected)
            {
                action_cb(node, action, first);
            }
            action->multi_action_off();
        }
    }
}

void GraphN_Flow::wf_node_toggle_pin()
{
    const auto action_name_cb = [](bool first) noexcept
    {
        return first ? "Unpin" : "Pin";
    };
    const auto first_cb = [](data::NodeInstance* node) noexcept
    {
        return node->flags().has(NodeInstFlags::Pinned);
    };
    const auto action_cb = [](data::NodeInstance* node, data::WorkflowAction* action, bool first)
    {
        action->set_node_pinning(node, !first);
    };
    wf_node_action_on_selected(action_name_cb, first_cb, action_cb);
}

void GraphN_Flow::wf_node_toggle_collapse()
{
    const auto action_name_cb = [](bool first) noexcept
    {
        return first ? "Collapse" : "Expand";
    };
    const auto first_cb = [](data::NodeInstance* node) noexcept
    {
        return node->flags().has(NodeInstFlags::Expanded);
    };
    const auto action_cb = [](data::NodeInstance* node, data::WorkflowAction* action, bool first)
    {
        action->set_node_expanded(node, !first);
    };
    wf_node_action_on_selected(action_name_cb, first_cb, action_cb);
}

void GraphN_Flow::wf_node_group()
{
    if (m_wf_focused != nullptr)
    {
        const std::set<data::NodeInstance*> nodes = m_wf_focused->get_selected_nodes(false);
        m_wf_focused->group_nodes(nodes);
    }
}

void GraphN_Flow::wf_node_always()
{
    const auto action_name_cb = [](bool /*first*/)
    {
        return "Set mode";
    };
    const auto first_cb = [](data::NodeInstance* /*node*/)
    {
        return true;
    };
    const auto action_cb = [](data::NodeInstance* node, data::WorkflowAction* action, bool /*first*/)
    {
        action->set_node_mode(node, NodeInstFlags::ModeAlways);
    };
    wf_node_action_on_selected(action_name_cb, first_cb, action_cb);
}

void GraphN_Flow::wf_node_toggle_mute()
{
    const auto action_name_cb = [](bool /*first*/) noexcept
    {
        return "Set mode";
    };
    const auto first_cb = [](data::NodeInstance* node) noexcept
    {
        return node->get_mode_mute();
    };
    const auto action_cb = [](data::NodeInstance* node, data::WorkflowAction* action, bool first)
    {
        action->set_node_mode(node, first ? NodeInstFlags::ModeAlways : NodeInstFlags::ModeMute);
    };
    wf_node_action_on_selected(action_name_cb, first_cb, action_cb);
}

void GraphN_Flow::wf_node_toggle_bypass()
{
    const auto action_name_cb = [](bool /*first*/) noexcept
    {
        return "Set mode";
    };
    const auto first_cb = [](data::NodeInstance* node) noexcept
    {
        return node->get_mode_bypass();
    };
    const auto action_cb = [](data::NodeInstance* node, data::WorkflowAction* action, bool first)
    {
        action->set_node_mode(node, first ? NodeInstFlags::ModeAlways : NodeInstFlags::ModeBypass);
    };
    wf_node_action_on_selected(action_name_cb, first_cb, action_cb);
}

void GraphN_Flow::wf_node_remove()
{
    if (m_wf_focused != nullptr)
    {
        std::set<data::NodeInstance*> nodes = m_wf_focused->get_selected_nodes(true);
        m_wf_focused->delete_nodes(nodes);
    }
}

void GraphN_Flow::wf_node_copy()
{
    if (m_wf_focused != nullptr)
    {
        std::set<data::NodeInstance*> nodes = m_wf_focused->get_selected_nodes(true);
        m_wf_focused->copy_nodes(nodes, {});
    }
}

bool GraphN_Flow::wf_node_paste(bool check_only)
{
    if (check_only)
        return (m_wf_focused != nullptr && g::pWfsManager->copy_has_data());

    if (m_wf_focused != nullptr)
    {
        m_wf_focused->paste_copied({});
    }
    return true;
}

void GraphN_Flow::wf_node_duplicate()
{
    wf_node_copy();
    wf_node_paste(false);
}

void GraphN_Flow::wf_node_move(bool left, bool right, bool up, bool down)
{
    const auto action_name_cb = [left, right, up, down](bool /*first*/) noexcept
    {
        if (left) return "Move left";
        if (right) return "Move right";
        if (up) return "Move up";
        return "Move down";
    };
    const auto first_cb = [](data::NodeInstance* /*node*/) noexcept
    {
        return true;
    };
    const auto action_cb = [left, right, up, down](data::NodeInstance* node, data::WorkflowAction* action, bool /*first*/)
    {
        ImRect rc = node->m_rc;
        if (left)       rc.Min.x--;
        else if (up)    rc.Min.y--;
        else if (right) rc.Min.x++;
        else            rc.Min.y++;
        action->set_node_rect(node, rc);
    };
    wf_node_action_on_selected(action_name_cb, first_cb, action_cb);
}

void GraphN_Flow::wf_node_align(bool left, bool top)
{
    if (m_wf_focused != nullptr)
    {
        auto selected = m_wf_focused->get_selected_nodes(false);
        if (!selected.empty())
        {
            float pos = FLT_MAX;
            for (auto node : selected)
            {
                if (left && node->m_rc.Min.x < pos)
                    pos = node->m_rc.Min.x;
                else if (top && node->m_rc.Min.y < pos)
                    pos = node->m_rc.Min.y;
            }

            auto action = m_wf_focused->get_action();
            action->multi_action_on(left ? "Align left" : "Align top");

            for (auto node : selected)
            {
                float side = 0.0f;
                ImRect rc = node->m_rc;
                if (left)
                {
                    side = rc.Max.x - rc.Min.x;
                    rc.Min.x = pos;
                    rc.Max.x = rc.Min.x + side;
                }
                else
                {
                    side = rc.Max.y - rc.Min.y;
                    rc.Min.y = pos;
                    rc.Max.y = rc.Min.y + side;
                }
                action->set_node_rect(node, rc);
            }

            action->multi_action_off();
        }
    }
}

void GraphN_Flow::wf_node_unlink()
{
    if (nullptr == m_wf_focused)
        return;

    auto action = m_wf_focused->get_action();
    action->multi_action_on("Unlink");

    const auto unlink_pins = [action](const std::function<std::vector<data::PinInstance*>()>& get_pins)
    {
        auto pins = get_pins();
        for (auto it = pins.begin(); it != pins.end(); )
        {
            const auto* pin = *it;
            if (pin->state().links > 0)
            {
                action->unlink(pin);
                pins = get_pins();
                it = pins.begin();
            }
            else
                ++it;
        }
    };

    auto selected = m_wf_focused->get_selected_nodes(true);
    for (auto node : selected)
    {
        unlink_pins([node] { return node->get_inputs(); });
        unlink_pins([node] { return node->get_outputs(); });
    }

    action->multi_action_off();
}

}

