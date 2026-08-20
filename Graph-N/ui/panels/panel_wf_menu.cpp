
#include "ui/panels/panel_wf.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/workflow/workflow_action.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/history_manager.h"
#include "core/managers/hotkeys_manager.h"
#include "ui/graph-n_flow.h"

namespace ui
{

bool PanelWorkflow::context_menu_handler()
{
    if (m_invalidation_state || m_wf->get_data().critical_failure)
        return false;

    if (m_display_new_node_menu)
    {
        m_display_new_node_menu = false;
        m_handle_context_menu = true;
        m_menu_objs = {};
        m_context_menu_type = MenuType::BackDblClick;
        if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_ChildWindows))
        {
            const ImVec2 win_pos = ImGui::GetWindowPos();
            const ImVec2 start_pos = ImGui::GetCursorStartPos();
            const auto offset = ImGui::GetTextLineHeight() + ImGui::GetStyle().FramePadding.y + 10.0f;
            m_menu_objs.click_pos = ImVec2(win_pos.x + start_pos.x + 10.0f, win_pos.y + start_pos.y + offset);
            m_menu_objs.align_menu_pos = true;
        }
        else
            m_menu_objs.click_pos = m_drawer.get_mouse_pos();

        Log("Bg menu initiated: new node");
        return true;
    }

    bool rclick = false, dbl_click = false;
    if (m_drawer.get_bg_menu_active(rclick, dbl_click))
    {
        m_handle_context_menu = true;
        m_menu_objs = {};
        m_context_menu_type = rclick ? MenuType::Back : (dbl_click ? MenuType::BackDblClick : MenuType::_none);
        m_menu_objs.click_pos = m_drawer.get_mouse_pos();
        Log("Bg menu initiated");
        return true;
    }

    if (!ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        return false;

    {
        const auto pin = m_drawer.get_hovered_pin();
        if (pin != nullptr)
        {
            const auto parent = m_wf->get_node_by_instance(pin->get_parent_node());
            if (parent->get_expanded() && !parent->get_object()->is_dummy)
            {
                m_handle_context_menu = true;
                m_menu_objs = {};
                m_context_menu_type = MenuType::Pin;
                m_menu_objs.pins.insert(pin);
                m_menu_objs.nodes.insert(parent);
                m_menu_objs.click_pos = m_drawer.get_mouse_pos();
                Log("Pin menu initiated");
                return true;
            }
        }
    }

    {
        const auto link = m_drawer.get_hovered_link();
        if (link != nullptr)
        {
            m_handle_context_menu = true;
            m_menu_objs = {};
            m_menu_objs.click_pos = m_drawer.get_mouse_pos();

            auto links = m_drawer.get_selected_links();
            if (!m_drawer.is_link_selected(link->ed()) || 1 == links.size())
            {
                m_drawer.select_link(link->ed(), false);
                m_context_menu_type = MenuType::Link;
                m_menu_objs.links.insert(link);
                Log("Link menu initiated");
            }
            else
            {
                m_context_menu_type = MenuType::Multilink;
                m_menu_objs.links = std::move(links);
                Log("Multilink menu initiated");
            }
            return true;
        }
    }

    {
        const auto node = m_drawer.get_hovered_node();
        if (node != nullptr)
        {
            m_handle_context_menu = true;
            m_menu_objs = {};
            m_menu_objs.click_pos = m_drawer.get_mouse_pos();

            auto nodes = m_drawer.get_selected_nodes(false);
            if (!m_drawer.is_node_selected(node->ed()))
            {
                m_drawer.select_node(node->ed(), false);
                nodes = m_drawer.get_selected_nodes(false);

                if (node->m_group)
                    m_menu_objs.groups.insert({ node, m_drawer.get_nodes_in_group(node, m_wf->get_nodes_all()) });
                else
                    m_menu_objs.nodes.insert(node);
                m_context_menu_type = node->m_group ? MenuType::Group : MenuType::Node;
                Log((node->m_group ? "Group" : "Node") << " menu initiated");
            }
            else
            {
                for (auto node_it : nodes)
                {
                    if (node_it->m_group)
                    {
                        m_menu_objs.groups.insert({ node_it, m_drawer.get_nodes_in_group(node_it, m_wf->get_nodes_all()) });
                    }
                    else
                    {
                        m_menu_objs.nodes.insert(node_it);
                    }
                }

                if (!m_menu_objs.groups.empty() && m_menu_objs.nodes.empty())
                {
                    m_context_menu_type = m_menu_objs.groups.size() == 1 ? MenuType::Group : MenuType::Multigroup;
                    Log("Multigroup menu initiated");
                }
                else if (m_menu_objs.groups.empty() && !m_menu_objs.nodes.empty())
                {
                    m_context_menu_type = m_menu_objs.nodes.size() == 1 ? MenuType::Node : MenuType::Multinode;
                    Log("Multinode menu initiated");
                }
                else
                {
                    m_context_menu_type = MenuType::Multiobj;
                    Log("Multiobj menu initiated");
                }
            }
            return true;
        }
    }

    return true;
}

