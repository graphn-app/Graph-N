#include "ui/panels/panel_wf.h"
#include "ui/custom_widgets/custom_widget.h"
#include "ui/graph-n_flow.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/pin_instance.h"
#include "core/workflow/link.h"
#include "core/workflow/workflow_action.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/history_manager.h"
#include "core/managers/hotkeys_manager.h"
#include "core/managers/host_manager.h"

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
        const auto pin = m_drawer.get_hovered_widget();
        if (pin != nullptr &&
            (PinType::INT == pin->state().type ||
            PinType::FLOAT == pin->state().type ||
            PinType::NUMBER == pin->state().type ||
            PinType::COMBO == pin->state().type ||
            PinType::STRING == pin->state().type ||
            PinType::COLOR == pin->state().type))
        {
            const auto parent = m_wf->get_node_by_instance(pin->get_parent_node());
            
            m_handle_context_menu = true;
            m_menu_objs = {};
            m_context_menu_type = MenuType::Widget;
            m_menu_objs.pins.insert(pin);
            m_menu_objs.nodes.insert(parent);
            m_menu_objs.click_pos = m_drawer.get_mouse_pos();
            Log("Widget menu initiated");
            return true;
        }
    }

    {
        const auto pin = m_drawer.get_hovered_pin();
        if (pin != nullptr)
        {
            const auto parent = m_wf->get_node_by_instance(pin->get_parent_node());
            if (parent->flags().has_not(NodeInstFlags::Collapsed) && parent->obj_flags().has_not(NodeObjFlags::Dummy))
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

                if (node->flags().has(NodeInstFlags::Group))
                    m_menu_objs.groups.insert({ node, m_drawer.get_nodes_in_group(node, m_wf->get_nodes_all()) });
                else
                    m_menu_objs.nodes.insert(node);
                m_context_menu_type = node->flags().has(NodeInstFlags::Group) ? MenuType::Group : MenuType::Node;
                Log((node->flags().has(NodeInstFlags::Group) ? "Group" : "Node") << " menu initialized");
            }
            else
            {
                for (auto node_it : nodes)
                {
                    if (node_it->flags().has(NodeInstFlags::Group))
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
        ImGui::OpenPopup(m_context_menuid);
        m_handle_context_menu = false;
        m_context_menu_nodes_need_focus = true;
        m_context_menu_nodes_filter.clear();
        m_context_menu_nodes_filter.shrink_to_fit();
    }

    if (m_menu_objs.align_menu_pos)
        ImGui::SetNextWindowPos(m_menu_objs.click_pos, ImGuiCond_Appearing);

    if (ImGui::BeginPopup(m_context_menuid))
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
        case MenuType::Widget:
            close = show_widget_menu();
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
        m_drawer.clear_selection();
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

    if (node->flags().has_any(NodeInstFlags::MaskEditorReady, NodeInstFlags::PosEditorReady))
    {
        if (ImGui::MenuItem("Open in Mask Editor | SAM Detector", nullptr, nullptr, node->flags().has_and_has_not(NodeInstFlags::MaskEditorReady, NodeInstFlags::PosEditorRunning)))
        {
            g::pFlow->host_open_mask_editor(get_workflow()->get_id(), node->get_instance_id());
            close = true;
        }
        if (ImGui::MenuItem("Open in 3D Pos Editor", nullptr, nullptr, node->flags().has_and_has_not(NodeInstFlags::MaskEditorReady, NodeInstFlags::MaskEditorRunning)))
        {
            g::pFlow->host_open_3dpos_editor(get_workflow()->get_id(), node->get_instance_id());
            close = true;
        }
        ImGui::Separator();
    }
    else if (node->obj_flags().has_all(NodeObjFlags::CustomWidget, NodeObjFlags::CustomWdgtFromOutput, NodeObjFlags::CustomWdgtText))
    {
        if (ImGui::MenuItem("Copy output text"))
        {
            g::pFlow->custom_widgets().copy_text_output(node->get_workflow_id(), node->get_instance_id());
            close = true;
        }
        if (ImGui::MenuItem("View output text in full-text mode", "Dbl-Click"))
        {
            g::pFlow->custom_widgets().view_text_output(node->get_workflow_id(), node->get_instance_id());
            close = true;
        }
        ImGui::Separator();
    }

    if (node->obj_flags().has(NodeObjFlags::CustomWidget))
    { 
        if (ImGui::MenuItem("Open in default app", nullptr, nullptr, g::pFlow->custom_widgets().check_open_def_app(node, true)))
        {
            g::pFlow->custom_widgets().check_open_def_app(node, false);
            close = true;
        }
        ImGui::Separator();
    }

    if (ImGui::MenuItem("Rename", nullptr, nullptr, m_drawer.zoomed_allow_renaming() && node->obj_flags().has_and_has_not(NodeObjFlags::CanRename, NodeObjFlags::Dummy)))
    {
        m_drawer.rename_node(node);
        close = true;
    }
    if (node->obj_flags().has_not(NodeObjFlags::Dummy) && node->get_name() != node->get_name_original())
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
    if (ImGui::MenuItem(node->flags().has(NodeInstFlags::Pinned) ? "Unpin" : "Pin", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Pin)))
    {
        m_action->set_node_pinning(node, node->flags().has_not(NodeInstFlags::Pinned));
        close = true;
    }
    if (ImGui::MenuItem(node->flags().has(NodeInstFlags::Collapsed) ? "Expand" : "Collapse", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Collapse)))
    {
        m_action->set_node_collapsed(node, node->flags().has_not(NodeInstFlags::Collapsed));
        close = true;
    }
    if (ImGui::MenuItem("Group", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Group)))
    {
        group_nodes({ node });
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Always", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Always), node->get_mode_always(), node->obj_flags().has_not(NodeObjFlags::Dummy)))
    {
        m_action->set_node_mode(node, NodeInstFlags::None);
        close = true;
    }
    if (ImGui::MenuItem("Bypass", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Bypass), node->get_mode_bypass(), node->obj_flags().has_and_has_not(NodeObjFlags::CanBypass, NodeObjFlags::Dummy)))
    {
        m_action->set_node_mode(node, NodeInstFlags::ModeBypass);
        close = true;
    }
    if (ImGui::MenuItem("Mute", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Mute), node->get_mode_mute(), node->obj_flags().has_and_has_not(NodeObjFlags::CanMute, NodeObjFlags::Dummy)))
    {
        m_action->set_node_mode(node, NodeInstFlags::ModeMute);
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Remove", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Remove)))
    {
        delete_nodes({ node }, false);
        close = true;
        create_toolbar();
        return close;
    }
    if (ImGui::MenuItem("Copy", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Copy), nullptr, node->obj_flags().has_not(NodeObjFlags::Dummy)))
    {
        copy_nodes({ node }, {});
        close = true;
    }
    ImGui::Separator();

    if (node->flags().has(NodeInstFlags::Reroute))
    {
        if (ImGui::MenuItem("Separate Reroute", nullptr, nullptr, node->obj_flags().has_not(NodeObjFlags::Dummy) && node->get_inputs().size() > 2))
        {
            separate_reroutes(m_menu_objs.nodes);
            close = true;
        }
        if (node->flags().has(NodeInstFlags::RerouteSplitted))
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


    data::Pin* preffered_source_pin = nullptr;
    bool restricted = node->obj_flags().has(NodeObjFlags::CustomWdgtRestrictCfg);
    if (!restricted)
    {
        if (node->get_object()->custom_widget_source != nullptr)
        {
            preffered_source_pin = node->get_object()->custom_widget_source;
        }
        else
        {
            bool has_combos = false;
            for (auto pin : node->get_object()->input)
            {
                if (PinType::COMBO == pin->def_state.type)
                {
                    has_combos = true;
                    if (preffered_source_pin != nullptr)
                    {
                        preffered_source_pin = nullptr;
                        break;
                    }
                    preffered_source_pin = pin;
                }
            }
            restricted = !has_combos;
        }
    }

    if (!restricted)
    {
        ImGui::Separator();
        if (ImGui::BeginMenu("Node config"))
        {
            ImGui::PushItemFlag(ImGuiItemFlags_SelectableDontClosePopup, true);

            const auto save_opts = [node]()
            {
                mngr::NodeOptions opts;
                opts.flags.copy_from_by_mask_obj(node->obj_flags(), opts.mask);
                opts.display_source = (node->get_object()->custom_widget_source != nullptr ? node->get_object()->custom_widget_source->def_state.name : "");
                g::pOptsManager->items_node_save(node->get_object()->get_hash(), opts);

                const bool has_media_opt = node->obj_flags().has_any_by_mask_obj(g_node_obj_all_custom_widget_mask);
                node->obj_flags().add_remove_if(NodeObjFlags::CustomWidget, has_media_opt);
            };

            struct CustomWidgetCfgMenu final
            {
                const char* menu_name = nullptr;
                NodeObjFlags media_flag = NodeObjFlags::None;
            };
            constexpr CustomWidgetCfgMenu menu_cfg[3] =
            {
                { "Add widget: Image", NodeObjFlags::CustomWdgtImage },
                { "Add widget: Video", NodeObjFlags::CustomWdgtVideo },
                { "Add widget: Audio", NodeObjFlags::CustomWdgtAudio }
            };

            for (const auto& cfg : menu_cfg)
            {
                if (ImGui::BeginMenu(cfg.menu_name))
                {
                    bool display = node->obj_flags().has_all(NodeObjFlags::CustomWdgtFromSource, cfg.media_flag);
                    if (ImGui::MenuItem("Display media", nullptr, &display))
                    {
                        node->obj_flags().remove(NodeObjFlags::CustomWdgtAudio, NodeObjFlags::CustomWdgtVideo, NodeObjFlags::CustomWdgtImage);
                        const bool media_flag = node->obj_flags().has(NodeObjFlags::CustomWdgtUploadButton) || display;
                        node->obj_flags().add_remove_if(cfg.media_flag, media_flag);
                        node->obj_flags().add_remove_if(NodeObjFlags::CustomWdgtFromSource, display);
                        node->get_object()->custom_widget_source = preffered_source_pin;
                        save_opts();
                    }

                    bool can_upload = node->obj_flags().has_all(NodeObjFlags::CustomWdgtUploadButton, cfg.media_flag);
                    if (ImGui::MenuItem("Add 'Upload' button", nullptr, &can_upload))
                    {
                        node->obj_flags().remove(NodeObjFlags::CustomWdgtAudio, NodeObjFlags::CustomWdgtVideo, NodeObjFlags::CustomWdgtImage);
                        const bool media_flag = node->obj_flags().has(NodeObjFlags::CustomWdgtFromSource) || can_upload;
                        node->obj_flags().add_remove_if(cfg.media_flag, media_flag);
                        node->obj_flags().add_remove_if(NodeObjFlags::CustomWdgtUploadButton, can_upload);
                        node->get_object()->custom_widget_source = preffered_source_pin;
                        save_opts();
                    }

                    ImGui::Separator();
                    ImGui::MenuItem("Select media source:", nullptr, nullptr, false);

                    const std::string* display_source = (nullptr == node->get_object()->custom_widget_source ? nullptr : &node->get_object()->custom_widget_source->def_state.name);
                    if (nullptr == display_source && preffered_source_pin != nullptr)
                        display_source = &preffered_source_pin->def_state.name;
                    if (node->obj_flags().has_none(NodeObjFlags::CustomWdgtFromSource, NodeObjFlags::CustomWdgtUploadButton) || node->obj_flags().has_not(cfg.media_flag))
                        display_source = nullptr;

                    for (auto pin : node->get_object()->input)
                    {
                        if (PinType::COMBO == pin->def_state.type)
                        {
                            const auto source = fmt::format(FMT_STRING("[{}]"), pin->def_state.name);
                            if (ImGui::MenuItem(source.c_str(), nullptr, display_source != nullptr && pin->def_state.name == *display_source))
                            {
                                node->get_object()->custom_widget_source = pin;
                                save_opts();
                            }
                        }
                    }

                    ImGui::EndMenu();
                }
            }

            ImGui::PopItemFlag();

            ImGui::Separator();
            if (ImGui::MenuItem("Reset"))
            {
                g::pFlow->custom_widgets().reset_widgets();
                g::pHostManager->kill_all();
                node->obj_flags().remove_raw(g_node_obj_all_custom_widget_mask.get_raw());
                save_opts();
                close = true;
            }

            ImGui::EndMenu();
        }
    }

    return close;
}

