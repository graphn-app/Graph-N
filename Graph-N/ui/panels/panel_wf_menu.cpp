
#include "ui/panels/panel_wf.h"
#include "core/workflow/node_instance.h"

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
        }
        else
        {
            m_context_menu_type = MenuType::Multilink;
            m_menu_objs.links = std::move(sel_links);
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
        if (!m_drawer.is_node_selected(node->get_ed()) || 1 == nodes.size())
        {
            m_drawer.select_node(node->get_ed(), false);
            if (node->m_group)
                m_menu_objs.groups.insert({ node, m_drawer.get_nodes_in_group(node, m_nodes) });
            else
                m_menu_objs.nodes.insert(node);
            m_context_menu_type = node->m_group ? MenuType::Group : MenuType::Node;
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
                m_context_menu_type = MenuType::Multigroup;
            else if (m_menu_objs.groups.empty() && !m_menu_objs.nodes.empty())
                m_context_menu_type = MenuType::Multinode;
            else
                m_context_menu_type = MenuType::Multiobj;
        }
        return true;
    }

    return true;
}

bool PanelWorkflow::show_context_menu()
{
    if (m_handle_context_menu)
    {
        ImGui::OpenPopup(m_context_menuid);
        m_handle_context_menu = false;
    }

    if (ImGui::BeginPopup(m_context_menuid))
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

    ImGui::MenuItem("Project name", nullptr, nullptr, false); // TODO
    ImGui::Separator();
    if (ImGui::MenuItem("Add node")) // TODO
    {
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
    if (ImGui::MenuItem("Add group"))
    {
        create_group(ImRect(m_menu_objs.click_pos.x, m_menu_objs.click_pos.y, -1, -1));
        close = true;
    }
    if (ImGui::MenuItem("Fit all groups to content")) // TODO
    {
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Paste", nullptr, false, !m_copied_objs.nodes.empty() || !m_copied_objs.groups.empty())) // TODO
    {
        close = true;
    }
    if (ImGui::MenuItem("Select all")) // TODO
    {
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Duplicate workflow")) // TODO
    {
        close = true;
    }
    if (ImGui::MenuItem("Reset workflow window position")) // TODO
    {
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
    if (ImGui::MenuItem("Reset width"))
    {
        node->reset_width();
        close = true;
    }
    if (ImGui::MenuItem(node->pinned() ? "Unpin" : "Pin"))
    {
        node->toggle_pinned();
        m_drawer.pin_node(node->get_ed(), node->pinned());
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
        m_drawer.delete_node(node->get_ed());
        node->reset_ed();
        delete_node(node);
        close = true;
    }
    if (ImGui::MenuItem("Copy"))
    {
        m_copied_objs = {};
        m_copied_objs.nodes.insert(node);
        close = true;
    }
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
            if (!nodes.empty())
            {
                group->m_rc = ImRect(FLT_MAX, FLT_MAX, -FLT_MIN, -FLT_MIN);
                for (auto node : nodes)
                {
                    group->m_rc.Min.x = std::min(group->m_rc.Min.x, node->m_rc.Min.x);
                    group->m_rc.Min.y = std::min(group->m_rc.Min.y, node->m_rc.Min.y);
                    group->m_rc.Max.x = std::max(group->m_rc.Max.x, node->m_rc.Min.x + node->m_rc.Max.x);
                    group->m_rc.Max.y = std::max(group->m_rc.Max.y, node->m_rc.Min.y + node->m_rc.Max.y);
                }
                group->m_rc = m_drawer.calc_group_rect(group->m_rc);
                group->m_pos_set = group->m_size_set = false;
            }
            close = true;
        }
        if (ImGui::MenuItem("Select nodes"))
        {
            m_drawer.select_node(nullptr, false);
            for (auto node : nodes)
            {
                m_drawer.select_node(node->get_ed(), true);
            }
        }
    }
    ImGui::Separator();
    if (ImGui::MenuItem(nodes.empty() ? "Remove" : "Remove group"))
    {
        m_drawer.delete_node(group->get_ed());
        group->reset_ed();
        delete_group(group);
        close = true;
    }
    if (!nodes.empty())
    {
        if (ImGui::MenuItem(fmt::format("Remove group and {} node(s) inside", nodes.size()).c_str()))
        {
            for (auto node : nodes)
            {
                m_drawer.delete_node(node->get_ed());
                node->reset_ed();
                delete_node(node);
            }

            m_drawer.delete_node(group->get_ed());
            group->reset_ed();
            delete_group(group);

            close = true;
        }
    }
    if (ImGui::MenuItem("Copy"))
    {
        m_copied_objs = m_menu_objs;
        close = true;
    }
    ImGui::Separator();
    if (!nodes.empty())
    {
        if (ImGui::MenuItem("Pin nodes"))
        {
            for (auto node : nodes)
            {
                node->pin(true);
                m_drawer.pin_node(node->get_ed(), true);
            }
            close = true;
        }
        if (ImGui::MenuItem("Unpin nodes"))
        {
            for (auto node : nodes)
            {
                node->pin(false);
                m_drawer.pin_node(node->get_ed(), false);
            }
            close = true;
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Always"))
        {
            for (auto node : nodes)
            {
                node->set_mode(NodeMode::Always);
            }
            close = true;
        }
        if (ImGui::MenuItem("Bypass"))
        {
            for (auto node : nodes)
            {
                node->set_mode(NodeMode::Bypass);
            }
            close = true;
        }
        if (ImGui::MenuItem("Mute"))
        {
            for (auto node : nodes)
            {
                node->set_mode(NodeMode::Mute);
            }
            close = true;
        }
    }
    ImGui::Separator();
    if (ImGui::BeginMenu("Color"))
    {
        static bool for_all = false;
        if (!nodes.empty())
        {
            ImGui::Checkbox("Apply for nodes in group", &for_all);
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
    if (ImGui::MenuItem("Reset nodes width")) // TODO
    {
        close = true;
    }
    if (ImGui::MenuItem("Group")) // TODO
    {
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Remove {} nodes")) // TODO
    {
        close = true;
    }
    if (ImGui::MenuItem("Copy")) // TODO
    {
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Pin nodes")) // TODO
    {
        close = true;
    }
    if (ImGui::MenuItem("Unpin nodes")) // TODO
    {
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Always")) // TODO
    {
        close = true;
    }
    if (ImGui::MenuItem("Bypass")) // TODO
    {
        close = true;
    }
    if (ImGui::MenuItem("Mute")) // TODO
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

bool PanelWorkflow::show_multigroup_menu()
{
    bool close = false;
    if (ImGui::MenuItem("Remove groups")) // TODO
    {
        close = true;
    }
    if (ImGui::MenuItem("Remove groups and {} nodes inside")) // TODO
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


} // namespace ui