bool PanelWorkflow::show_context_menu()
{
    if (m_handle_context_menu)
    {
        ImGui::OpenPopup(m_context_menuid.c_str());
        m_handle_context_menu = false;
        m_context_menu_nodes_need_focus = true;
        m_context_menu_nodes_filter.clear();
        m_context_menu_nodes_filter.shrink_to_fit();
    }

    if (m_menu_objs.align_menu_pos)
        ImGui::SetNextWindowPos(m_menu_objs.click_pos, ImGuiCond_Appearing);

    if (ImGui::BeginPopup(m_context_menuid.c_str()))
    {
        bool close = false;
        switch (m_context_menu_type)
        {
        case MenuType::Back:
            close = show_canvas_menu();
            break;
        case MenuType::BackDblClick:
            close = show_canvas_nodes_menu();
            break;
        case MenuType::Group:
            close = show_group_menu();
            break;
        case MenuType::Node:
            close = show_node_menu();
            break;
        case MenuType::Pin:
            close = show_pin_menu();
            break;
        case MenuType::Link:
            close = show_link_menu();
            break;
        case MenuType::Multigroup:
            close = show_multigroup_menu();
            break;
        case MenuType::Multilink:
            close = show_multilink_menu();
            break;
        case MenuType::Multinode:
            close = show_multinode_menu();
            break;
        case MenuType::Multiobj:
            close = show_multiobj_menu();
            break;
        case MenuType::SpawnNode:
            close = show_new_node_menu();
            break;
        default:
            LogError("Unhandled menu type");
            close = true;
        }

        if (close)
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return true;
    }

    return false;
}