bool PanelWorkflow::show_group_menu()
{
    bool close = false;
    auto group = m_menu_objs.groups.begin()->first;
    auto& nodes = m_menu_objs.groups.begin()->second;

    if (ImGui::MenuItem("Rename", nullptr, nullptr, m_drawer.zoomed_allow_renaming()))
    {
        m_drawer.rename_node(group);
        close = true;
    }
    if (!nodes.empty())
    {
        if (ImGui::MenuItem("Fit"))
        {
            m_action->set_node_rect(group, m_drawer.get_group_rect(nodes));
            close = true;
        }
        if (ImGui::MenuItem("Select nodes"))
        {
            m_drawer.clear_selection();
            for (auto node : nodes)
            {
                m_drawer.select_node(node->ed(), true);
            }
        }
    }
    ImGui::Separator();
    if (ImGui::MenuItem(nodes.empty() ? "Remove" : "Remove group", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Remove)))
    {
        delete_nodes({ group }, false);
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
            delete_nodes(std::move(objs), false);

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
        delete_nodes(std::move(m_menu_objs.nodes), false);
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
            m_action->set_node_collapsed(node, false);
        }
        close = true;
    }
    if (ImGui::MenuItem("Collapse", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Collapse)))
    {
        for (auto node : m_menu_objs.nodes)
        {
            m_action->set_node_collapsed(node, true);
        }
        close = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Always", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Always)))
    {
        m_action->multi_action_on("Always mode");
        for (auto node : m_menu_objs.nodes)
        {
            if (node->obj_flags().has_not(NodeObjFlags::Dummy))
                m_action->set_node_mode(node, NodeInstFlags::None);
        }
        m_action->multi_action_off();
        close = true;
    }
    if (ImGui::MenuItem("Bypass", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Bypass)))
    {
        m_action->multi_action_on("Bypass");
        for (auto node : m_menu_objs.nodes)
        {
            if (node->obj_flags().has_not(NodeObjFlags::Dummy))
                m_action->set_node_mode(node, NodeInstFlags::ModeBypass);
        }
        m_action->multi_action_off();
        close = true;
    }
    if (ImGui::MenuItem("Mute", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Mute)))
    {
        m_action->multi_action_on("Mute");
        for (auto node : m_menu_objs.nodes)
        {
            if (node->obj_flags().has_not(NodeObjFlags::Dummy))
                m_action->set_node_mode(node, NodeInstFlags::ModeMute);
        }
        m_action->multi_action_off();
        close = true;
    }
    ImGui::Separator();

    bool all_reroutes = true;
    for (auto node : m_menu_objs.nodes)
    {
        if (node->flags().has_not(NodeInstFlags::Reroute))
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
        delete_nodes(std::move(objs), false);
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
            delete_nodes(std::move(objs), false);
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
                m_action->set_node_rect(group, m_drawer.get_group_rect(nodes));
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
        delete_nodes(std::move(objs), false);
        close = true;
        create_toolbar();
        return close;
    }
    if (ImGui::MenuItem("Copy", g::pHotkeysManager->get_key_binding_name(KeyTarget::Node_Copy)))
    {
        std::set<data::NodeInstance*> objs;
        for (auto node : m_menu_objs.nodes)
        {
            if (node->obj_flags().has_not(NodeObjFlags::Dummy))
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

bool PanelWorkflow::show_widget_menu()
{
    LogRAssert(m_menu_objs.pins.size() == 1, "More than one pin cannot be selected", true);
    LogRAssert(m_menu_objs.nodes.size() == 1, "More than one node cannot be selected", true);

    bool close = false;

    auto pin = *m_menu_objs.pins.begin();

    const auto draw_cag_menu = [pin, action = m_action]()
    {
        bool close = false;
        const auto cag_val = pin->state().cag_val;

        ImGui::MenuItem("Control after generate:", nullptr, nullptr, false);
        if (ImGui::MenuItem("fixed", nullptr, ControlAfterGenerate::Fixed == cag_val, true))
        {
            pin->state().cag_val = ControlAfterGenerate::Fixed;
            close = true;
        }
        if (ImGui::MenuItem("increment", nullptr, ControlAfterGenerate::Increment == cag_val, true))
        {        
            pin->state().cag_val = ControlAfterGenerate::Increment;
            close = true;
        }
        if (ImGui::MenuItem("decrement", nullptr, ControlAfterGenerate::Decrement == cag_val, true))
        {        
            pin->state().cag_val = ControlAfterGenerate::Decrement;
            close = true;
        }
        if (ImGui::MenuItem("randomize", nullptr, ControlAfterGenerate::Randomize == cag_val, true))
        {        
            pin->state().cag_val = ControlAfterGenerate::Randomize;
            close = true;
        }

        if (close)
            action->set_pin_cag(pin, cag_val, pin->state().min, pin->state().max, pin->state().step);

        if (ControlAfterGenerate::Fixed == pin->state().cag_val)
            return close;

        if (pin->state().min.is_null()) pin->state().min = pin->get_object()->def_state.min;
        if (pin->state().max.is_null()) pin->state().max = pin->get_object()->def_state.max;
        if (pin->state().step.is_null()) pin->state().step = pin->get_object()->def_state.step;

        const auto handle_updated = [action, pin, old_min = pin->state().min, old_max = pin->state().max, old_step = pin->state().step]()
        {
            if (ImGui::IsItemDeactivatedAfterEdit())
                action->set_pin_cag(pin, pin->state().cag_val, old_min, old_max, old_step);
        };

        switch (pin->state().type)
        {
        case PinType::INT:
        case PinType::COMBO:
            {
                int64_t* min = pin->state().min.get_int_ref();
                int64_t* max = pin->state().max.get_int_ref();
                ImGuiEx::DragIntRange(0x0031501E, min, max, *min, *max, "Min: %lld", "Max: %lld");
                handle_updated();
                if ((ControlAfterGenerate::Increment == pin->state().cag_val || ControlAfterGenerate::Decrement == pin->state().cag_val) && PinType::INT == pin->state().type)
                {
                    int64_t* step = pin->state().step.get_int_ref();
                    const int64_t step_min = 1;
                    const int64_t step_max = *max / 2;
                    ImGui::DragScalar("##step", ImGuiDataType_S64, step, 1.0f, &step_min, &step_max, "Step: %lld");
                    handle_updated();
                }
            }
            break;
        case PinType::FLOAT:
            {
                double* min = pin->state().min.get_float_ref();
                double* max = pin->state().max.get_float_ref();
                ImGuiEx::DragDoubleRange(0x0040DB0D, min, max, *min, *max, "Min: %.2f", "Max: %.2f");
                handle_updated();
                if (ControlAfterGenerate::Increment == pin->state().cag_val || ControlAfterGenerate::Decrement == pin->state().cag_val)
                {
                    double* step = pin->state().step.get_float_ref();
                    const double step_min = 0.01f;
                    const double step_max = *max / 2;
                    ImGui::DragScalar("##step", ImGuiDataType_Double, step, 1.0f, &step_min, &step_max, "Step: %.2f");
                    handle_updated();
                }
            }
            break;
        case PinType::NUMBER:
            {
                number* min_num = pin->state().min.get_number_ref();
                number* max_num = pin->state().max.get_number_ref();
                double* min = min_num->underlying_type_ref();
                double* max = max_num->underlying_type_ref();
                ImGuiEx::DragDoubleRange(0x00F82C85, min, max, *min, *max, "Min: %.2f", "Max: %.2f");
                handle_updated();
                if (ControlAfterGenerate::Increment == pin->state().cag_val || ControlAfterGenerate::Decrement == pin->state().cag_val)
                {
                    number* step_num = pin->state().step.get_number_ref();
                    double* step = step_num->underlying_type_ref();
                    const double step_min = 0.01f;
                    const double step_max = *max / 2;
                    ImGui::DragScalar("##step", ImGuiDataType_Double, step, 1.0f, &step_min, &step_max, "Step: %.2f");
                    handle_updated();
                }
            }
            break;
        }
        return close;
    };

    const auto draw_color_menu = [pin, action = m_action]()
    {
        bool close = false;
        const bool hsv = pin->flags().has(PinInstFlags::ColorHSV);
        const bool hex = pin->flags().has(PinInstFlags::ColorHex);
        const bool raw = pin->flags().has(PinInstFlags::ColorRaw);
        const bool rgb = !hsv && !hex && !raw;
        if (raw || hex)
            pin->flags().remove(PinInstFlags::ColorFloat);
        const bool dec = pin->flags().has(PinInstFlags::ColorFloat);
        const bool int_ = !dec;

        const auto handle_updated = [pin, action, flags = pin->flags()]()
        {
            action->set_pin_color_config(pin, flags);
        };

        if (ImGui::MenuItem("Palette: RBG(A)", nullptr, rgb, true))
        {
            pin->flags().remove(PinInstFlags::ColorHSV, PinInstFlags::ColorHex, PinInstFlags::ColorRaw);
            handle_updated();
            close = true;
        }
        if (ImGui::MenuItem("Palette: HSV(A)", nullptr, hsv, true))
        {
            pin->flags().remove(PinInstFlags::ColorHSV, PinInstFlags::ColorHex, PinInstFlags::ColorRaw);
            pin->flags().add(PinInstFlags::ColorHSV);
            handle_updated();
            close = true;
        }
        if (ImGui::MenuItem("Palette: HEX", nullptr, hex, true))
        {
            pin->flags().remove(PinInstFlags::ColorHSV, PinInstFlags::ColorHex, PinInstFlags::ColorRaw);
            pin->flags().add(PinInstFlags::ColorHex);
            handle_updated();
            close = true;
        }
        if (ImGui::MenuItem("Palette: raw input", nullptr, raw, true))
        {
            pin->flags().remove(PinInstFlags::ColorHSV, PinInstFlags::ColorHex, PinInstFlags::ColorRaw);
            pin->flags().add(PinInstFlags::ColorRaw);
            handle_updated();
            close = true;
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Input format: integer", nullptr, int_, true))
        {
            pin->flags().remove(PinInstFlags::ColorFloat);
            handle_updated();
            close = true;
        }
        if (ImGui::MenuItem("Input format: decimal", nullptr, dec, rgb || hsv))
        {
            pin->flags().add(PinInstFlags::ColorFloat);
            handle_updated();
            close = true;
        }
        return close;
    };

    switch (pin->state().type)
    {
    case PinType::INT:
    case PinType::FLOAT:
    case PinType::NUMBER:
    case PinType::COMBO:
        if (pin->obj_flags().has(PinObjFlags::DisplAsColor))
            close = draw_color_menu();
        else
            close = draw_cag_menu();
        break;
    case PinType::STRING:
        if (pin->obj_flags().has_not(PinObjFlags::DenyMultilineSwitch))
        {
            if (ImGui::MenuItem("Single line", nullptr, !pin->is_multiline(), true))
            {
                m_action->set_pin_multiline(pin, false);
                close = true;
            }
            if (ImGui::MenuItem("Multi line", nullptr, pin->is_multiline(), true))
            {
                m_action->set_pin_multiline(pin, true);
                close = true;
            }
        }
        break;
    case PinType::COLOR:
        close = draw_color_menu();
    break;
    }

    return close;
}

bool PanelWorkflow::show_pin_menu()
{
    LogRAssert(m_menu_objs.pins.size() == 1, "More than one pin cannot be selected", true);
    LogRAssert(m_menu_objs.nodes.size() == 1, "More than one node cannot be selected", true);

    bool close = false;

    auto pin = *m_menu_objs.pins.begin();
    const bool linked = (pin->state().links > 0);
    const bool is_widget = pin->get_object()->is_widget();
    const bool name_eq = (pin->state().name == pin->get_object()->def_state.name);
    const bool can_rename = m_drawer.zoomed_allow_renaming() && pin->obj_flags().has(PinObjFlags::CanRename);

    if (ImGui::MenuItem("Unlink", nullptr, nullptr, linked && pin->obj_flags().has(PinObjFlags::CanUnlink)))
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
    const bool can_unlink = (link->get_input()->obj_flags().has(PinObjFlags::CanUnlink) && link->get_output()->obj_flags().has(PinObjFlags::CanUnlink));

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

    const bool is_input = pin_inst->obj_flags().has(PinObjFlags::Input);
    const auto input_filter = is_input ? std::list<std::pair<PinType, std::string>>{{ PinType::_none, "" }} : node_inst->get_supported_pin_types(pin_inst);
    const auto output_filter = is_input ? node_inst->get_supported_pin_types(pin_inst) : std::list<std::pair<PinType, std::string>>{{ PinType::_none, "" }};
    const auto node = show_nodes_menu(input_filter, output_filter);
    if (node != nullptr)
    {
        m_action->multi_action_on("Add node");
        close = true;
        bool linked = false;
        const auto* inst = m_action->add_node(node, "", ImRect(m_menu_objs.click_pos, ImVec2(-1, -1)));
        const auto& oppo = is_input ? inst->get_outputs() : inst->get_inputs();
        const auto& filter = is_input ? output_filter : input_filter;
        if (PinType::ANY == filter.front().first && !oppo.empty())
        {
            if (pin_inst->state().links > 0 && is_input)
                m_action->unlink(pin_inst);
            m_action->link(*oppo.begin(), pin_inst);
            linked = true;
        }
        if (!linked)
        {
            for (auto pin : oppo)
            {
                if (std::ranges::find_if(filter, [&type = pin->state()](const std::pair<PinType, std::string>& filter)
                {
                    if (PinType::CUSTOM == filter.first)
                        return filter.second == type.type_str;
                    return filter.first == type.type;
                })
                    != filter.end())
                {
                    if (pin_inst->state().links > 0 && is_input)
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
                if (pin->obj_flags().has(PinObjFlags::TypeAny))
                {
                    if (pin_inst->state().links > 0 && is_input)
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

data::Node* PanelWorkflow::show_nodes_menu(const std::list<std::pair<PinType, std::string>>& in_pins, const std::list<std::pair<PinType, std::string>>& out_pins)
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
        const auto nodes_in_label = g::pNodesManager->get_nodes_in_label(label.id);
        for (auto node_item : nodes_in_label)
        {
            if (!node_item->filtered)
                nodes_count++;
        }

        if (nodes_count > 0)
        {
            labels_shown = true;

            m_fmt_buffer.clear();
            fmt::format_to(std::back_inserter(m_fmt_buffer), FMT_STRING("{} [{}]"), label.name, nodes_count);
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
