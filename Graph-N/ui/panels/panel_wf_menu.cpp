
#include "ui/panels/panel_wf.h"
#include "core/workflow/node_instance.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/nodes_manager.h"
#include "ui/graph-n_flow.h"

namespace ui
{

bool PanelWorkflow::context_menu_handler()
{
    if (m_drawer.get_bg_menu_active())
    {
        m_handle_context_menu = true;
        m_menu_objs = {};
        m_context_menu_type = MenuType::Back;
        m_menu_objs.click_pos = m_drawer.get_mouse_click_pos();
        Log("Bg menu initiated");
        return true;
    }

    if (!ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        return false;

    const auto pin = m_drawer.get_hovered_pin();
    if (pin != nullptr)
    {
        m_handle_context_menu = true;
        m_menu_objs = {};
        m_context_menu_type = MenuType::Pin;
        m_menu_objs.pins.insert(pin);
        m_menu_objs.click_pos = m_drawer.get_mouse_click_pos();
        Log("Pin menu initiated");
        return true;
    }

    const auto link = m_drawer.get_hovered_link();
    if (link != nullptr)
    {
        m_handle_context_menu = true;
        m_menu_objs = {};
        m_menu_objs.click_pos = m_drawer.get_mouse_click_pos();

        const auto sel_links = m_drawer.get_selected_links();
        if (sel_links.size() == 1)
        {
            m_context_menu_type = MenuType::Link;
            m_menu_objs.links.insert(link);
            Log("Link menu initiated");
        }
        else
        {
            m_context_menu_type = MenuType::Multilink;
            m_menu_objs.links = std::move(sel_links);
            Log("Multilink menu initiated");
        }
        return true;
    }

    const auto node = m_drawer.get_hovered_node();
    if (node != nullptr)
    {
        m_handle_context_menu = true;
        m_menu_objs = {};
        m_menu_objs.click_pos = m_drawer.get_mouse_click_pos();

        auto nodes = m_drawer.get_selected_nodes();
        if (!m_drawer.is_node_selected(node->ed()) || 1 == nodes.size())
        {
            m_drawer.select_node(node->ed(), false);
            if (node->m_group)
                m_menu_objs.groups.insert({ node, m_drawer.get_nodes_in_group(node, m_nodes) });
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
                    m_menu_objs.groups.insert({ node_it, m_drawer.get_nodes_in_group(node_it, m_nodes) });
                }
                else
                {
                    m_menu_objs.nodes.insert(node_it);
                }
            }

            if (!m_menu_objs.groups.empty() && m_menu_objs.nodes.empty())
            {
                m_context_menu_type = MenuType::Multigroup;
                Log("Multigroup menu initiated");
            }
            else if (m_menu_objs.groups.empty() && !m_menu_objs.nodes.empty())
            {
                m_context_menu_type = MenuType::Multinode;
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
    }

    if (ImGui::BeginPopup(m_context_menuid.c_str()))
    {
        bool close = false;
        switch (m_context_menu_type)
        {
        case MenuType::Back:
            close = show_canvas_menu();
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
        default:
            {
                LogInternalLogic("Unhandled menu type");
            }
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
    
    ImGui::MenuItem(g::pWfsManager->get_data(m_wf).name.c_str(), nullptr, nullptr, false);
    ImGui::Separator();
    if (ImGui::BeginMenu("Add node##allnodes"))
    {
        auto node = show_nodes_menu("", "");
        if (node != nullptr)
        {
            close = true;
            add_node(node, "", ImRect(m_menu_objs.click_pos, ImVec2(-1, -1)));
        }
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Add group"))
    {
        create_group(ImRect(m_menu_objs.click_pos, ImVec2(-1, -1)));
        close = true;
    }
    if (ImGui::MenuItem("Fit all"))
    {
        fit(m_menu_objs.groups);
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Undo")) // TODO
    {
        close = true;
    }
    if (ImGui::MenuItem("Redo")) // TODO
    {
        close = true;
    }
    ImGui::Separator();
    //if (ImGui::MenuItem("Paste", nullptr, false, !m_copied_objs.nodes.empty() || !m_copied_objs.groups.empty())) // TODO
    //{
    //    close = true;
    //}
    if (ImGui::MenuItem("Select all"))
    {
        m_drawer.select_node({}, false);
        for (auto node : m_nodes)
        {
            m_drawer.select_node(node->ed(), true);
        }
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Duplicate workflow"))
    {
        g::pFlow->wf_create_copy_from(m_wf);
        close = true;
    }
    if (ImGui::MenuItem("Reset workflow window position")) // TODO
    {
        restore_pos();
        close = true;
    }
    return close;
}

bool PanelWorkflow::show_node_menu()
{
    bool close = false;
    auto node = *m_menu_objs.nodes.begin();

    if (ImGui::MenuItem("Rename"))
    {
        m_drawer.rename_node(node);
        close = true;
    }
    if (ImGui::MenuItem("Fit"))
    {
        node->reset_width();
        close = true;
    }
    if (ImGui::MenuItem(node->pinned() ? "Unpin" : "Pin"))
    {
        node->toggle_pinned();
        m_drawer.pin_node(node->ed(), node->pinned());
        close = true;
    }
    if (ImGui::MenuItem("Group"))
    {
        create_group(m_drawer.calc_group_rect(node->m_rc));
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Always", nullptr, node->get_mode() == NodeMode::Always))
    {
        node->set_mode(NodeMode::Always);
        close = true;
    }
    if (ImGui::MenuItem("Bypass", nullptr, node->get_mode() == NodeMode::Bypass))
    {
        node->set_mode(NodeMode::Bypass);
        close = true;
    }
    if (ImGui::MenuItem("Mute", nullptr, node->get_mode() == NodeMode::Mute || node->get_mode() == NodeMode::OnTrigger || node->get_mode() == NodeMode::OnEvent))
    {
        node->set_mode(NodeMode::Mute);
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Remove"))
    {
        m_drawer.delete_node(node->ed());
        node->ed().reset();
        delete_node(node);
        close = true;
    }
    //if (ImGui::MenuItem("Copy"))
    //{
    //    m_copied_objs = {};
    //    m_copied_objs.nodes.insert(node);
    //    close = true;
    //}
    ImGui::Separator();

    //if (ImGui::MenuItem(node->expanded() ? "Collapse" : "Expand")) // TODO: FIX
    //{
    //    node->toggle_expanded();
    //    close = true;
    //}
    ////ImGui::Separator();
    //if (ImGui::MenuItem("Convert to subgraph")) // TODO
    //{
    //    close = true;
    //}
    //ImGui::Separator();

    if (ImGui::BeginMenu("Color"))
    {
        bool reset = false;
        if (show_colors_menu(close, reset, node->m_clr_custom, true, node->m_clr_node))
        {
            if (reset)
            {
                node->reset_to_default_colors();
            }
            else
            {
                node->m_clr_node.x = m_color_buff[0]; node->m_clr_node.y = m_color_buff[1]; node->m_clr_node.z = m_color_buff[2]; node->m_clr_node.w = m_color_buff[3];
                node->set_node_color(ImVec4(m_color_buff[0], m_color_buff[1], m_color_buff[2], m_color_buff[3]));
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
            m_drawer.fit_group_rect(group, nodes);
            close = true;
        }
        if (ImGui::MenuItem("Select nodes"))
        {
            m_drawer.select_node({}, false);
            for (auto node : nodes)
            {
                m_drawer.select_node(node->ed(), true);
            }
        }
    }
    ImGui::Separator();
    if (ImGui::MenuItem(nodes.empty() ? "Remove" : "Remove group"))
    {
        m_drawer.delete_node(group->ed());
        group->ed().reset();
        delete_group(group);
        close = true;
    }
    if (!nodes.empty())
    {
        m_fmt_buffer.clear();
        fmt::format_to(std::back_inserter(m_fmt_buffer), "Remove group and {} node(s) inside", nodes.size());
        *(m_fmt_buffer.data() + m_fmt_buffer.size()) = 0;

        if (ImGui::MenuItem(m_fmt_buffer.data()))
        {
            for (auto node : nodes)
            {
                m_drawer.delete_node(node->ed());
                node->ed().reset();
                delete_node(node);
            }

            m_drawer.delete_node(group->ed());
            group->ed().reset();
            delete_group(group);

            close = true;
        }
    }
    //if (ImGui::MenuItem("Copy"))
    //{
    //    m_copied_objs = m_menu_objs;
    //    close = true;
    //}
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
        if (show_colors_menu(close, reset, true, for_all, ImGui::ColorConvertU32ToFloat4(group->m_clr_group)))
        {
            if (reset)
            {
                group->reset_to_default_colors();
                if (for_all)
                {
                    for (auto node : nodes)
                    {
                        node->reset_to_default_colors();
                    }
                }
            }
            else
            {
                group->set_group_color(ImVec4(m_color_buff[0], m_color_buff[1], m_color_buff[2], m_color_buff[3]));
                if (for_all)
                {
                    for (auto node : nodes)
                    {
                        node->set_node_color(ImVec4(m_color_buff[0], m_color_buff[1], m_color_buff[2], m_color_buff[3]));
                    }
                }
            }
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
    if (ImGui::MenuItem("Fit"))
    {
        for (auto node : m_menu_objs.nodes)
        {
            node->reset_width();
        }
        close = true;
    }
    if (ImGui::MenuItem("Group"))
    {
        auto group = add_group(std::nullopt);
        m_drawer.fit_group_rect(group, m_menu_objs.nodes);
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Remove"))
    {
        for (auto node : m_menu_objs.nodes)
        {
            m_drawer.delete_node(node->ed());
            node->ed().reset();
            delete_node(node);
        }
        close = true;
    }
    if (ImGui::MenuItem("Copy")) // TODO
    {
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Pin"))
    {
        for (auto node : m_menu_objs.nodes)
        {
            node->pin(true);
            m_drawer.pin_node(node->ed(), node->pinned());
        }
        close = true;
    }
    if (ImGui::MenuItem("Unpin"))
    {
        for (auto node : m_menu_objs.nodes)
        {
            node->pin(false);
            m_drawer.pin_node(node->ed(), node->pinned());
        }
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Always"))
    {
        for (auto node : m_menu_objs.nodes)
        {
            node->set_mode(NodeMode::Always);
        }
        close = true;
    }
    if (ImGui::MenuItem("Bypass"))
    {
        for (auto node : m_menu_objs.nodes)
        {
            node->set_mode(NodeMode::Bypass);
        }
        close = true;
    }
    if (ImGui::MenuItem("Mute"))
    {
        for (auto node : m_menu_objs.nodes)
        {
            node->set_mode(NodeMode::Mute);
        }
        close = true;
    }
    ImGui::Separator();
    if (ImGui::BeginMenu("Color"))
    {
        bool reset = false;
        if (show_colors_menu(close, reset, true, true, (*m_menu_objs.nodes.begin())->m_clr_node))
        {
            for (auto node : m_menu_objs.nodes)
            {
                if (reset)
                    node->reset_to_default_colors();
                else
                {
                    node->m_clr_node.x = m_color_buff[0]; node->m_clr_node.y = m_color_buff[1]; node->m_clr_node.z = m_color_buff[2]; node->m_clr_node.w = m_color_buff[3];
                    node->set_node_color(ImVec4(m_color_buff[0], m_color_buff[1], m_color_buff[2], m_color_buff[3]));
                }
            }
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
    if (ImGui::MenuItem(nodes_inside > 0 ? "Remove groups" : "Remove"))
    {
        for (const auto& pair : m_menu_objs.groups)
        {
            m_drawer.delete_node(pair.first->ed());
            pair.first->ed().reset();
            delete_group(pair.first);
        }
        close = true;
    }
    if (nodes_inside > 0)
    {
        if (ImGui::MenuItem("Remove groups and nodes inside"))
        {
            for (const auto&[group, nodes] : m_menu_objs.groups)
            {
                m_drawer.delete_node(group->ed());
                group->ed().reset();
                delete_group(group);

                for (auto node : nodes)
                {
                    m_drawer.delete_node(node->ed());
                    node->ed().reset();
                    delete_node(node);
                }
            }
            close = true;
        }
    }
    if (ImGui::MenuItem("Copy")) // TODO
    {
        close = true;
    }
    ImGui::Separator();
    if (nodes_inside > 0)
    {
        if (ImGui::MenuItem("Fit"))
        {
            for (const auto&[group, nodes] : m_menu_objs.groups)
            {
                m_drawer.fit_group_rect(group, nodes);
            }
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
        if (show_colors_menu(close, reset, true, for_all, ImGui::ColorConvertU32ToFloat4(m_menu_objs.groups.begin()->first->m_clr_group)))
        {
            for (const auto& [group, nodes] : m_menu_objs.groups)
            {
                if (reset)
                    group->reset_to_default_colors();
                else
                    group->set_group_color(ImVec4(m_color_buff[0], m_color_buff[1], m_color_buff[2], m_color_buff[3]));

                if (for_all)
                {
                    for (auto node : nodes)
                    {
                        if (reset)
                            node->reset_to_default_colors();
                        else
                            node->set_node_color(ImVec4(m_color_buff[0], m_color_buff[1], m_color_buff[2], m_color_buff[3]));
                    }
                }
            }
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
    if (ImGui::MenuItem("Remove all")) // TODO
    {
        close = true;
    }
    if (ImGui::MenuItem("Copy")) // TODO
    {
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Fit groups to content")) // TODO
    {
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Color")) // TODO
    {
        close = true;
    }

    return close;
}

bool PanelWorkflow::show_pin_menu()
{
    bool close = false;

    if (ImGui::MenuItem("Unlink")) // TODO
    {
        close = true;
    }
    if (ImGui::MenuItem("Rename slot")) // TODO
    {
        close = true;
    }

    return close;
}

bool PanelWorkflow::show_link_menu()
{
    bool close = false;

    if (ImGui::MenuItem("Remove link")) // TODO
    {
        close = true;
    }
    if (ImGui::MenuItem("Insert angle")) // TODO
    {
        close = true;
    }

    return close;
}

bool PanelWorkflow::show_multilink_menu()
{
    bool close = false;

    if (ImGui::MenuItem("Remove all links"))
    {
        close = true;
    }
    return close;
}

////////////////////////////////

bool PanelWorkflow::show_colors_menu(bool& close, bool& reset, bool use_custom, bool show_alpha, const ImVec4& init)
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

    if (ImGui::MenuItem("Reset to default", nullptr, nullptr, use_custom))
    {
        reset = close = true;
        return true;
    }
    ImGui::Separator();

    for (auto& [clr, name] : colors)
    {
        ImGui::PushID(clr);
        auto v4 = ImGui::ColorConvertU32ToFloat4(clr);
        bool sel = ImGui::ColorButton("", v4, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder);
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
        if (use_custom)
        {
            m_color_buff[0] = init.x; m_color_buff[1] = init.y; m_color_buff[2] = init.z; m_color_buff[3] = init.w;
        }
        else
        {
            auto vec = ImGui::ColorConvertU32ToFloat4(m_drawer.get_default_node_color());
            m_color_buff[0] = vec.x; m_color_buff[1] = vec.y; m_color_buff[2] = vec.z; m_color_buff[3] = vec.w;
        }
    }
    if (ImGui::ColorPicker4("###Background color", m_color_buff, (show_alpha ? ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview : 0) | ImGuiColorEditFlags_DefaultOptions_))
    {
        ret = true;
    }

    return ret;
}

data::Node* PanelWorkflow::show_nodes_menu(const std::string& in_pin_filter, const std::string& out_pin_filter)
{
    if (m_context_menu_nodes_need_focus)
    {
        ImGui::SetKeyboardFocusHere();
        m_context_menu_nodes_need_focus = false;
    }
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGuiEx::InputTextWithHint("##nodes_filter", "filter nodes", &m_context_menu_nodes_filter,
        ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoUndoRedo | ImGuiInputTextFlags_EscapeClearsAll))
    {
        g::pNodesManager->calc_number_inside(tools::str::trim_their(m_context_menu_nodes_filter), in_pin_filter, out_pin_filter);
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
            fmt::format_to(std::back_inserter(m_fmt_buffer), "{} [{}]", label.first, nodes_count);
            *(m_fmt_buffer.data() + m_fmt_buffer.size()) = 0;

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
                fmt::format_to(std::back_inserter(mem_buff), "{} [{}]", node_item->name_or_category, node_item->nodes_inside);
                *(mem_buff.data() + mem_buff.size()) = 0;

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