bool PanelWorkflow::show_canvas_menu()
{    
    bool close = false;
    
    ImGui::MenuItem(m_wf->get_data().name.c_str(), nullptr, nullptr, false);
    ImGui::Separator();
    if (ImGui::BeginMenu("Add node##allnodes"))
    {
        auto node = show_nodes_menu({}, {});
        if (node != nullptr)
        {
            close = true;
            m_action->add_node(node, "", ImRect(m_menu_objs.click_pos, ImVec2(-1, -1)));
        }
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Add group"))
    {
        m_action->add_group(ImRect(m_menu_objs.click_pos, ImVec2(-1, -1)));
        close = true;
    }
    if (ImGui::MenuItem("Fit all"))
    {
        m_action->multi_action_on("Fit");
        fit(m_menu_objs.groups);
        m_action->multi_action_off();
        close = true;
    }
    ImGui::Separator();

    const auto history = g::pHistoryManager->get_state(m_wf->get_id());
    const char* undo = nullptr;
    const char* hotkey = g::pHotkeysManager->get_key_binding_name(KeyTarget::Workspace_Undo);
    if (history.undo_name != nullptr)
    {
        m_fmt_buffer.clear();
        fmt::format_to(std::back_inserter(m_fmt_buffer), FMT_STRING("Undo: {}"), *history.undo_name);
        m_fmt_buffer.push_back('\0');
        undo = m_fmt_buffer.data();
    }
    if (ImGui::MenuItem(nullptr == undo ? "Undo" : undo, hotkey, nullptr, undo != nullptr))
    {
        m_drawer.clear_selection();
        g::pHistoryManager->undo(m_wf->get_id());
        close = true;
    }

    const char* redo = nullptr;
    hotkey = g::pHotkeysManager->get_key_binding_name(KeyTarget::Workspace_Redo);
    if (history.redo_name != nullptr)
    {
        m_fmt_buffer.clear();
        fmt::format_to(std::back_inserter(m_fmt_buffer), FMT_STRING("Redo: {}"), *history.redo_name);
        m_fmt_buffer.push_back('\0');
        redo = m_fmt_buffer.data();        
    }
    if (ImGui::MenuItem(nullptr == redo ? "Redo" : redo, hotkey, nullptr, redo != nullptr))
    {
        m_drawer.clear_selection();
        g::pHistoryManager->redo(m_wf->get_id());
        close = true;
    }
    ImGui::Separator();

    if (ImGui::MenuItem("Paste", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Paste), nullptr, g::pWfsManager->copy_has_data()))
    {
        paste_copied(m_menu_objs.click_pos);
        close = true;
    }
    if (ImGui::MenuItem("Select all", g::pHotkeysManager->get_key_binding_name(KeyTarget::Workspace_SelAll)))
    {
        m_drawer.select_node(EdType(false), false);
        for (auto& node : m_wf->get_nodes_all())
        {
            m_drawer.select_node(node->ed(), true);
        }
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Duplicate workflow", nullptr, nullptr, !m_wf->get_nodes_all().empty() && m_wf->get_data().valid))
    {
        g::pFlow->wf_create_copy_from(m_wf->get_id(), m_drawer.get_viewport_rect(), m_drawer.get_zoom());
        close = true;
    }
    if (ImGui::MenuItem("Reset workflow window position"))
    {
        restore_pos();
        close = true;
    }
    return close;
}

bool PanelWorkflow::show_canvas_nodes_menu()
{
    bool close = false;
    auto node = show_nodes_menu({}, {});
    if (node != nullptr)
    {
        close = true;
        m_action->add_node(node, "", ImRect(m_menu_objs.click_pos, ImVec2(-1, -1)));
    }
    return close;
}

bool PanelWorkflow::show_node_menu()
{
    bool close = false;
    auto node = *m_menu_objs.nodes.begin();

    if (ImGui::MenuItem("Rename", nullptr, nullptr, !node->get_object()->is_dummy && node->get_object()->can_rename))
    {
        m_drawer.rename_node(node);
        close = true;
    }
    if (!node->get_object()->is_dummy && node->get_name() != node->get_name_original())
    {
        if (ImGui::MenuItem("Reset node name"))
        {
            m_action->reset_node_name(node);
            g::pFlow->txt_panel_update(m_wf->get_id(), node);
            close = true;
        }
    }
    //if (ImGui::MenuItem("Fit"))   // TODO:FIX resizing
    //{
    //    node->reset_width();
    //    close = true;
    //}
    if (ImGui::MenuItem(node->get_pinned() ? "Unpin" : "Pin", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Pin)))
    {
        m_action->set_node_pinning(node, !node->get_pinned());
        close = true;
    }
    if (ImGui::MenuItem(node->get_expanded() ? "Collapse" : "Expand", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Collapse)))
    {
        m_action->set_node_expanded(node, !node->get_expanded());
        close = true;
    }
    if (ImGui::MenuItem("Group", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Group)))
    {
        group_nodes({ node });
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Always", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Always), node->get_mode() == NodeMode::Always, !node->get_object()->is_dummy))
    {
        m_action->set_node_mode(node, NodeMode::Always);
        close = true;
    }
    if (ImGui::MenuItem("Bypass", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Bypass), node->get_mode() == NodeMode::Bypass, node->get_object()->can_bypass && !node->get_object()->is_dummy))
    {
        m_action->set_node_mode(node, NodeMode::Bypass);
        close = true;
    }
    if (ImGui::MenuItem("Mute", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Mute), node->get_mode() == NodeMode::Mute || node->get_mode() == NodeMode::OnTrigger || node->get_mode() == NodeMode::OnEvent, node->get_object()->can_mute && !node->get_object()->is_dummy))
    {
        m_action->set_node_mode(node, NodeMode::Mute);
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Remove", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Remove)))
    {
        delete_nodes({ node });
        close = true;
        create_toolbar();
        return close;
    }
    if (ImGui::MenuItem("Copy", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Copy), nullptr, !node->get_object()->is_dummy))
    {
        copy_nodes({ node }, {});
        close = true;
    }
    ImGui::Separator();

    if (node->is_cni_reroute())
    {
        if (ImGui::MenuItem("Separate Reroute", nullptr, nullptr, !node->get_object()->is_dummy && node->get_inputs().size() > 2))
        {
            separate_reroutes(m_menu_objs.nodes);
            close = true;
        }
        if (node->is_cni_reroute_splitted())
        {
            if (ImGui::MenuItem("Merge Pair"))
            {
                merge_reroute_parts(*m_menu_objs.nodes.begin(), true);
                close = true;
            }
        }
        else
        {
            if (ImGui::MenuItem("Split into Linked Pair"))
            {
                split_reroute_to_parts(*m_menu_objs.nodes.begin());
                close = true;
            }
        }
        ImGui::Separator();
    }

    //if (ImGui::MenuItem("Convert to subgraph")) // TODO
    //{
    //    close = true;
    //}
    //ImGui::Separator();

    if (ImGui::BeginMenu("Color"))
    {
        bool reset = false;
        if (show_colors_menu(close, reset, true, node->m_clr_node))
        {
            if (reset)
            {
                m_action->reset_node_color(node);
            }
            else
            {
                m_action->set_node_color(node, ImGui::ColorConvertFloat4ToU32(ImVec4(m_color_buff[0], m_color_buff[1], m_color_buff[2], m_color_buff[3])));
            }
        }

        ImGui::EndMenu();
    }
    else
        m_color_buff[0] = -1.0f;

    return close;
}

