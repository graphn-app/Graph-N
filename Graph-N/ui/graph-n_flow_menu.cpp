
#include "ui/graph-n_flow.h"
#include "core/managers/settings_manager.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/queue_manager.h"
#include "core/managers/hotkeys_manager.h"
#include "ui/panels/panel_wf.h"
#include "main.h"

namespace ui
{

void GraphN_Flow::show_menu_bar()
{
    const auto* hk = g::pHotkeysManager;
    const bool connecting_mode = (m_windows & ViewConnect);
    if (connecting_mode)
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, g::pOptsManager->color_load(AppColor::DisconnectedMenuBarBg));

    if (ImGui::BeginMainMenuBar())
    {
        const bool nodes_ready = get_nodes_ready();
        const bool active_wf = g::pWfsManager->get_active() != 0;
        const bool online = get_online();

        if (ImGui::BeginMenu("File##MainMenuBar"))
        {
            if (ImGui::MenuItem("Import Project from file...##FileMenuBar", hk->get_key_binding_name(KeyTarget::App_Import), nullptr, nodes_ready))
            {
                on_menu_import();
            }
            if (ImGui::BeginMenu("Export...##MainMenuBar"))
            {
                if (ImGui::MenuItem("Export Project to json...##FileMenuBar", nullptr, nullptr, active_wf))
                {
                    on_menu_export_json(false);
                }
                if (ImGui::MenuItem("Export Project to json (API)...##FileMenuBar", nullptr, nullptr, active_wf))
                {
                    on_menu_export_json(true);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Export all Projects to json...##FileMenuBar", nullptr, nullptr, g::pWfsManager->get_wfs_count() > 0))
                {
                    on_menu_export_json_all(false);
                }
                if (ImGui::MenuItem("Export all Projects to json (API)...##FileMenuBar", nullptr, nullptr, g::pWfsManager->get_wfs_count() > 0))
                {
                    on_menu_export_json_all(true);
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Options##FileMenuBar", hk->get_key_binding_name(KeyTarget::App_Options)))
            {
                on_menu_options();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Toggle full screen mode##FileMenuBar", hk->get_key_binding_name(KeyTarget::App_FullScreen)))
            {
                on_menu_fullscreen();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Restart server##FileMenuBar", hk->get_key_binding_name(KeyTarget::App_Restart), nullptr, online))
            {
                on_menu_restart();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit " APP_CAPTION "##FileMenuBar", hk->get_key_binding_name(KeyTarget::App_Quit)))
            {
                on_menu_quit();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Projects##MainMenuBar"))
        {
            if (ImGui::MenuItem("Add new Project##WfMenuBar", hk->get_key_binding_name(KeyTarget::Project_CreateNew), nullptr, nodes_ready))
            {
                on_menu_activate_wf(nullptr);
            }
            if (ImGui::MenuItem("Rename active Project##WfMenuBar", hk->get_key_binding_name(KeyTarget::Project_Rename), nullptr, nodes_ready))
            {
                on_menu_wf_rename();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Sync and manage Projects...##WfMenuBar"))
            {
                on_menu_sync_projects();
            }
            if (!m_panels_wf.empty())
            {
                ImGui::Separator();
                if (ImGui::MenuItem("Switch to the next Project##WfMenuBar", hk->get_key_binding_name(KeyTarget::Wf_Next)))
                {
                    on_menu_next_project(true);
                }
                if (ImGui::MenuItem("Switch to the previous Project##WfMenuBar", hk->get_key_binding_name(KeyTarget::Wf_Prev)))
                {
                    on_menu_next_project(false);
                }
                ImGui::Separator();
                for (auto& wf : m_panels_wf)
                {
                    const auto wf_name = fmt::format(FMT_STRING("{}##PanelsMenuBar"), wf->get_panel_name_full());
                    if (ImGui::MenuItem(wf_name.c_str(), nullptr, wf.get() == m_wf_focused))
                    {
                        on_menu_activate_wf(wf.get());
                    }
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Close all Projects##WfMenuBar", nullptr, nullptr, !m_panels_wf.empty()))
            {
                on_menu_close_all_projects();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Panels##MainMenuBar"))
        {
            if (ImGui::MenuItem("Nodes Library##PanelsMenuBar", hk->get_key_binding_name(KeyTarget::PanelOpen_Nodes), (m_windows & ViewPanelNodes)))
            {
                on_menu_panel_window(ViewPanelNodes);
            }
            if (ImGui::MenuItem("Models Library##PanelsMenuBar", hk->get_key_binding_name(KeyTarget::PanelOpen_Models), (m_windows & ViewPanelModels)))
            {
                on_menu_panel_window(ViewPanelModels);
            }
            if (ImGui::MenuItem("Node Inspector##PanelsMenuBar", hk->get_key_binding_name(KeyTarget::PanelOpen_Inspector), (m_windows & ViewPanelNodeInpector)))
            {
                on_menu_panel_window(ViewPanelNodeInpector);
            }
            if (ImGui::MenuItem("My Projects##PanelsMenuBar", hk->get_key_binding_name(KeyTarget::PanelOpen_Projects), (m_windows & ViewPanelWfList)))
            {
                on_menu_panel_window(ViewPanelWfList);
            }
            if (ImGui::MenuItem("Stats##PanelsMenuBar", hk->get_key_binding_name(KeyTarget::PanelOpen_Stats), (m_windows & ViewPanelStats)))
            {
                on_menu_panel_window(ViewPanelStats);
            }
            if (ImGui::MenuItem("Queue##PanelsMenuBar", hk->get_key_binding_name(KeyTarget::PanelOpen_Queue), (m_windows & ViewPanelQueue)))
            {
                on_menu_panel_window(ViewPanelQueue);
            }
            //if (ImGui::MenuItem("Notes##PanelsMenuBar", nullptr, (m_windows & ViewPanelNotes)))
            //{
            //    on_menu_panel_window(ViewPanelNotes);
            //}
            //if (ImGui::MenuItem("Templates##PanelsMenuBar", hk->get_key_binding_name(KeyTarget::PanelOpen_Templates), (m_windows & ViewPanelTemplates)))
            //{
            //    on_menu_panel_window(ViewPanelTemplates);
            //}
            if (ImGui::MenuItem("Server Log##PanelsMenuBar", hk->get_key_binding_name(KeyTarget::PanelOpen_Logs), (m_windows & ViewPanelOutput)))
            {
                on_menu_panel_window(ViewPanelOutput);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Show/hide active panels##PanelsMenuBar", hk->get_key_binding_name(KeyTarget::App_TogglePanels)))
            {
                on_menu_panel_toggle_active();
            }
            if (ImGui::MenuItem("Reset default layout##PanelsMenuBar"))
            {
                on_menu_panel_window(ViewPanelsResetAll);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Workspace##Workspace"))
        {
            if (ImGui::MenuItem("Save workflow##Workspace", hk->get_key_binding_name(KeyTarget::Workspace_Save), nullptr, wf_save(true)))
            {
                wf_save(false);
            }
            if (ImGui::MenuItem("Undo##Workspace", hk->get_key_binding_name(KeyTarget::Workspace_Undo), nullptr, wf_undo(true)))
            {
                wf_undo(false);
            }
            if (ImGui::MenuItem("Redo##Workspace", hk->get_key_binding_name(KeyTarget::Workspace_Redo), nullptr, wf_redo(true)))
            {
                wf_redo(false);
            }
            if (ImGui::MenuItem("Paste##Workspace", hk->get_key_binding_name(KeyTarget::Node_Paste), nullptr, wf_node_paste(true)))
            {
                wf_node_paste(false);
            }
            if (ImGui::MenuItem("Select all nodes##Workspace", hk->get_key_binding_name(KeyTarget::Workspace_SelAll), nullptr, m_wf_focused != nullptr))
            {
                wf_node_select_all();
            }
            if (ImGui::MenuItem("Focus##Workspace", hk->get_key_binding_name(KeyTarget::Workspace_Focus), nullptr, m_wf_focused != nullptr))
            {
                wf_focus();
            }
            if (ImGui::MenuItem("New node##Workspace", hk->get_key_binding_name(KeyTarget::Workspace_NewNode), nullptr, m_wf_focused != nullptr))
            {
                wf_new_node();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Close workflow##Workspace", hk->get_key_binding_name(KeyTarget::Workspace_Close), nullptr, m_wf_focused != nullptr))
            {
                wf_panel_close(m_wf_focused);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Queue##Queue"))
        {
            if (ImGui::MenuItem("Queue prompt##PanelsQueue", hk->get_key_binding_name(KeyTarget::Workspace_RunBack), nullptr, wf_can_run()))
            {
                wf_run(false);
            }
            if (ImGui::MenuItem("Queue prompt (front)##PanelsQueue", hk->get_key_binding_name(KeyTarget::Workspace_RunFront), nullptr, wf_can_run()))
            {
                wf_run(true);
            }
            const bool paused = g::pQueueManager->is_paused();
            if (ImGui::MenuItem(paused ? "Continue queue##PanelsQueue" : "Pause queue##PanelsQueue", hk->get_key_binding_name(KeyTarget::Queue_Pause)))
            {
                on_menu_queue_pause();
            }
            if (ImGui::MenuItem("Interrupt##PanelsQueue", hk->get_key_binding_name(KeyTarget::Queue_Interrupt), nullptr, g::pQueueManager->get_current_group() != nullptr))
            {
                on_menu_queue_interrupt();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help##MainMenuBar"))
        {
#ifdef _DEBUG
            if (ImGui::MenuItem("Debug metrics##HelpMenuBar", nullptr, (m_windows & ViewDebugMetrics)))
            {
                if (m_windows & ViewDebugMetrics)
                    m_windows &= ~ViewDebugMetrics;
                else
                    m_windows |= ViewDebugMetrics;
            }
            if (ImGui::MenuItem("Debug logs##HelpMenuBar", nullptr, (m_windows & ViewDebugLogs)))
            {
                if (m_windows & ViewDebugLogs)
                    m_windows &= ~ViewDebugLogs;
                else
                    m_windows |= ViewDebugLogs;
            }
            if (ImGui::MenuItem("Demo##HelpMenuBar", nullptr, (m_windows & ViewDebugDemo)))
            {
                if (m_windows & ViewDebugDemo)
                    m_windows &= ~ViewDebugDemo;
                else
                    m_windows |= ViewDebugDemo;
            }
            if (ImGui::MenuItem("Stack tool##HelpMenuBar", nullptr, (m_windows & ViewStackTool)))
            {
                if (m_windows & ViewStackTool)
                    m_windows &= ~ViewStackTool;
                else
                    m_windows |= ViewStackTool;
            }
            if (ImGui::MenuItem("Style editor##HelpMenuBar", nullptr, (m_windows & ViewStyleEditor)))
            {
                if (m_windows & ViewStyleEditor)
                    m_windows &= ~ViewStyleEditor;
                else
                    m_windows |= ViewStyleEditor;
            }
            if (ImGui::MenuItem("Style selector##HelpMenuBar", nullptr, (m_windows & ViewStyleSelector)))
            {
                if (m_windows & ViewStyleSelector)
                    m_windows &= ~ViewStyleSelector;
                else
                    m_windows |= ViewStyleSelector;
            }
            if (ImGui::MenuItem("Font selector##HelpMenuBar", nullptr, (m_windows & ViewFontSelector)))
            {
                if (m_windows & ViewFontSelector)
                    m_windows &= ~ViewFontSelector;
                else
                    m_windows |= ViewFontSelector;
            }
            ImGui::Separator();
#endif
            if (ImGui::MenuItem("About " APP_CAPTION "##HelpMenuBar"))
            {
                on_menu_about();
            }
            ImGui::EndMenu();
        }

        show_menu_bar_net_status();

        ImGui::EndMainMenuBar();
    }

    if (connecting_mode)
        ImGui::PopStyleColor();
}

void GraphN_Flow::on_menu_options()
{
    m_windows |= ViewOptions;
    g::pOptsManager->ui_open_wnds_save(m_windows);
}

void GraphN_Flow::on_menu_quit() noexcept
{
    g::pGraphN->handle_wnd_on_close();
}

void GraphN_Flow::on_menu_about() noexcept
{
    //TODO
}

void GraphN_Flow::on_menu_panel_window(ViewDialogs wnd, bool toggle /*= false*/)
{
    LogV(wnd, toggle);
    if (toggle && wnd != ViewPanelsResetAll)
    {
        const bool displayed = (m_windows & wnd) > 0;
        if (displayed) m_windows &= ~wnd;
        else m_windows |= wnd;
    }
    else
        m_windows |= wnd;

    if (wnd != ViewPanelsResetAll)
        g::pOptsManager->ui_open_wnds_save(m_windows);
}

void GraphN_Flow::on_menu_panel_toggle_active() noexcept
{
    // isolate currently visible panels
    const auto current_panels = (m_windows & ViewAllPanels);

    if (current_panels > 0)
    {
        // if N panels are open: save their state and hide them
        m_windows_toggled = current_panels;
        m_windows &= ~current_panels;
    }
    else
    {
        if (m_windows_toggled != ViewNone)
        {
            // if panels are hidden by this function: restore them
            m_windows |= m_windows_toggled;

            // reset saved state so manual closing after this works properly
            m_windows_toggled = ViewNone; 
        }
        else
        {
            // if 0 panels are open and no saved state exists (closed manually): show all
            m_windows |= ViewAllPanels;
        }
    }
}

void GraphN_Flow::on_menu_fullscreen()
{
    g::pMain->wnd_toggle_fullscreen();
}

void GraphN_Flow::on_menu_command() noexcept
{
    // TODO
}

void GraphN_Flow::on_menu_activate_wf(const PanelWorkflow* wf_panel)
{
    if (wf_panel != nullptr)
    {
        LogV(wf_panel->get_panel_name());
        wf_activate(wf_panel->get_workflow()->get_id());
    }
    else
    {
        wf_create();
    }
}

void GraphN_Flow::on_menu_wf_rename() noexcept
{
    wf_rename();
}

void GraphN_Flow::on_menu_close_all_projects()
{
    for (auto it = m_panels_wf.begin(); it != m_panels_wf.end(); )
    {
        wf_panel_close(it->get());
        it = m_panels_wf.begin();
    }
}

void GraphN_Flow::on_menu_sync_projects()
{
    m_windows |= ViewSyncProjects;
    g::pOptsManager->ui_open_wnds_save(m_windows);
}

void GraphN_Flow::on_menu_next_project(bool next)
{
    wf_panel_activate_next(next);
}

void GraphN_Flow::on_menu_restart() noexcept
{
    // TODO
}

void GraphN_Flow::on_menu_import()
{
    tools::path::FdSelectResultMt callback = [this](bool result, std::list<std::filesystem::path>&& paths)
    {
        if (result)
        {
            import_files(std::move(paths));
        }
    };
    tools::path::select_files_open({{ L"ComfyUI Workflows (*.json, *.png, *.webp)", L"*.json;*.png;*.webp" }}, callback);
}

void GraphN_Flow::on_menu_export_json(bool api)
{
    const auto id = g::pWfsManager->get_active();
    LogRAssert(id != 0, "No active workflows", );

    const auto& data = g::pWfsManager->access(id)->get_data();
    auto filename = data.name;
    filename = tools::path::clear_filename(filename) + (api ? " (API).json" : ".json");

    tools::path::FdSelectResultSg callback = [api, id](bool result, std::filesystem::path&& path)
    {
        if (result)
        {
            g::pWfsManager->export_wf(id, api, path);
        }
    };

    tools::path::select_file_save({{ L"Json file (*.json)", L"*.json" }}, filename, callback);
}

void GraphN_Flow::on_menu_export_json_all(bool api)
{
    tools::path::FdSelectResultSg callback = [api](bool result, std::filesystem::path&& path)
    {
        if (result)
        {
            const auto wfs = g::pWfsManager->get_undeleted();
            if (wfs.empty())
                return;

            for (auto wf_data : wfs)
            {
                const auto& data = g::pWfsManager->access(wf_data->id)->get_data();
                auto filename = data.name;
                filename = tools::path::clear_filename(filename) + (api ? " (API).json" : ".json");
                g::pWfsManager->export_wf(wf_data->id, api, path / filename);
            }
        }
    };

    tools::path::select_dir(callback);
}

void GraphN_Flow::on_menu_queue_pause() noexcept
{
    g::pQueueManager->toggle_pause();
}

void GraphN_Flow::on_menu_queue_interrupt()
{
    g::pQueueManager->interrupt_task();
}

}