bool PanelWorkflow::show_group_menu()
{
    bool close = false;
    auto group = m_menu_objs.groups.begin()->first;
    auto& nodes = m_menu_objs.groups.begin()->second;

    if (ImGui::MenuItem("Rename"))
    {
        m_drawer.rename_node(group);
        close = true;
    }
    if (!nodes.empty())
    {
        if (ImGui::MenuItem("Fit"))
        {
            m_action->set_node_rect(group, m_drawer.fit_group_rect(group, nodes));
            close = true;
        }
        if (ImGui::MenuItem("Select nodes"))
        {
            m_drawer.select_node(EdType(false), false);
            for (auto node : nodes)
            {
                m_drawer.select_node(node->ed(), true);
            }
        }
    }
    ImGui::Separator();
    if (ImGui::MenuItem(nodes.empty() ? "Remove" : "Remove group", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Remove)))
    {
        delete_nodes({ group });
        close = true;
    }
    if (!nodes.empty())
    {
        m_fmt_buffer.clear();
        fmt::format_to(std::back_inserter(m_fmt_buffer), FMT_STRING("Remove group and {} node(s) inside"), nodes.size());
        m_fmt_buffer.push_back('\0');

        if (ImGui::MenuItem(m_fmt_buffer.data()))
        {
            std::set<data::NodeInstance*> objs;
            for (auto node : nodes)
            {
                objs.insert(node);
            }
            objs.insert(group);
            delete_nodes(objs);

            close = true;
            create_toolbar();
            return close;
        }
    }
    if (ImGui::MenuItem("Copy", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Copy)))
    {
        copy_nodes({ group }, {});
        close = true;
    }
    ImGui::Separator();
    if (ImGui::BeginMenu("Color"))
    {
        static bool for_all = false;
        if (!nodes.empty())
        {
            ImGui::Checkbox("Apply to nodes in group", &for_all);
            ImGui::Separator();
        }
        bool reset = false;
        if (show_colors_menu(close, reset, for_all, ImGui::ColorConvertU32ToFloat4(group->m_clr_group)))
        {
            m_action->multi_action_on("Set color");
            if (reset)
            {
                m_action->reset_node_color(group);
                if (for_all)
                {
                    for (auto node : nodes)
                    {
                        m_action->reset_node_color(node);
                    }
                }
            }
            else
            {
                const auto color = ImGui::ColorConvertFloat4ToU32(ImVec4(m_color_buff[0], m_color_buff[1], m_color_buff[2], m_color_buff[3]));
                m_action->set_node_color(group, color);
                if (for_all)
                {
                    for (auto node : nodes)
                    {
                        m_action->set_node_color(node, color);
                    }
                }
            }
            m_action->multi_action_off();
        }

        ImGui::EndMenu();
    }
    else
        m_color_buff[0] = -1.0f;

    return close;
}

bool PanelWorkflow::show_multinode_menu()
{
    bool close = false;
    //if (ImGui::MenuItem("Fit"))   // TODO:FIX resizing
    //{
    //    for (auto node : m_menu_objs.nodes)
    //    {
    //        node->reset_width();
    //    }
    //    close = true;
    //}
    if (ImGui::MenuItem("Group", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Group)))
    {
        group_nodes(m_menu_objs.nodes);
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Remove", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Remove)))
    {
        delete_nodes(m_menu_objs.nodes);
        close = true;
        create_toolbar();
        return close;
    }
    if (ImGui::MenuItem("Copy", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Copy)))
    {
        copy_nodes(m_menu_objs.nodes, {});        
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Pin", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Pin)))
    {
        m_action->multi_action_on("Pin");
        for (auto node : m_menu_objs.nodes)
        {
            m_action->set_node_pinning(node, true);
        }
        m_action->multi_action_off();
        close = true;
    }
    if (ImGui::MenuItem("Unpin"))
    {
        m_action->multi_action_on("Unpin");
        for (auto node : m_menu_objs.nodes)
        {
            m_action->set_node_pinning(node, false);
        }
        m_action->multi_action_off();
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Expand"))
    {
        for (auto node : m_menu_objs.nodes)
        {
            m_action->set_node_expanded(node, true);
        }
        close = true;
    }
    if (ImGui::MenuItem("Collapse", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Collapse)))
    {
        for (auto node : m_menu_objs.nodes)
        {
            m_action->set_node_expanded(node, false);
        }
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Always", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Always)))
    {
        m_action->multi_action_on("Always mode");
        for (auto node : m_menu_objs.nodes)
        {
            if (!node->get_object()->is_dummy)
                m_action->set_node_mode(node, NodeMode::Always);
        }
        m_action->multi_action_off();
        close = true;
    }
    if (ImGui::MenuItem("Bypass", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Bypass)))
    {
        m_action->multi_action_on("Bypass");
        for (auto node : m_menu_objs.nodes)
        {
            if (!node->get_object()->is_dummy)
                m_action->set_node_mode(node, NodeMode::Bypass);
        }
        m_action->multi_action_off();
        close = true;
    }
    if (ImGui::MenuItem("Mute", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Mute)))
    {
        m_action->multi_action_on("Mute");
        for (auto node : m_menu_objs.nodes)
        {
            if (!node->get_object()->is_dummy)
                m_action->set_node_mode(node, NodeMode::Mute);
        }
        m_action->multi_action_off();
        close = true;
    }
    ImGui::Separator();

    bool all_reroutes = true;
    for (auto node : m_menu_objs.nodes)
    {
        if (!node->is_cni_reroute())
        {
            all_reroutes = false;
            break;
        }
    }
    if (all_reroutes)
    {
        if (ImGui::MenuItem("Separate Reroutes", nullptr, nullptr, true))
        {
            separate_reroutes(m_menu_objs.nodes);
            close = true;
        }
        if (ImGui::MenuItem("Bundle Reroutes", nullptr, nullptr, true))
        {
            bundle_reroutes(m_menu_objs.nodes);
            close = true;
        }
        ImGui::Separator();
    }

    if (ImGui::BeginMenu("Color"))
    {
        bool reset = false;
        if (show_colors_menu(close, reset, true, (*m_menu_objs.nodes.begin())->m_clr_node))
        {
            m_action->multi_action_on("Set color");
            const auto color = ImGui::ColorConvertFloat4ToU32(ImVec4(m_color_buff[0], m_color_buff[1], m_color_buff[2], m_color_buff[3]));
            for (auto node : m_menu_objs.nodes)
            {
                if (reset)
                    m_action->reset_node_color(node);
                else
                {
                    m_action->set_node_color(node, color);
                }
            }
            m_action->multi_action_off();
        }

        ImGui::EndMenu();
    }
    else
        m_color_buff[0] = -1.0f;

    return close;
}

bool PanelWorkflow::show_multigroup_menu()
{
    size_t nodes_inside = 0;
    for (const auto& group : m_menu_objs.groups)
    {
        nodes_inside += group.second.size();
    }

    bool close = false;
    if (ImGui::MenuItem("Remove", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Remove)))
    {
        std::set<data::NodeInstance*> objs;
        for (const auto& pair : m_menu_objs.groups)
        {
            objs.insert(pair.first);
        }
        delete_nodes(objs);
        close = true;
    }
    if (nodes_inside > 0)
    {
        if (ImGui::MenuItem("Remove groups and nodes inside"))
        {
            std::set<data::NodeInstance*> objs;
            for (const auto&[group, nodes] : m_menu_objs.groups)
            {
                objs.insert(group);
                objs.insert(nodes.begin(), nodes.end());
            }
            delete_nodes(objs);
            close = true;
            create_toolbar();
            return close;
        }
    }
    if (ImGui::MenuItem("Copy", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Copy)))
    {
        std::set<data::NodeInstance*> objs;
        for (auto& group : m_menu_objs.groups)
        {
            objs.insert(group.first);
        }
        copy_nodes(objs, {});
        close = true;
    }
    ImGui::Separator();
    if (nodes_inside > 0)
    {
        if (ImGui::MenuItem("Fit"))
        {
            m_action->multi_action_on("Fit");
            for (const auto&[group, nodes] : m_menu_objs.groups)
            {
                m_action->set_node_rect(group, m_drawer.fit_group_rect(group, nodes));
            }
            m_action->multi_action_off();
            close = true;
        }
        ImGui::Separator();
    }
    if (ImGui::BeginMenu("Color"))
    {
        static bool for_all = false;
        if (nodes_inside > 0)
        {
            ImGui::Checkbox("Apply to nodes in group", &for_all);
            ImGui::Separator();
        }
        bool reset = false;
        if (show_colors_menu(close, reset, for_all, ImGui::ColorConvertU32ToFloat4(m_menu_objs.groups.begin()->first->m_clr_group)))
        {
            m_action->multi_action_on("Set color");
            const auto color = ImGui::ColorConvertFloat4ToU32(ImVec4(m_color_buff[0], m_color_buff[1], m_color_buff[2], m_color_buff[3]));
            for (const auto& [group, nodes] : m_menu_objs.groups)
            {
                if (reset)
                    m_action->reset_node_color(group);
                else
                    m_action->set_node_color(group, color);

                if (for_all)
                {
                    for (auto node : nodes)
                    {
                        if (reset)
                            m_action->reset_node_color(node);
                        else
                            m_action->set_node_color(node, color);
                    }
                }
            }
            m_action->multi_action_off();
        }

        ImGui::EndMenu();
    }
    else
        m_color_buff[0] = -1.0f;

    return close;
}

bool PanelWorkflow::show_multiobj_menu()
{
    bool close = false;
    if (ImGui::MenuItem("Group", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Group)))
    {
        std::set<data::NodeInstance*> objs;
        for (auto node : m_menu_objs.nodes)
        {
            objs.insert(node);
        }
        for (auto& group : m_menu_objs.groups)
        {
            objs.insert(group.first);
        }

        group_nodes(objs);
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Remove", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Remove)))
    {
        std::set<data::NodeInstance*> objs;
        for (auto node : m_menu_objs.nodes)
        {
            objs.insert(node);
        }
        for (auto& group : m_menu_objs.groups)
        {
            objs.insert(group.first);
        }
        delete_nodes(objs);
        close = true;
        create_toolbar();
        return close;
    }
    if (ImGui::MenuItem("Copy", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Copy)))
    {
        std::set<data::NodeInstance*> objs;
        for (auto node : m_menu_objs.nodes)
        {
            if (!node->get_object()->is_dummy)
                objs.insert(node);
        }
        for (auto& group : m_menu_objs.groups)
        {
            objs.insert(group.first);
        }
        copy_nodes(objs, {});
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Fit"))
    {
        m_action->multi_action_on("Fit");
        for (auto node : m_menu_objs.nodes)
        {
            m_action->reset_node_size(node);
        }
        fit(m_menu_objs.groups);
        m_action->multi_action_off();
        close = true;
    }
    ImGui::Separator();
    if (ImGui::BeginMenu("Color"))
    {
        bool reset = false;
        if (show_colors_menu(close, reset, false, ImGui::ColorConvertU32ToFloat4(m_menu_objs.groups.begin()->first->m_clr_group)))
        {
            m_action->multi_action_on("Set color");
            const auto color = ImGui::ColorConvertFloat4ToU32(ImVec4(m_color_buff[0], m_color_buff[1], m_color_buff[2], m_color_buff[3]));
            for (const auto& [group, nodes] : m_menu_objs.groups)
            {
                if (reset)
                    m_action->reset_node_color(group);
                else
                    m_action->set_node_color(group, color);
            }
            for (const auto node : m_menu_objs.nodes)
            {
                if (reset)
                    m_action->reset_node_color(node);
                else
                    m_action->set_node_color(node, color);
            }
            m_action->multi_action_off();
        }

        ImGui::EndMenu();
    }
    else
        m_color_buff[0] = -1.0f;

    return close;
}

bool PanelWorkflow::show_pin_menu()
{
    LogRAssert(m_menu_objs.pins.size() == 1, "More than one pin cannot be selected", true);

    bool close = false;

    auto pin = *m_menu_objs.pins.begin();
    const bool linked = (pin->state().links > 0);
    const bool is_widget = pin->get_object()->is_widget(pin->state());
    const bool name_eq = (pin->state().name == pin->get_object()->def_state.name);
    const bool can_rename = pin->get_object()->can_rename;

    if (ImGui::MenuItem("Unlink", nullptr, nullptr, linked && pin->get_object()->can_unlink))
    {
        m_action->unlink(pin); // pin might be destroyed here
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Rename slot", nullptr, nullptr, !is_widget && can_rename))
    {
        m_drawer.rename_pin(pin);
        close = true;
    }
    if (ImGui::MenuItem("Reset slot name", nullptr, nullptr, !is_widget && !name_eq && can_rename))
    {
        m_action->reset_pin_name(pin);
        close = true;
    }

    return close;
}

bool PanelWorkflow::show_link_menu()
{
    bool close = false;
    const auto link = *m_menu_objs.links.begin();
    const bool can_unlink = (link->get_input()->get_object()->can_unlink && link->get_output()->get_object()->can_unlink);

    if (ImGui::MenuItem("Unlink", nullptr, nullptr, can_unlink))
    {
        m_action->unlink(link);
        close = true;
    }
    if (ImGui::MenuItem("Reroute", nullptr, nullptr, can_unlink))
    {
        insert_reroute(link, m_menu_objs.click_pos);
        close = true;
    }
    //if (ImGui::MenuItem("Insert angle")) // TODO. deny if dummy
    //{
    //    close = true;
    //}

    return close;
}

bool PanelWorkflow::show_multilink_menu()
{
    bool close = false;

    if (ImGui::MenuItem("Unlink all"))
    {
        m_action->multi_action_on("Unlink");
        for (auto link : m_menu_objs.links)
        {
            m_action->unlink(link);
        }
        m_action->multi_action_off();
        close = true;
    }
    return close;
}

bool PanelWorkflow::show_new_node_menu()
{
    auto pin_inst = *m_menu_objs.pins.begin();
    auto node_inst = *m_menu_objs.nodes.begin();

    m_fmt_buffer.clear();
    fmt::format_to(std::back_inserter(m_fmt_buffer), FMT_STRING("{} [{}]"), node_inst->get_name(), pin_inst->state().type_str);
    m_fmt_buffer.push_back('\0');

    ImGui::MenuItem(m_fmt_buffer.data(), nullptr, nullptr, false);
    ImGui::Separator();

    bool close = false;

    const bool input = pin_inst->get_object()->input;
    const auto input_filter = input ? std::list<PinType>{ PinType::_none } : node_inst->get_supported_pin_types(pin_inst);
    const auto output_filter = input ? node_inst->get_supported_pin_types(pin_inst) : std::list<PinType>{ PinType::_none };
    const auto node = show_nodes_menu(input_filter, output_filter);
    if (node != nullptr)
    {
        m_action->multi_action_on("Add node");
        close = true;
        bool linked = false;
        const auto* inst = m_action->add_node(node, "", ImRect(m_menu_objs.click_pos, ImVec2(-1, -1)));
        const auto& oppo = input ? inst->get_outputs() : inst->get_inputs();
        const auto& filter = input ? output_filter : input_filter;
        if (PinType::ANY == filter.front() && !oppo.empty())
        {
            if (pin_inst->state().links > 0 && pin_inst->get_object()->input)
                m_action->unlink(pin_inst);
            m_action->link(*oppo.begin(), pin_inst);
            linked = true;
        }
        if (!linked)
        {
            for (auto pin : oppo)
            {
                if (std::ranges::find_if(filter, [type = pin->state().type](PinType filter) { return filter == type; }) != filter.end())
                {
                    if (pin_inst->state().links > 0 && pin_inst->get_object()->input)
                        m_action->unlink(pin_inst);

                    m_action->link(pin, pin_inst);
                    linked = true;
                    break;
                }
            }
        }
        if (!linked)
        {
            for (auto pin : oppo)
            {
                if (pin->get_object()->type_any)
                {
                    if (pin_inst->state().links > 0 && pin_inst->get_object()->input)
                        m_action->unlink(pin_inst);

                    m_action->link(pin, pin_inst);
                    linked = true;
                    break;
                }
            }
        }
        m_action->multi_action_off();
    }
    return close;
}

////////////////////////////////

bool PanelWorkflow::show_colors_menu(bool& close, bool& reset, bool show_alpha, const ImVec4& init)
{
    const std::pair<ImU32, const char*> colors[] =
    {
        { IM_COL32(89, 27, 27, 200), "Red" },
        { IM_COL32(7, 53, 7, 169), "Green" },
        { IM_COL32(19, 19, 144, 169), "Blue" },
        { IM_COL32(30, 47, 64, 212), "Pale blue" },
        { IM_COL32(7, 63, 63, 181), "Cyan" },
        { IM_COL32(67, 26, 106, 153), "Purple" },
        { IM_COL32(61, 61, 5, 199), "Yellow" },
        { IM_COL32(103, 49, 9, 161), "Brown" },
        { IM_COL32(0, 0, 0, 194), "Black" },
    };

    reset = close = false;
    bool ret = false;

    if (ImGui::MenuItem("Reset to default"))
    {
        reset = close = true;
        return true;
    }
    ImGui::Separator();

    for (auto& [clr, name] : colors)
    {
        ImGui::PushID(clr);
        const auto v4 = ImGui::ColorConvertU32ToFloat4(clr);
        const bool sel = ImGui::ColorButton("", v4, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder);
        ImGui::SameLine();
        if (ImGui::MenuItem(name) || sel)
        {
            m_color_buff[0] = v4.x; m_color_buff[1] = v4.y; m_color_buff[2] = v4.z; m_color_buff[3] = v4.w;
            ret = true;
        }
        ImGui::PopID();
    }

    ImGui::Spacing();
    if (m_color_buff[0] < 0.0f)
    {
        m_color_buff[0] = init.x; m_color_buff[1] = init.y; m_color_buff[2] = init.z; m_color_buff[3] = init.w;
    }
    if (ImGui::ColorPicker4("###Background color", m_color_buff, (show_alpha ? ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview : 0) | ImGuiColorEditFlags_DefaultOptions_))
    {
        ret = true;
    }

    return ret;
}

data::Node* PanelWorkflow::show_nodes_menu(const std::list<PinType>& in_pins, const std::list<PinType>& out_pins)
{
    if (m_context_menu_nodes_need_focus)
    {
        ImGui::SetKeyboardFocusHere();
        m_context_menu_nodes_need_focus = false;

        g::pNodesManager->calc_number_inside(tools::str::trim_their(m_context_menu_nodes_filter), in_pins, out_pins);
    }
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGuiEx::InputTextWithHint("##nodes_filter", "filter nodes", &m_context_menu_nodes_filter,
        ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoUndoRedo | ImGuiInputTextFlags_EscapeClearsAll))
    {
        g::pNodesManager->calc_number_inside(tools::str::trim_their(m_context_menu_nodes_filter), in_pins, out_pins);
    }

    auto& nodes = g::pNodesManager->get_nodes_tree();

    data::Node* ret_node = nullptr;

    auto& labels = g::pNodesManager->get_labels();
    bool labels_shown = false;

    for (auto& label : labels)
    {
        size_t nodes_count = 0;
        auto& nodes_in_label = g::pNodesManager->get_nodes_in_label(label.second);
        for (auto node_item : nodes_in_label)
        {
            if (!node_item->filtered)
                nodes_count++;
        }

        if (nodes_count > 0)
        {
            labels_shown = true;

            m_fmt_buffer.clear();
            fmt::format_to(std::back_inserter(m_fmt_buffer), FMT_STRING("{} [{}]"), label.first, nodes_count);
            m_fmt_buffer.push_back('\0');

            if (ImGui::BeginMenu(m_fmt_buffer.data()))
            {
                for (auto node_item : nodes_in_label)
                {
                    if (!node_item->filtered)
                    {
                        if (ImGui::MenuItem(node_item->name_or_category.c_str()))
                        {
                            ret_node = node_item->node;
                        }
                    }
                }
                ImGui::EndMenu();
            }
        }
    }

    if (labels_shown)
        ImGui::Separator();

    std::function<void(const std::vector<data::NodeItem*>& tree)> nodes_menu;
    nodes_menu = [&nodes_menu, &ret_node, &mem_buff = m_fmt_buffer](const std::vector<data::NodeItem*>& nodes)
    {
        for (auto node_item : nodes)
        {
            if (node_item->filtered || (!node_item->is_node && 0 == node_item->nodes_inside))
                continue;

            if (!node_item->is_node)
            {
                mem_buff.clear();
                fmt::format_to(std::back_inserter(mem_buff), FMT_STRING("{} [{}]"), node_item->name_or_category, node_item->nodes_inside);
                mem_buff.push_back('\0');

                if (ImGui::BeginMenu(mem_buff.data()))
                {
                    nodes_menu(node_item->children);
                    ImGui::EndMenu();
                }
            }
            else
            {
                if (ImGui::MenuItem(node_item->name_or_category.c_str()))
                    ret_node = node_item->node;
            }
        }
    };

    nodes_menu(nodes);
    return ret_node;
}

} // namespace ui
