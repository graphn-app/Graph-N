/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "ui/graph-n_flow.h"
#include "ui/base_dlg.h"
#include "ui/custom_nodes/custom_media_widget.h"
#include "ui/custom_nodes/custom_js_node.h"
#include "ui/dialogs/about_dlg.h"
#include "ui/dialogs/options/opts_dlg.h"
#include "ui/dialogs/modal_message.h"
#include "ui/dialogs/sync_projects_dlg.h"
#ifdef _DEBUG
#include "ui/dialogs/debug_wf_view.h"
#endif
#include "ui/panels/activity_bar.h"
#include "ui/panels/panel_nodes.h"
#include "ui/panels/panel_node_inspector.h"
#include "ui/panels/panel_stats.h"
#include "ui/panels/panel_wf.h"
#include "ui/panels/panel_text_editor.h"
#include "ui/panels/panel_wf_list.h"
#include "ui/panels/panel_models.h"
#include "ui/panels/panel_queue.h"
#include "ui/panels/panel_notes.h"
#include "ui/panels/panel_logs.h"
#include "ui/panels/panel_templates.h"
#include "core/managers/settings_manager.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/nodes_parser.h"
#include "core/managers/db_manager.h"
#include "core/managers/workflows_manager.h"
#include "core/managers/models_manager.h"
#include "core/managers/comfyui_manager.h"
#include "core/managers/history_manager.h"
#include "core/managers/queue_manager.h"
#include "core/managers/hotkeys_manager.h"
#include "core/managers/host_manager.h"
#include "core/workflow/workflow.h"
#include "core/workflow/node_instance.h"
#include "core/workflow/workflow_action.h"
#include "core/workflow/serializer/wf_serializers.h"
#include "core/base/comfyui_web_paths.h"
#include "core/base/version.h"
#include "main.h"

namespace w
{
float pin_l_offset_ro = 0.0f;
float pin_l_offset_rw = 0.0f;
float pin_r_offset = 0.0f;
float ico_circle = 0.0f;
float ico_eye = 0.0f;
float ico_max = 0.0f;
float v_spacing = 0.0f;
}

namespace ui
{

GraphN_Flow::GraphN_Flow() noexcept = default;
GraphN_Flow::~GraphN_Flow() noexcept = default;

bool GraphN_Flow::init()
{
    LogIn;
    w::pin_l_offset_ro = 4.0f;
    w::pin_l_offset_rw = 0.0f;
    w::pin_r_offset = 4.0f;
    w::ico_circle = ImGui::CalcTextSize(ICON_LC_CIRCLE_SMALL).x;
    w::ico_eye = ImGui::CalcTextSize(ICON_LC_EYE).x;
    w::ico_max = std::max(w::ico_circle, w::ico_eye);
    w::v_spacing = 4.0f;

    m_ui_thread_id = std::this_thread::get_id();

    // menu colors
    ImGuiStyle& style = ImGui::GetStyle();

    // Background: Dark Grey (Opaque) - Lighter than main window
    style.Colors[ImGuiCol_PopupBg] = ImColor(g::pOptsManager->color_load(AppColor::PopupBg));
    style.Colors[ImGuiCol_MenuBarBg] = ImColor(g::pOptsManager->color_load(AppColor::MenubarBg));

    m_custom_media_widget = std::make_unique<CustomMediaWidget>();
    m_custom_js_node = std::make_unique<CustomJsNode>();

    // open windows
    m_windows = g::pOptsManager->ui_open_wnds_load();

    // create panels
    m_panel_node_inspector = std::make_unique<PanelNodeInspector>();
    m_panel_node_inspector->init();
    m_all_dialogs.insert(m_panel_node_inspector.get());

    m_panel_nodes = std::make_unique<PanelNodes>(m_panel_node_inspector.get());
    m_panel_nodes->init();
    m_all_dialogs.insert(m_panel_nodes.get());

    m_panel_stats = std::make_unique<PanelStats>();
    m_panel_stats->init();
    m_all_dialogs.insert(m_panel_stats.get());

    m_panel_wf_list = std::make_unique<PanelWfList>();
    m_panel_wf_list->init();
    m_all_dialogs.insert(m_panel_wf_list.get());

    //m_panel_templates = std::make_unique<PanelTemplates>();
    //m_panel_templates->init();
    //m_all_dialogs.insert(m_panel_templates.get());

    m_panel_logs = std::make_unique<PanelServerLogs>();
    m_panel_logs->init();
    m_all_dialogs.insert(m_panel_logs.get());

    //m_panel_notes = std::make_unique<PanelNotes>();
    //m_panel_notes->init();
    //m_all_dialogs.insert(m_panel_notes.get());

    m_panel_models = std::make_unique<PanelModels>();
    m_panel_models->init();
    m_all_dialogs.insert(m_panel_models.get());

    m_panel_queue = std::make_unique<PanelQueue>();
    m_panel_queue->init();
    m_all_dialogs.insert(m_panel_queue.get());

  //  wf_panels_init();

    // activity bar
    m_activity_bar = std::make_unique<ActivityBar>();
    load_activity_bar_mode();
    m_activity_bar->register_panels({ // except PanelServerLogs
        DlgObj::PanelNodes,
        DlgObj::PanelModels,
        DlgObj::PanelQueue,
        DlgObj::PanelNodeInspector,
        DlgObj::PanelWfList,
        //DlgObj::PanelTemplates,
        DlgObj::PanelStats,
    });

    if (g::pDbManager->get_is_first_run() || !g::pMain->get_ini_config_exists())
        m_windows |= ViewPanelsResetAll;

    Log("Panels created");

    //ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive] = ImGui::GetStyle().Colors[ImGuiCol_TitleBg];
    ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive] = ImGui::GetStyle().Colors[ImGuiCol_TitleBg];
    ImGui::GetStyle().Colors[ImGuiCol_Tab] = ImGui::ColorConvertU32ToFloat4(g::pOptsManager->color_load(AppColor::TabInactive));
    ImGui::GetStyle().Colors[ImGuiCol_TabUnfocused] = ImGui::ColorConvertU32ToFloat4(g::pOptsManager->color_load(AppColor::TabInactive));
    ImGui::GetStyle().Colors[ImGuiCol_TabUnfocusedActive] = ImGui::ColorConvertU32ToFloat4(g::pOptsManager->color_load(AppColor::TabInactive));
    ImGui::GetStyle().Colors[ImGuiCol_TabHovered] = ImGui::ColorConvertU32ToFloat4(g::pOptsManager->color_load(AppColor::TabHovered));
    ImGui::GetStyle().Colors[ImGuiCol_TabActive] = ImGui::ColorConvertU32ToFloat4(g::pOptsManager->color_load(AppColor::TabActive));

    const auto dnd_callback = [this](std::list<std::filesystem::path>&& files)
    {
        if (!get_is_quit() && get_nodes_ready() && get_projects_ready())
            import_files(std::move(files));
    };
    g::pMain->set_dnd_callback(dnd_callback);

    load_hotkeys();
    g::block_hotkeys_mask = 0;

    g::pOptsManager->items_module_colors_load_all();

    g::pGraphN->subscribe_timers(this);

    LogOut;
    return true;
}

void GraphN_Flow::reconnect()
{
    m_stats.user_sent = {};
    m_windows |= ViewConnect;
    show_logs_panel(true);
    g::pConnectManager->reconnect();
}

void GraphN_Flow::load_activity_bar_mode() noexcept
{
    m_activity_bar_mode = g::pOptsManager->ui_activity_bar_mode_load();
    m_windows &= ~(ViewActivityBarL | ViewActivityBarR);

    if (ActivityBarMode::Smart == m_activity_bar_mode || ActivityBarMode::Left == m_activity_bar_mode)
    {
        m_windows |= ViewActivityBarL;
    }
    else if (ActivityBarMode::Right == m_activity_bar_mode)
    {
        m_windows |= ViewActivityBarR;
    }
}

bool GraphN_Flow::is_panel_active(DlgObj panel_obj) const noexcept
{
    switch (panel_obj)
    {
    case DlgObj::PanelNodes:
        return (m_windows & ViewPanelNodes) != 0;
    case DlgObj::PanelModels:
        return (m_windows & ViewPanelModels) != 0;
    case DlgObj::PanelQueue:
        return (m_windows & ViewPanelQueue) != 0;
    case DlgObj::PanelNodeInspector:
        return (m_windows & ViewPanelNodeInpector) != 0;
    case DlgObj::PanelWfList:
        return (m_windows & ViewPanelWfList) != 0;
    //case DlgObj::PanelTemplates:
    //    return (m_windows & ViewPanelTemplates) != 0;
    case DlgObj::PanelStats:
        return (m_windows & ViewPanelStats) != 0;
    case DlgObj::PanelServerLogs:
        return (m_windows & ViewPanelOutput) != 0;
    //case DlgObj::PanelNotes:
    //    return (m_windows & ViewPanelNotes) != 0;
    case DlgObj::PanelWf:
        return (m_windows & ViewPanelWf) != 0;
    case DlgObj::PanelTextEditor:
        return (m_windows & ViewPanelTextEditor) != 0;
    //case DlgObj::PanelMarkdownPreview:
    //    return (m_windows & ViewPanelMarkdownPreview) != 0;
    }
    LogRError("Unhandled panel id " << cast::to_i32(panel_obj), false);
}

const char* GraphN_Flow::get_panel_icon(DlgObj panel_obj) const noexcept
{
    switch (panel_obj)
    {
    case DlgObj::PanelNodes:
        return ICON_LC_BOOK_OPEN;
    case DlgObj::PanelModels:
        return ICON_LC_COMPONENT;
    case DlgObj::PanelQueue:
        return ICON_LC_FOOTPRINTS;
    case DlgObj::PanelNodeInspector:
        return ICON_LC_TANGENT;
    case DlgObj::PanelWfList:
    case DlgObj::PanelWf:
        return ICON_LC_WORKFLOW;
    //case DlgObj::PanelTemplates:
    //    return ICON_LC_LAYOUT_TEMPLATE;
    case DlgObj::PanelStats:
        return ICON_LC_CHART_LINE;
    case DlgObj::PanelServerLogs:
        return ICON_LC_TERMINAL;
    case DlgObj::PanelTextEditor:
        return ICON_LC_TEXT_INITIAL;
    //case DlgObj::PanelMarkdownPreview:
    //    return ICON_LC_HEADING_1;
    //case DlgObj::PanelNotes:
    //    return ICON_LC_SCROLL_TEXT;
    case DlgObj::PanelDebugWf:
        return ICON_LC_BUG;
    }
    LogRError("Unhandled panel id " << cast::to_i32(panel_obj), "");
}

const char* GraphN_Flow::get_panel_name(DlgObj panel_obj) const noexcept
{
    switch (panel_obj)
    {
    case DlgObj::PanelNodes:
        return m_panel_nodes->get_panel_name();
    case DlgObj::PanelModels:
        return m_panel_models->get_panel_name();
    case DlgObj::PanelQueue:
        return m_panel_queue->get_panel_name();
    case DlgObj::PanelNodeInspector:
        return m_panel_node_inspector->get_panel_name();
    case DlgObj::PanelWfList:
        return m_panel_wf_list->get_panel_name();
    //case DlgObj::PanelTemplates:
    //    return m_panel_templates->get_panel_name();
    case DlgObj::PanelStats:
        return m_panel_stats->get_panel_name();
    case DlgObj::PanelServerLogs:
        return m_panel_logs->get_panel_name();
//    case DlgObj::PanelMarkdownPreview:
//        return m_panel_nodes->get_panel_name();
//    case DlgObj::PanelNotes:
//        return m_panel_notes->get_panel_name();
    }
    LogRError("Unhandled panel id " << cast::to_i32(panel_obj), nullptr);
}

void GraphN_Flow::set_panel_visibility(DlgObj panel_obj, bool show_panel)
{
    const bool active = is_panel_active(panel_obj);
    if (active != show_panel)
        toggle_panel_visibility(panel_obj);
}

void GraphN_Flow::toggle_panel_visibility(DlgObj panel_obj)
{
    int32_t panel_view = 0;

    switch (panel_obj)
    {
    case DlgObj::PanelNodes:
        panel_view = ViewPanelNodes;
        break;
    case DlgObj::PanelModels:
        panel_view = ViewPanelModels;
        break;
    case DlgObj::PanelQueue:
        panel_view = ViewPanelQueue;
        break;
    case DlgObj::PanelNodeInspector:
        panel_view = ViewPanelNodeInpector;
        break;
    case DlgObj::PanelWfList:
        panel_view = ViewPanelWfList;
        break;
    //case DlgObj::PanelTemplates:
    //    panel_view = ViewPanelTemplates;
    //    break;
    case DlgObj::PanelStats:
        panel_view = ViewPanelStats;
        break;
    case DlgObj::PanelServerLogs:
        panel_view = ViewPanelOutput;
        break;
    //case DlgObj::PanelMarkdownPreview:
    //    panel_view = ViewPanelMarkdownPreview;
    //    break;
    //case DlgObj::PanelNotes:
    //    panel_view = ViewPanelNotes;
    //    break;
    default:
        LogRError("Unhandled panel id " << cast::to_i32(panel_obj), );
    }

    if ((m_windows & panel_view) != 0)
        m_windows &= ~panel_view;
    else
    {
        m_windows |= panel_view;
        m_panel_appeared = panel_view;
    }
    g::pOptsManager->ui_open_wnds_save(m_windows);
}

bool GraphN_Flow::run()
{
    show_menu_bar();
    g::pHotkeysManager->process_hotkey();

    if ((m_windows & ViewActivityBarL) || (m_windows & ViewActivityBarR))
        show_activity_bar();

    if ((m_windows & ViewAbout) > 0)
        show_about_dlg(false);

    if ((m_windows & ViewOptions) > 0)
        show_options_dlg();

    if ((m_windows & ViewSyncProjects) > 0)
        show_sync_projs_dlg();

    show_panels();

#ifdef _DEBUG
    show_debug_windows();
#endif

    if ((m_windows & ViewModal) > 0 || (m_windows & ViewModalStop) > 0)
        show_modal();

    if (!m_toast.expired)
        ImGuiEx::toast_show(m_toast);

    check_connection();

    static bool wf_panels_initialized = false;
    if (!wf_panels_initialized && check_wf_panels_initialized())
        wf_panels_initialized = true;

    g::pQueueManager->process();

    check_http_reply();
    check_host_reply();

    return true;
}

void GraphN_Flow::cleanup_frame()
{
    if (m_pending_cleanups.about_dlg)
    {
        m_about_dlg->stop_and_cleanup();
        m_all_dialogs.erase(m_about_dlg.get());
        m_about_dlg.reset();
    }

    if (m_pending_cleanups.opts_dlg)
    {
        m_opts_dlg->stop_and_cleanup();
        m_all_dialogs.erase(m_opts_dlg.get());
        m_opts_dlg.reset();
    }

    if (m_pending_cleanups.sync_projs_dlg)
    {
        m_sync_projs_dlg->stop_and_cleanup();
        m_all_dialogs.erase(m_sync_projs_dlg.get());
        m_sync_projs_dlg.reset();
    }

    if (m_pending_cleanups.modal_dlg)
    {
        m_modal_message.reset();
    }

    for (auto panel : m_pending_cleanups.text_editors)
    {
        panel->stop_and_cleanup();
        std::erase_if(m_panels_text_editor, [panel](const std::unique_ptr<PanelTextEditor>& ptr) { return ptr.get() == panel; });
        m_all_dialogs.erase(panel);
    }

    for (auto& wf_panel : m_panels_wf)
    {
        wf_panel->cleanup_frame();
    }

    const bool reassign_sequential_numbers = !m_pending_cleanups.wf_panels.empty();
    for (auto panel : m_pending_cleanups.wf_panels)
    {
        if (m_wf_focused == panel)
            m_wf_focused = nullptr;

        const wid panel_wf = panel->get_workflow()->get_id();
        txt_panel_clear(panel_wf);
        g::pHistoryManager->cleanup(panel_wf);
        g::pWfsManager->close(panel_wf, false);

        panel->stop_and_cleanup();
        m_all_dialogs.erase(panel);
        std::erase_if(m_panels_wf, [panel](const std::unique_ptr<PanelWorkflow>& ptr) { return ptr.get() == panel; });
    }
    if (reassign_sequential_numbers)
    {
        g::pWfsManager->set_active(wf_get_active());

        if (m_panels_wf.empty())
        {
            for (auto& ptr : m_panels_text_editor)
            {
                ptr->reset();
            }
            m_panels_text_editor.clear();
            m_windows &= ~ViewPanelTextEditor;
        }

        std::list<std::pair<std::string, bool>> active_wfs;
        for (auto it = m_panels_wf.begin(); it != m_panels_wf.end(); ++it)
        {
            auto& panel = *it;
            active_wfs.push_back({ panel->get_workflow()->get_uuid(), m_wf_focused == panel.get() });
        }
        g::pOptsManager->wf_active_save(active_wfs);
        on_dlg_closed(DlgObj::PanelWf);

        wf_assign_sequential_numbers();
        m_panel_wf_list->rebuild_items();
    }

    g::pWfsManager->cleanup_frame();

    m_pending_cleanups = {};
}

void GraphN_Flow::prepare_to_quit()
{
    LogIn;

    stats_save_session_end();
    if (g::pGraphN != nullptr)
        g::pGraphN->unsubscribe_timers(this);

    dead();
    g::block_hotkeys_mask = 0xffffffff;

    if (m_nodes_parser)
        m_nodes_parser->stop_and_cleanup();
    m_custom_media_widget->reset_widgets();

    for (auto dlg : m_all_dialogs)
    {
        if (dlg)
            dlg->stop_and_cleanup();
    }
    m_wf_focused = nullptr;

    LogOut;
}

void GraphN_Flow::quit()
{
    LogIn;


    {
        std::unique_lock<std::mutex> lock(m_http_req_mutex);
        m_http_requests.clear();
    }
    m_pending_cleanups = {};

    m_nodes_parser.reset();
    m_custom_media_widget.reset();
    m_custom_js_node.reset();
    m_all_dialogs.clear();

    m_activity_bar.reset();
    m_panel_nodes.reset();
    m_panel_node_inspector.reset();
    //m_panel_templates.reset();
    //m_panel_notes.reset();
    m_panel_stats.reset();
    m_panel_logs.reset();
    m_panel_models.reset();
    m_panel_wf_list.reset();
    m_panel_queue.reset();
    m_panels_text_editor.clear();
    m_panels_wf.clear();

    m_about_dlg.reset();
    m_opts_dlg.reset();
    m_sync_projs_dlg.reset();
    m_modal_message.reset();

    LogOut;
}

void GraphN_Flow::show_logs_panel(bool show) noexcept
{
    if (g::pOptsManager->comfyui_autohide_output_load() && g::pOptsManager->comfyui_capture_output_load() &&
        g::pOptsManager->comfyui_start_server_load() && g::pComfyUIManager->get_process_started())
    {
        if (show)
            m_windows |= ViewPanelOutput;
        else
            m_windows &= ~ViewPanelOutput;
    }
}

void GraphN_Flow::check_connection()
{
    const auto status = g::pConnectManager->get_status();
    if (ConnectState::Disconnected == status)
    {
        g::pQueueManager->interrupt_task();
        m_wf_imported = false;
        m_projects_ready.store(false, std::memory_order_relaxed);
        reconnect();
    }
    else if (ConnectState::Connected == status)
    {
        if (!m_wf_imported)
            import_user_projects();
    }
}

void GraphN_Flow::import_user_projects()
{
    if (m_wf_imported || !m_nodes_ready.load(std::memory_order_relaxed))
        return;
    m_wf_imported = true;

    auto projects = std::make_shared<std::list<std::pair<std::string, tools::dt>>>();

    const auto all_names = [this, projects](int32_t result, std::string&& payload)
    {
        if (result != HTTP_RESP_OK) [[unlikely]]
        {
            m_wf_imported = false;
            m_projects_ready.store(true, std::memory_order_relaxed);
            return;
        }

        if (!data::WfDeserializerFromJson::deserialize_all_wf_names(payload, *projects)) [[unlikely]]
        {
            Log("All project names deserializarion failed");
            m_wf_imported = false;
            m_projects_ready.store(true, std::memory_order_relaxed);
            return;
        }

        if (projects->empty())
        {
            m_projects_ready.store(true, std::memory_order_relaxed);
            return;
        }

        auto counter = std::make_shared<int32_t>();
        for (auto& project : *projects)
        {
            if (!g::pOptsManager->wf_imported_load(project.first))
            {
                (*counter)++;
                const auto download_project = [this, name = project.first, time = project.second, counter](int32_t result, std::string&& payload)
                {
                    if (HTTP_RESP_OK == result)
                    {
                        if (g::pWfsManager->import_wf_from_json(payload, name, time) != 0)
                        {
                            g::pOptsManager->wf_imported_save(name);
                        }
                    }

                    (*counter)--;
                    if (0 == *counter)
                    {
                        m_panel_wf_list->rebuild_items();
                        m_projects_ready.store(true, std::memory_order_relaxed);
                    }
                };

                const auto url = fmt::format(comfyui_http::wf_download_, tools::str::url_encode(project.first));
                http_get(HttpRequestor::Flow_Import_projects, []() { return true; }, url.c_str(), download_project, true /*retry on timeout*/);
            }
        }

        if (0 == *counter)
        {
            m_panel_wf_list->rebuild_items();
            m_projects_ready.store(true, std::memory_order_relaxed);
        }
    };

    http_get(HttpRequestor::Flow_Import_projects, []() { return true; }, comfyui_http::wfs_all_names, all_names, true /*retry on timeout*/);
}

void GraphN_Flow::import_files(std::list<std::filesystem::path>&& paths)
{
    wid activate = 0;
    for (auto& path : paths)
    {
        const auto id = g::pWfsManager->import_wf(path);
        if (id != 0)
            activate = id;
    }

    if (paths.size() > 1)
        activate = 0;

    if (activate != 0)
        wf_activate(activate);
    else
        m_panel_wf_list->rebuild_items();
}

void GraphN_Flow::handle_connected()
{
    if (!get_online())
        return;

    m_windows &= ~ViewConnect;

    show_logs_panel(false);
    reparse_nodes();
}

void GraphN_Flow::reload_nodes()
{
    reparse_nodes();
}

void GraphN_Flow::reparse_nodes()
{
    const auto parser = [this](int32_t result, std::string&& data)
    {
        if (GraphN_Flow::get_is_quit()) return;
        if (result != HTTP_RESP_OK)
        {
            Log("Failed to load object_info");
            if (HTTP_RESP_TIMEOUT == result)
                reparse_nodes();
            return;
        }

#if 0
        auto path = fs::get_root_directory_path() / "object_info.json";
        data = tools::path::read_file(path, false);
#endif

        m_windows |= ViewParsingNodes;
        m_nodes_ready.store(false, std::memory_order_relaxed);
        m_models_ready.store(false, std::memory_order_relaxed);

        g::pModelsManager->load_models();

        m_nodes_parser = std::make_unique<mngr::NodesParser>();
        m_nodes_parser->parse(std::move(data));
    };

    http_get(HttpRequestor::Flow_Reparse_nodes, [](){ return true; }, comfyui_http::object_info, parser, true /*retry on timeout*/);
}

void GraphN_Flow::on_dlg_closed(DlgObj dlg)
{
    switch (dlg)
    {
    case DlgObj::AboutDlg:
        m_windows &= ~ViewAbout;
        m_pending_cleanups.about_dlg = true;
        Log("About dlg closed");
        break;
    case DlgObj::OptionsDlg:
        m_windows &= ~ViewOptions;
        m_pending_cleanups.opts_dlg = true;
        Log("Options dlg closed");
        break;
    case DlgObj::SyncProjects:
        m_windows &= ~ViewSyncProjects;
        m_pending_cleanups.sync_projs_dlg = true;
        Log("Sync projs dlg closed");
        break;
    case DlgObj::PanelNodes:
        m_windows &= ~ViewPanelNodes;
        Log("Nones panel closed");
        break;
    case DlgObj::PanelNodeInspector:
        m_windows &= ~ViewPanelNodeInpector;
        Log("Nones Inspector panel closed");
        break;
    case DlgObj::PanelStats:
        m_windows &= ~ViewPanelStats;
        Log("Stats panel closed");
        break;
    case DlgObj::PanelWf:
        if (m_panels_wf.empty())
            m_windows &= ~ViewPanelWf;
        Log("Wf panel closed");
        break;
    case DlgObj::PanelTextEditor:
        Log("Txt editor panel closed");
        break;
    case DlgObj::PanelWfList:
        m_windows &= ~ViewPanelWfList;
        Log("Wf list panel closed");
        break;
    //case DlgObj::PanelTemplates:
    //    m_windows &= ~ViewPanelTemplates;
    //    Log("Templates panel closed");
    //    break;
    case DlgObj::PanelServerLogs:
        m_windows &= ~ViewPanelOutput;
        Log("Output panel closed");
        break;
    //case DlgObj::PanelNotes:
    //    m_windows &= ~ViewPanelNotes;
    //    Log("Notes panel closed");
    //    break;
    case DlgObj::PanelModels:
        m_windows &= ~ViewPanelModels;
        Log("Models panel closed");
        break;
    case DlgObj::PanelQueue:
        m_windows &= ~ViewPanelQueue;
        Log("Queue panel closed");
        break;
    case DlgObj::ActivityBarL:
        m_windows &= ~ViewActivityBarL;
        Log("Left activity bar closed");
        break;
    case DlgObj::ActivityBarR:
        m_windows &= ~ViewActivityBarR;
        Log("Right activity bar closed");
        break;
#ifdef _DEBUG
    case DlgObj::PanelDebugWf:
        if (m_wf_debug_panels.empty())
            m_windows &= ~ViewWfDebug;
        Log("Wf debug dlgs closed");
        break;
#endif
    default:
        LogRError("Unknown dialog is trying to be closed " << cast::to_u64(dlg), );
    }

    g::pOptsManager->ui_open_wnds_save(m_windows);
}

void GraphN_Flow::txt_panel_create(wid wf, data::NodeInstance* node, data::PinInstance* pin, std::string* buffer, const char* hint)
{
    PanelTextEditor* current_panel = nullptr;
    PanelTextEditor* def_panel = nullptr;
    for (auto& ptr : m_panels_text_editor)
    {
        if (ptr->is_current(wf, node, pin))
        {
            current_panel = ptr.get();
            break;
        }
        if (ptr->is_default())
        {
            def_panel = ptr.get();
        }
    }

    if (nullptr == current_panel && nullptr == def_panel)
    {
        auto ptr = std::make_unique<PanelTextEditor>(DlgObj_SID[cast::to_szt(DlgObj::PanelTextEditor)], true);
        ptr->init();
        current_panel = ptr.get();
        m_all_dialogs.insert(ptr.get());
        m_panels_text_editor.push_back(std::move(ptr));
        Log("Default text panel created, wf=" << wf);
    }
    else if (nullptr == current_panel && def_panel != nullptr)
    {
        current_panel = def_panel;
    }

    m_windows |= ViewPanelTextEditor;
    current_panel->set_force_focused(true);
    current_panel->update(wf, node, pin, buffer, hint);
}

void GraphN_Flow::txt_panel_update(wid wf, data::NodeInstance* node)
{
    for (auto& ptr : m_panels_text_editor)
    {
        ptr->update_node_name(wf, node);
    }
}

void GraphN_Flow::txt_panel_delete(wid wf, const data::NodeInstance* node /*= nullptr*/, const data::PinInstance* pin /*= nullptr*/) noexcept
{
    for (auto& ptr : m_panels_text_editor)
    {
        ptr->reset(wf, node, pin);
    }
}

void GraphN_Flow::txt_panel_clear(wid wf) noexcept
{
    for (auto& ptr : m_panels_text_editor)
    {
        ptr->reset(wf);
    }
}

std::tuple<int32_t, int32_t, int32_t> GraphN_Flow::txt_panel_get_selection(wid wf, data::NodeInstance* node, data::PinInstance* pin) const noexcept
{
    for (auto& ptr : m_panels_text_editor)
    {
        if (ptr->is_current(wf, node, pin))
            return ptr->get_text_selection();
    }
    return {};
}

ImGuiID GraphN_Flow::find_def_txt_panel_dock() noexcept
{
    const auto* wnd = ImGui::FindWindowByName(DlgObj_SID[cast::to_szt(DlgObj::PanelTextEditor)]);
    if (wnd != nullptr)
        return wnd->DockId;
    return 0;
}

void GraphN_Flow::activate_txt_panels(wid wf)
{
    for (auto& ptr : m_panels_text_editor)
    {
        ptr->activate(wf);
    }
}

void GraphN_Flow::destroy_txt_panel(PanelTextEditor* panel) noexcept
{
    LogRAssert(panel, "Zero ptr", );
    for (auto it = m_panels_text_editor.begin(); it != m_panels_text_editor.end(); ++it)
    {
        if (it->get() == panel)
        {
            m_pending_cleanups.text_editors.push_back(panel);
            panel->reset();
            break;
        }
    }
}

void GraphN_Flow::show_about_dlg(bool autocheck)
{
    if ((m_windows & ViewAbout) == 0)
        return;

    g::block_hotkeys_mask |= ViewAbout;

    if (!m_about_dlg) [[unlikely]]
    {
        m_about_dlg = std::make_unique<AboutDlg>(autocheck);
        m_about_dlg->init();
        m_all_dialogs.insert(m_about_dlg.get());
        Log("AboutDlg initialized");
    }

    bool focused = false;
    if (m_about_dlg->show_dialog(focused) != DlgCode::Progress)
    {
        on_dlg_closed(DlgObj::AboutDlg);
        g::block_hotkeys_mask &= ~ViewAbout;
    }
}

void GraphN_Flow::open_settings_dlg(OptPane section /*= OptPane::_none*/)
{
    m_windows |= ViewOptions;
    show_options_dlg();
    m_opts_dlg->select_right_pane(section);
}

void GraphN_Flow::show_options_dlg()
{
    if ((m_windows & ViewOptions) == 0)
        return;

    g::block_hotkeys_mask |= ViewOptions;

    if (!m_opts_dlg) [[unlikely]]
    {
        m_opts_dlg = std::make_unique<OptionsDlgs>();
        m_opts_dlg->init();
        m_all_dialogs.insert(m_opts_dlg.get());
        Log("OptionsDlg initialized");
    }

    bool focused = false;
    if (m_opts_dlg->show_dialog(focused) != DlgCode::Progress)
    {
        on_dlg_closed(DlgObj::OptionsDlg);
        load_hotkeys();
        g::block_hotkeys_mask &= ~ViewOptions;
    }
}

void GraphN_Flow::show_sync_projs_dlg()
{
    if ((m_windows & ViewSyncProjects) == 0)
        return;

    g::block_hotkeys_mask |= ViewSyncProjects;

    if (!m_sync_projs_dlg) [[unlikely]]
    {
        m_sync_projs_dlg = std::make_unique<SyncProjectsDlg>();
        m_sync_projs_dlg->init();
        m_all_dialogs.insert(m_sync_projs_dlg.get());
        Log("SyncProjectsDlg initialized");
    }

    bool focused = false;
    if (m_sync_projs_dlg->show_dialog(focused) != DlgCode::Progress)
    {
        on_dlg_closed(DlgObj::SyncProjects);
        g::block_hotkeys_mask &= ~ViewSyncProjects;
    }
}

void GraphN_Flow::show_activity_bar()
{
    if ((m_windows & ViewActivityBarL) || (m_windows & ViewActivityBarR))
    {
        m_activity_bar->show(m_activity_bar_mode);
    }
}

void GraphN_Flow::show_debug_windows() noexcept
{
#ifdef _DEBUG
    if (m_windows & ViewDebugMetrics)
    {
        static bool open_debug_metrics = true;
        ImGui::ShowMetricsWindow(&open_debug_metrics);
        if (!open_debug_metrics)
            m_windows &= ~ViewDebugMetrics;
        open_debug_metrics = true;
    }
    if (m_windows & ViewDebugDemo)
    {
        static bool open_debug_demo = true;
        ImGui::ShowDemoWindow(&open_debug_demo);
        if (!open_debug_demo)
            m_windows &= ~ViewDebugDemo;
        open_debug_demo = true;
    }
    if (m_windows & ViewDebugLogs)
    {
        static bool open_debug_logs = true;
        ImGui::ShowDebugLogWindow(&open_debug_logs);
        if (!open_debug_logs)
            m_windows &= ~ViewDebugLogs;
        open_debug_logs = true;
    }
    if (m_windows & ViewStackTool)
    {
        static bool open_stack_tool = true;
        ImGui::ShowStackToolWindow(&open_stack_tool);
        if (!open_stack_tool)
            m_windows &= ~ViewStackTool;
        open_stack_tool = true;
    }
    if (m_windows & ViewStyleEditor)
    {
        ImGui::ShowStyleEditor();
    }
    if (m_windows & ViewStyleSelector)
    {
        if (!ImGui::ShowStyleSelector("DebugStyleSelector"))
            m_windows &= ~ViewStyleSelector;
    }
    if (m_windows & ViewFontSelector)
    {
        ImGui::ShowFontSelector("DebugFontSelector");
    }
    if (m_windows & ViewWfDebug)
    {
        show_wf_debug_dlg();
    }
#endif
}

void GraphN_Flow::show_modal()
{
    if (m_windows & ViewModal)
    {
        modal_show();
    }
    else if (m_windows & ViewModalStop)
    {
        modal_last_frame();
    }
}

void GraphN_Flow::show_panel(BaseDlg* panel, DlgObj obj)
{
    LogRAssert(panel, "Zero ptr", );
    bool focused = false;

    if (m_panel_appeared != ViewNone) [[unlikely]]
    {
        DlgObj appeared = DlgObj::_count;
        switch (m_panel_appeared)
        {
        case ViewPanelNodes:
            appeared = DlgObj::PanelNodes;
            break;
        case ViewPanelNodeInpector:
            appeared = DlgObj::PanelNodeInspector;
            break;
        case ViewPanelStats:
            appeared = DlgObj::PanelStats;
            break;
        case ViewPanelWfList:
            appeared = DlgObj::PanelWfList;
            break;
        case ViewPanelModels:
            appeared = DlgObj::PanelModels;
            break;
        case ViewPanelOutput:
            appeared = DlgObj::PanelServerLogs;
            break;
        }

        if (appeared != DlgObj::_count)
            panel->on_appear();
        m_panel_appeared = ViewNone;
    }

    if (panel->show_dialog(focused) != DlgCode::Progress)
    {
        on_dlg_closed(obj);
    }
}

void GraphN_Flow::show_panel_nodes()
{
    show_panel(m_panel_nodes.get(), DlgObj::PanelNodes);
}

void GraphN_Flow::show_panel_node_inspector()
{
    show_panel(m_panel_node_inspector.get(), DlgObj::PanelNodeInspector);
}

void GraphN_Flow::show_panel_stats()
{
    show_panel(m_panel_stats.get(), DlgObj::PanelStats);
}

void GraphN_Flow::show_panel_models()
{
    show_panel(m_panel_models.get(), DlgObj::PanelModels);
}

void GraphN_Flow::show_panel_queue()
{
    show_panel(m_panel_queue.get(), DlgObj::PanelQueue);
}

void GraphN_Flow::show_panel_output()
{
    show_panel(m_panel_logs.get(), DlgObj::PanelServerLogs);
}

//void GraphN_Flow::show_panel_templates()
//{
//    show_panel(m_panel_templates.get(), DlgObj::PanelTemplates);
//}

//void GraphN_Flow::show_panel_notes()
//{
//    //TODO
//    //show_panel(m_panel_notes.get(), DlgObj::PanelNotes);
//}

void GraphN_Flow::show_panel_wf_list()
{
    show_panel(m_panel_wf_list.get(), DlgObj::PanelWfList);
}

void GraphN_Flow::show_panel_text_editor()
{
    for (auto it = m_panels_text_editor.begin(); it != m_panels_text_editor.end();)
    {
        auto& panel = *it;
        ++it;

        if (panel->is_active())
        {
            bool focused;
            if (panel->show_dialog(focused) != DlgCode::Progress)
            {
                destroy_txt_panel(panel.get());
                on_dlg_closed(DlgObj::PanelTextEditor);
            }
        }
    }
}

void GraphN_Flow::show_panel_wf()
{
    for (auto it = m_panels_wf.begin(); it != m_panels_wf.end(); )
    {
        auto& panel = *it;
        ++it; // wf_panel_close might remove element
        bool focused = false;

        if (panel->show_dialog(focused) != DlgCode::Progress)
        {
            wf_panel_close(panel.get());
        }
        else if (focused && panel.get() != m_wf_focused)
        {
            bool forced = false;
            for (auto it2 = m_panels_wf.begin(); it2 != m_panels_wf.end(); ++it2)
            {
                if (!forced) forced = (*it2)->get_force_focused();
                else break;
            }

            if (!forced)
                wf_activate(panel->get_workflow()->get_id());
        }
    }
}

void GraphN_Flow::show_wf_debug_dlg()
{
#ifdef _DEBUG
    for (auto it = m_wf_debug_panels.begin(); it != m_wf_debug_panels.end(); )
    {
        auto& dlg = *it;
        if (dlg->show() != DlgCode::Progress)
        {
            it = m_wf_debug_panels.erase(it);
            on_dlg_closed(DlgObj::PanelDebugWf);
        }
        else
            ++it;
    }
#endif
}

void GraphN_Flow::handle_comfyui_runserver_error(int32_t error, std::string&& title, std::string&& description) noexcept
{
    description = fmt::format("{}\nAPI error: {}", description, error);
    modal_begin(std::move(title), std::move(description), ModalDlgCtrl::BtnOK | ModalDlgCtrl::IcoErr, ModalDlgCtrl::BtnOK, nullptr);
}

void GraphN_Flow::show_menu_bar_net_status()
{
    static fmt::memory_buffer text;

    if ((m_windows & ViewConnect) || !get_online())
    {
        const auto status = g::pConnectManager->get_status();
        if (ConnectState::Connected == status)
        {
            handle_connected();
            return;
        }
        if (ConnectState::Disconnected == status)
        {
            //text.clear();
            //fmt::format_to(std::back_inserter(text), APP_COMFYUI " disconnected");
        }
        else if (ConnectState::Connecting == status)
        {
            text.clear();
            fmt::format_to(std::back_inserter(text), FMT_STRING("Connecting to " APP_COMFYUI " on {}:{}"), g::pOptsManager->comfyui_host_load(), g::pOptsManager->comfyui_port_load());
        }
    }
    else if (m_windows & ViewParsingNodes)
    {
        if (m_nodes_parser)
        {
            size_t parsed_item = 0, parse_total = 0;
            m_nodes_parser->check_status(parsed_item, parse_total);
            text.clear();
            fmt::format_to(std::back_inserter(text), FMT_STRING("Parsing node {} of {}"), parsed_item, parse_total);

            if (m_nodes_parser->is_parse_finished())
            {
                if (m_nodes_parser->is_parse_failed())
                {
                    text.clear();
                    fmt::format_to(std::back_inserter(text), "Failed to parse loaded nodes");
                }
                else
                {
                    m_windows &= ~ViewParsingNodes;
                    m_nodes_parser.reset();
                }
            }
        }
    }
    else if (get_online()) [[likely]]
    {
        text.clear();
        fmt::format_to(std::back_inserter(text), FMT_STRING("Connected to " APP_COMFYUI " on {}:{}"), g::pOptsManager->comfyui_host_load(), g::pOptsManager->comfyui_port_load());
    }

    const float text_width = ImGui::CalcTextSize(text.data()).x;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float right_align_pos = ImGui::GetWindowWidth() - (text_width + spacing + 10.0f);
    if (right_align_pos > ImGui::GetCursorPosX())
    {
        ImGui::SetCursorPosX(right_align_pos);
    }

    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::DisconnectedMenuBarText));
    ImGui::TextUnformatted(text.data(), text.data() + text.size());
    ImGui::PopStyleColor();
}

void GraphN_Flow::show_panels()
{
    const auto* viewport = ImGui::GetMainViewport();
    //m_main_dock_id = ImGui::DockSpaceOverViewport(viewport); // , ImGuiDockNodeFlags_PassthruCentralNode);

    static bool focus_fixed = false;

    ImVec2 dock_pos = viewport->WorkPos;
    ImVec2 dock_size = viewport->WorkSize;

    if ((m_windows & ViewActivityBarL) || (m_windows & ViewActivityBarR))
    {
        dock_pos.x += m_activity_bar->get_l_width();
        dock_size.x -= m_activity_bar->get_l_width() + m_activity_bar->get_r_width();
    }
    
    ImGui::SetNextWindowPos(dock_pos);
    ImGui::SetNextWindowSize(dock_size);
    ImGui::SetNextWindowViewport(viewport->ID);

    constexpr ImGuiWindowFlags host_window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    bool open = true;
    const bool opened = ImGui::Begin("MainWorkspaceHost", &open, host_window_flags);
    ImGui::PopStyleVar(3);
    if (!opened)
        return;

    m_main_dock_id = ImGui::GetID(APP_CAPTION "_MainDockSpace");
    ImGui::DockSpace(m_main_dock_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();

    const auto id_as_name = [](std::string& id_ret, const char* panel_id)
    {
        id_ret = fmt::format(FMT_STRING("###{}"), panel_id);
        return id_ret.c_str();
    };

    if (m_windows & ViewPanelsResetAll) [[unlikely]]
    {
        focus_fixed = false;

        m_windows &= ~ViewPanelsResetAll;
        m_windows |= ViewAllPanels;
        g::pOptsManager->ui_open_wnds_save(m_windows);

        ImGui::DockBuilderRemoveNode(m_main_dock_id);
        ImGui::DockBuilderAddNode(m_main_dock_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(m_main_dock_id, viewport->WorkSize);

        ImGuiID dock_id_left_top = ImGui::DockBuilderSplitNode(m_main_dock_id, ImGuiDir_Left, 0.15f, nullptr, &m_main_dock_id);
        const ImGuiID dock_id_left_btm = ImGui::DockBuilderSplitNode(dock_id_left_top, ImGuiDir_Down, 0.3f, nullptr, &dock_id_left_top);
        ImGuiID dock_id_right_top = ImGui::DockBuilderSplitNode(m_main_dock_id, ImGuiDir_Right, 0.2f, nullptr, &m_main_dock_id);
        const ImGuiID dock_id_right_bottom = ImGui::DockBuilderSplitNode(dock_id_right_top, ImGuiDir_Down, 0.5f, nullptr, &dock_id_right_top);
        const ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(m_main_dock_id, ImGuiDir_Down, 0.25f, nullptr, &m_main_dock_id);

        std::string id;

        // left top
        ImGui::DockBuilderDockWindow(id_as_name(id, m_panel_nodes->get_panel_id()), dock_id_left_top);
        ImGui::DockBuilderDockWindow(id_as_name(id, m_panel_models->get_panel_id()), dock_id_left_top);

        // left bottom
        ImGui::DockBuilderDockWindow(id_as_name(id, m_panel_wf_list->get_panel_id()), dock_id_left_btm);
        //ImGui::DockBuilderDockWindow(id_as_name(id, m_panel_templates->get_panel_id()), dock_id_left_btm);

        // right top
        ImGui::DockBuilderDockWindow(id_as_name(id, m_panel_stats->get_panel_id()), dock_id_right_top);
        ImGui::DockBuilderDockWindow(id_as_name(id, m_panel_queue->get_panel_id()), dock_id_right_top);

        // right bottom
        //ImGui::DockBuilderDockWindow(id_as_name(id, m_panel_notes->get_panel_id()), dock_id_right_bottom);
        ImGui::DockBuilderDockWindow(id_as_name(id, m_panel_node_inspector->get_panel_id()), dock_id_right_bottom);

        // bottom
        ImGui::DockBuilderDockWindow(id_as_name(id, m_panel_logs->get_panel_id()), dock_id_bottom);
        ImGui::DockBuilderDockWindow(id_as_name(id, DlgObj_SID[cast::to_szt(DlgObj::PanelTextEditor)]), dock_id_bottom);

        for (auto& wf : m_panels_wf)
        {
            ImGui::DockBuilderDockWindow(id_as_name(id, wf->get_panel_id()), m_main_dock_id);
        }
        for (auto& editor : m_panels_text_editor)
        {
            ImGui::DockBuilderDockWindow(id_as_name(id, editor->get_panel_id()), dock_id_bottom);
        }

        ImGui::DockBuilderFinish(m_main_dock_id);
    }

    if (ImGui::GetFrameCount() > 10 && !focus_fixed) [[unlikely]]
    {
        focus_fixed = true;
        std::string id;
        ImGui::SetWindowFocus(id_as_name(id, m_panel_nodes->get_panel_id()));
        ImGui::SetWindowFocus(id_as_name(id, m_panel_wf_list->get_panel_id()));
        ImGui::SetWindowFocus(id_as_name(id, m_panel_stats->get_panel_id()));
        ImGui::SetWindowFocus(nullptr);
    }

    if (m_windows & ViewPanelNodes)
        show_panel_nodes();
    if (m_windows & ViewPanelNodeInpector)
        show_panel_node_inspector();
    if (m_windows & ViewPanelStats)
        show_panel_stats();
    if (m_windows & ViewPanelWfList)
        show_panel_wf_list();
    if (m_windows & ViewPanelModels)
        show_panel_models();
    if (m_windows & ViewPanelQueue)
        show_panel_queue();
    //if (m_windows & ViewPanelNotes)
    //    show_panel_notes();
    if (m_windows & ViewPanelOutput)
        show_panel_output();
    //if (m_windows & ViewPanelTemplates)
    //    show_panel_templates();

    if (m_windows & ViewPanelWf)
        show_panel_wf();
    if (m_windows & ViewPanelTextEditor)
        show_panel_text_editor();
}

void GraphN_Flow::modal_begin(std::string&& caption, std::string&& message, int32_t buttons, ModalDlgCtrl def, const std::function<void(ModalDlgCtrl)>& callback)
{
    LogRAssert(!m_modal_message, "Message dialog already open", );
    Log("Modal started: " << message);
    m_msg_callback = callback;
    m_modal_message = std::make_unique<ModalMessage>();
    m_modal_message->init(std::move(caption), std::move(message), buttons, def);
    m_windows |= ViewModal;
    g::block_hotkeys_mask |= ViewModal;
}

void GraphN_Flow::GraphN_Flow::modal_show()
{
    LogRAssert(m_modal_message, "Message dialog wasn't initialized", );

    const auto ret = m_modal_message->show();
    if (ret != ModalDlgCtrl::Continue)
    {
        Log("Modal replied: " << ret);
        if (m_msg_callback != nullptr)
            m_msg_callback(ret);
        modal_stop();
    }
}

void GraphN_Flow::GraphN_Flow::modal_update(std::string&& message)
{
    LogRAssert(m_modal_message, "Message dialog wasn't initialized", );
    Log("Modal updated: " << message);
    m_modal_message->update(std::move(message));
}

void GraphN_Flow::modal_stop() noexcept
{
    m_msg_callback = nullptr;
    m_windows &= ~ViewModal;
    m_windows |= ViewModalStop;
}

void GraphN_Flow::modal_last_frame()
{
    if (!m_modal_message)
    {
        m_windows &= ~(ViewModal | ViewModalStop);
        LogRError("Message dialog wasn't initialized", );
    }

    m_modal_message->stop();
    m_modal_message->show();
    m_windows &= ~(ViewModal | ViewModalStop);
    g::block_hotkeys_mask &= ~ViewModal;
    m_pending_cleanups.modal_dlg = true;
}

void GraphN_Flow::load_hotkeys()
{
    g::pOptsManager->ui_hotkeys_load();
    wf_assign_sequential_numbers();
}

uint32_t GraphN_Flow::http_get(
    HttpRequestor requestor, const StillAliveCallback& alive, const char* request, const HttpResponseCb& callback, bool retry_on_timeout /*= false*/)
{
    LogRAssert(request != nullptr && *request != 0, "Request can't be empty", 0);
    std::unique_lock<std::mutex> lock(m_http_req_mutex);
    const auto req_id = g::pConnectManager->send_http_get_request(requestor, request, retry_on_timeout);
    if (alive != nullptr && callback != nullptr)
        m_http_requests[req_id] = { .alive = alive, .callback = callback, .time = ImGui::GetTime(), .retry = retry_on_timeout, .force_drop = false };
    return req_id;
}

uint32_t GraphN_Flow::http_get(
    HttpRequestor requestor, const StillAliveCallback& alive, const char* request, httplib::Params&& params, httplib::Headers&& headers,
    const HttpResponseCb& callback, bool retry_on_timeout /*= false*/)
{
    LogRAssert(request != nullptr && *request != 0, "Request can't be empty", 0);
    std::unique_lock<std::mutex> lock(m_http_req_mutex);
    const auto req_id = g::pConnectManager->send_http_get_request(requestor, request, std::move(params), std::move(headers), retry_on_timeout);
    if (alive != nullptr && callback != nullptr)
        m_http_requests[req_id] = { .alive = alive, .callback = callback, .time = ImGui::GetTime(), .retry = retry_on_timeout, .force_drop = false };
    return req_id;
}

int32_t GraphN_Flow::http_get_and_wait(
    HttpRequestor requestor, const char* request, httplib::Params&& params, httplib::Headers&& headers, bool retry_on_timeout,
    const std::function<bool()>& stop_callback, std::string& ret_data)
{
    LogRAssert(request != nullptr && *request != 0, "Request can't be empty", 0);
    LogRAssert(stop_callback != nullptr, "Stop callback is mandatory", HTTP_RESP_FAIL);

    ret_data.clear();
    if (std::this_thread::get_id() == m_ui_thread_id) [[unlikely]]
    {
        LogError("Function must not be called from UI thread");
        return HTTP_RESP_FAIL;
    }

    const auto main_ui_alive = get_alive_callback();

    std::shared_ptr<std::atomic_int> http_status = std::make_shared<std::atomic_int>(HTTP_RESP_UNDEFINED);
    std::shared_ptr<std::string> http_data = std::make_shared<std::string>();

    const auto http_callback = [http_status, http_data](int32_t result, std::string&& binary) noexcept
    {
        if (HTTP_RESP_OK == result)
            *http_data = std::move(binary);

        http_status->store(result, std::memory_order_release);
    };

    while (main_ui_alive() && !get_online()) [[unlikely]]
    {
        if (stop_callback())
            return HTTP_RESP_FAIL;
        std::this_thread::sleep_for(100ms);
    }

    if (!main_ui_alive() || stop_callback()) [[unlikely]]
        return HTTP_RESP_FAIL;

    uint32_t req_id = 0;
    if (params.empty() && headers.empty())
        req_id = http_get(requestor, main_ui_alive, request, http_callback, retry_on_timeout);
    else
        req_id = http_get(requestor, main_ui_alive, request, std::move(params), std::move(headers), http_callback, retry_on_timeout);

    int32_t current_status = 0;
    while ((current_status = http_status->load(std::memory_order_acquire)) == HTTP_RESP_UNDEFINED)
    {
        if (!main_ui_alive() || stop_callback()) [[unlikely]]
        {
            if (main_ui_alive())
            {
                std::unique_lock<std::mutex> lock(m_http_req_mutex);
                const auto it = m_http_requests.find(req_id);
                if (it != m_http_requests.end())
                    it->second.force_drop = true;
            }
            return HTTP_RESP_FAIL;
        }

        std::this_thread::sleep_for(100ms);
    }

    if (main_ui_alive()) [[likely]]
    {
        ret_data = std::move(*http_data);
        return current_status;
    }

    return HTTP_RESP_FAIL;
}

uint32_t GraphN_Flow::http_post(HttpRequestor requestor, const StillAliveCallback& alive, const char* request, std::string&& payload, const HttpResponseCb& callback, bool retry_on_timeout /*= false*/)
{
    std::unique_lock<std::mutex> lock(m_http_req_mutex);
    const auto req_id = g::pConnectManager->send_http_post_request(requestor, request, std::move(payload), retry_on_timeout);
    if (alive != nullptr && callback != nullptr)
        m_http_requests[req_id] = { .alive = alive, .callback = callback, .time = ImGui::GetTime(), .retry = retry_on_timeout, .force_drop = false };
    return req_id;
}

uint32_t GraphN_Flow::http_post(HttpRequestor requestor, const StillAliveCallback& alive, const char* request, httplib::UploadFormDataItems&& payload, const HttpResponseCb& callback, bool retry_on_timeout /*= false*/)
{
    std::unique_lock<std::mutex> lock(m_http_req_mutex);
    const auto req_id = g::pConnectManager->send_http_post_request(requestor, request, std::move(payload), retry_on_timeout);
    if (alive != nullptr && callback != nullptr)
        m_http_requests[req_id] = { .alive = alive, .callback = callback, .time = ImGui::GetTime(), .retry = retry_on_timeout, .force_drop = false };
    return req_id;
}

void GraphN_Flow::post_stats_event(
    HttpRequestor requestor, 
    int64_t session_id, 
    const char* ev_type, uint64_t ev_time,
    const char* prop_type, nlohmann::json&& event_props,
    bool callback_needed, mngr::StatsData::Progress* progress /*= nullptr*/)
{
    LogRAssert(ev_type, "Zero ptr", );

    if (m_stats.customer_id.empty())
        m_stats.customer_id = g::pDbManager->read_db_metadata().customer_id;

    if (session_id != 0)
        m_stats.session_id = session_id;
    if (0 == m_stats.session_id)
        m_stats.session_id = tools::dt::from_current_date_utc().time();

    nlohmann::json j;
    j["api_key"] = STATS_API_KEY;

    nlohmann::json event;
    event["event"] = ev_type; 

    nlohmann::json props;
    props["distinct_id"] = m_stats.customer_id;
    props["session_id"] = m_stats.session_id;
    props["app_version"] = ver::ver_as_string();

#if defined _WIN32
    props["os_name"] = "Windows";
#elif defined __linux__
    props["os_name"] = "Linux";
#elif defined __APPLE__
    props["os_name"] = "MacOS";
#endif

    if (prop_type != nullptr)
    {
        if (std::string_view(prop_type) == "user_properties")
        {
            props["$set"] = std::move(event_props);
        }
        else
        {
            props.update(event_props);
        }
    }

    event["properties"] = std::move(props);

    //if (0 == ev_time)
    //    ev_time = tools::dt::from_current_date_utc().time();
    if (ev_time != 0)
    {
        const auto time = tools::dt::from_time_utc(ev_time).as_iso8601();
        event["timestamp"] = time;
    }
    //event["timestamp"] = time;

    j["batch"] = nlohmann::json::array({ std::move(event) });

    const auto callback = [this, progress, ev_type](int32_t result, std::string&& payload)
    {
        if (progress != nullptr)
        {
            if (HTTP_RESP_OK == result)
                progress->sent = true;
            else
            {
                progress->in_progress = progress->sent = false;
                progress->next_attempt = g::pMain->get_seconds_elapsed() + 60 * 5;
            }
        }

        LogS("Session#" << m_stats.session_id << ":" << ev_type << ". res " << result << ", payload sz " << payload.size());
    };

    auto payload = j.dump();
    if (progress != nullptr) progress->in_progress = true;
    std::unique_lock<std::mutex> lock(m_http_req_mutex);

    const auto req_id = g::pConnectManager->send_http_post_request_ext(requestor, STATS_SERVER, STATS_PORT, STATS_URL, std::move(payload), true);
    if (callback_needed)
        m_http_requests[req_id] = { .alive = get_alive_callback(), .callback = callback, .time = ImGui::GetTime(), .retry = false, .force_drop = false };
}

void GraphN_Flow::stats_post_session_start()
{
    if (!g::pOptsManager->app_metrics_load())
        return;

    g::pDbManager->session_nodes_load(m_stats.invalid_definition, m_stats.unrecognized_class, m_stats.unrecognized_definition, m_stats.python_modules);
    post_stats_event(HttpRequestor::Ext_Stat_Session, 0 /*session_id*/, "session", 0 /*ev_time*/, nullptr /*prop_type*/, {} /*event_props*/, true /*callback_needed*/, &m_stats.app_start_sent);
}

void GraphN_Flow::stats_save_session_end()
{
    g::pDbManager->session_end_save(m_stats.session_id, std::to_string(g::pMain->get_seconds_elapsed()));
}

void GraphN_Flow::stats_post_session_end()
{
    if (!g::pOptsManager->app_metrics_load())
        return;

    const auto meta = g::pDbManager->read_db_metadata();
    const auto duration = std::strtoll(meta.last_session_duration.c_str(), nullptr, 10);
    if (0 == duration)
    {
        m_stats.app_end_sent.sent = true;
        return;
    }

    u_json prop;
    prop["duration"] = duration;

    //const auto session_time = (0 == duration ? 0 : meta.last_session_id + duration);
    post_stats_event(HttpRequestor::Ext_Stat_Session, meta.last_session_id, "session", 0 /*session_time*/, "event_properties", std::move(prop), true /*callback_needed*/, &m_stats.app_end_sent);
}

void GraphN_Flow::stats_post_monitor_data()
{
    if (!g::pOptsManager->app_metrics_load())
        return;

    if (0 == g::pMain->get_monitor_frequency() || 0 == g::pMain->get_monitor_x())
        return;

    u_json prop;
    prop["monitor_frequency"] = std::to_string(g::pMain->get_monitor_frequency());
    prop["monitor_resolution"] = fmt::format(FMT_STRING("{}x{}"), g::pMain->get_monitor_x(), g::pMain->get_monitor_y());
    prop["monitor_dpi_scale"] = std::to_string(g::pMain->get_monitor_dpi());

    post_stats_event(HttpRequestor::Ext_Stat_Hardware, 0 /*session_id*/, "hardware", 0 /*ev_time*/, "event_properties", std::move(prop), true /*callback_needed*/, &m_stats.monitor_sent);
}

void GraphN_Flow::stats_post_user_data()
{
    if (!g::pOptsManager->app_metrics_load())
        return;

    std::string ram, vram;
    if (!m_panel_stats || !m_panel_stats->collect_metrics(m_stats.comfyui_ver, ram, vram))
        return;

    const auto host = g::pOptsManager->comfyui_host_load();
    const auto type = (host.starts_with("127.") || host == "localhost" ? "local" : "remote");
    u_json comfyui_prop;
    comfyui_prop["comfyui_ver"] = m_stats.comfyui_ver;
    comfyui_prop["comfyui_type"] = type;
    post_stats_event(HttpRequestor::Ext_Stat_Comfyui, 0 /*session_id*/, "comfyui", 0 /*ev_time*/, "event_properties", std::move(comfyui_prop), true /*callback_needed*/, &m_stats.user_sent);

    u_json hardware_prop;
    hardware_prop["ram"] = ram;
    hardware_prop["vram"] = vram;
    post_stats_event(HttpRequestor::Ext_Stat_Hardware, 0 /*session_id*/, "hardware", 0 /*ev_time*/, "event_properties", std::move(hardware_prop), true /*callback_needed*/, &m_stats.user_sent);
}

void GraphN_Flow::stats_post_nodes(
    std::unordered_set<std::string>&& invalid_definition,
    std::unordered_set<std::string>&& unrecognized_class,
    std::unordered_set<std::string>&& unrecognized_definition,
    std::unordered_set<std::string>&& python_modules)
{
    if (!g::pOptsManager->app_metrics_load())
        return;

    std::erase_if(invalid_definition, [&saved = m_stats.invalid_definition](const std::string& node) { return saved.contains(node); });
    std::erase_if(unrecognized_class, [&saved = m_stats.unrecognized_class](const std::string& node) { return saved.contains(node); });
    std::erase_if(unrecognized_definition, [&saved = m_stats.unrecognized_definition](const std::string& node) { return saved.contains(node); });
    std::erase_if(python_modules, [&saved = m_stats.python_modules](const std::string& module) { return saved.contains(module); });
    if (invalid_definition.empty() && unrecognized_class.empty() && unrecognized_definition.empty() && python_modules.empty())
        return;

    u_json prop;
    prop["invalid_definition"] = invalid_definition;
    prop["unrecognized_class"] = unrecognized_class;
    prop["unrecognized_definition"] = unrecognized_definition;
    prop["python_modules"] = python_modules;
    post_stats_event(HttpRequestor::Ext_Stat_Nodes, 0 /*session_id*/, "nodes", 0 /*ev_time*/, "event_properties", std::move(prop), false /*callback_needed*/, nullptr /*progress*/);

    m_stats.invalid_definition.merge(invalid_definition);
    m_stats.unrecognized_class.merge(unrecognized_class);
    m_stats.unrecognized_definition.merge(unrecognized_definition);
    m_stats.python_modules.merge(python_modules);
    g::pDbManager->session_nodes_save(m_stats.invalid_definition, m_stats.unrecognized_class, m_stats.unrecognized_definition, m_stats.python_modules);
}

void GraphN_Flow::stats_post_host(HostType host_type)
{
    if (!g::pOptsManager->app_metrics_load())
        return;

    u_json prop;
    switch (host_type)
    {
    case HostType::MaskEditor:
        prop["type"] = "mask_editor";
        break;
    case HostType::PosEditor:
        prop["type"] = "3dpos_editor";
        break;
    default:
        LogRError("Unknown host type", );
    }
    post_stats_event(HttpRequestor::Ext_Stat_Host, 0 /*session_id*/, "host", 0 /*ev_time*/, "event_properties", std::move(prop), false /*callback_needed*/, nullptr /*progress*/);
}

void GraphN_Flow::stats_post_config(const char* property, const char* value)
{
    if (!g::pOptsManager->app_metrics_load())
        return;

    u_json prop;
    prop[property] = value;

    post_stats_event(HttpRequestor::Ext_Stat_Config, 0 /*session_id*/, "config", 0 /*ev_time*/, "event_properties", std::move(prop), false /*callback_needed*/, nullptr /*progress*/);
}

void GraphN_Flow::check_for_updates(const std::function<void(bool /*http_result*/, bool /*upd_found*/, const std::string& /*version*/, const std::string& /*date*/)>& callback)
{
    if (m_upd_checked)
    {
        if (callback != nullptr)
            callback(true, m_upd_found, m_upd_version, m_upd_date);
        return;
    }

    m_last_update_check = -1; // pause auto check

    const auto ver_callback = [this, callback](int32_t result, std::string&& data)
    {
        m_last_update_check = g::pMain->get_seconds_elapsed();

        if (result != HTTP_RESP_OK || data.empty())
        {
            m_upd_checked = false;
            m_upd_found = false;
            m_upd_version.clear();
            m_upd_date.clear();
        }
        else
        {
            try
            {
                const auto j = u_json::parse(data);
                if (j.contains("tag_name") && j["tag_name"].is_string() && j.contains("published_at") && j["published_at"].is_string())
                {
                    const auto ver_or = j["tag_name"].get<std::string>();
                    const auto version = ver::from_string(ver_or);
                    const auto date_or = j["published_at"].get<std::string>();
                    const std::string date = date_or.length() >= 10 ? date_or.substr(0, 10) : date_or;
                    const bool is_new_ver = ver::is_greater(version);
                    LogS("Version check: " << ver_or << ", " << date_or << ", is new: " << is_new_ver);

                    m_upd_checked = true;
                    m_upd_found = is_new_ver;
                    m_upd_version = ver_or;
                    m_upd_date = date;
                }
            }
            catch(...)
            {
                LogExcepS("Failed to parse version: " << data);
            }
        }

        if (callback != nullptr)
            callback(HTTP_RESP_OK == result, m_upd_found, m_upd_version, m_upd_date);
    };

    const auto req_id = g::pConnectManager->send_http_get_request_ext(HttpRequestor::Ext_Update, UPDATE_SERVER, UPDATE_PORT, UPDATE_URL, true);
    m_http_requests[req_id] = { .alive = get_alive_callback(), .callback = ver_callback, .time = ImGui::GetTime(), .retry = false, .force_drop = false };
}

bool GraphN_Flow::get_updates_info(std::string& upd_version, std::string& upd_date) const
{
    if (!m_upd_found)
        return false;

    upd_version = m_upd_version;
    upd_date = m_upd_date;
    return true;
}

void GraphN_Flow::on_1s_lazy_timer(int64_t total_seconds_elapsed)
{
    if (g::pOptsManager->app_metrics_load())
    {
        if (!m_stats.all_sent())
        {
            if (total_seconds_elapsed > m_stats.app_end_sent.next_attempt && !m_stats.app_end_sent.in_progress && !m_stats.app_end_sent.sent)
                stats_post_session_end();
            if (total_seconds_elapsed > m_stats.app_start_sent.next_attempt && !m_stats.app_start_sent.in_progress && !m_stats.app_start_sent.sent)
                stats_post_session_start();
            if (total_seconds_elapsed > m_stats.monitor_sent.next_attempt && !m_stats.monitor_sent.in_progress && !m_stats.monitor_sent.sent)
                stats_post_monitor_data();
        }

        if (!m_stats.user_sent.in_progress && !m_stats.user_sent.sent && get_online())
            stats_post_user_data();
    }

    if (!m_upd_found && m_last_update_check >= 0 && g::pOptsManager->app_upd_check_load())
    {
        // next check in 24h or next app restart
        if (total_seconds_elapsed > FIRST_UPD_ATTEMPT && (0 == m_last_update_check || total_seconds_elapsed - m_last_update_check >= 3600 * 24))
        {
            check_for_updates(nullptr);
        }
    }
}

void GraphN_Flow::check_http_reply()
{
    if (get_is_quit()) [[unlikely]]
        return;

    bool drop_all = false;
    if (!get_online())
    {
        g::pConnectManager->drop_all_messages();
        drop_all = true;
    }

    constexpr auto max_processing_time = 0.003; // max 3ms per frame
    const auto start_time = ImGui::GetTime();

    uint32_t rid = 0;
    int32_t result = 0;
    std::string response;

    while (g::pConnectManager->pickup_http_response(rid, result, response))
    {
        std::unique_lock<std::mutex> lock(m_http_req_mutex);
        const auto it = m_http_requests.find(rid);
        if (it != m_http_requests.end())
        {
            const auto alive = it->second.alive;
            const auto callback = it->second.callback;
            const bool drop = it->second.force_drop;
            m_http_requests.erase(it);
            lock.unlock();

            if (!drop && alive && callback && alive())
                callback(result, std::move(response));
            response.clear();
        }

        if ((ImGui::GetTime() - start_time) > max_processing_time)
            break;
    }

    static double last_dead_http_check = ImGui::GetTime();
    const double now = ImGui::GetTime();
    if (drop_all || now - last_dead_http_check > HTTP_REQUEST_LIFETIME / 2.0) [[unlikely]]
    {
        std::unique_lock<std::mutex> lock(m_http_req_mutex);
        for (auto it = m_http_requests.begin(); it != m_http_requests.end(); )
        {
            if (!drop_all && it->second.force_drop)
            {
                it = m_http_requests.erase(it);
                continue;
            }
            if (!drop_all && it->second.retry)
            {
                ++it;
                continue;
            }

            if (drop_all || now - it->second.time > HTTP_REQUEST_LIFETIME)
            {
                if (drop_all)   Log("HTTP-Req#" << it->first << ". Force timeout as server disconnected");
                else            Log("HTTP-Req#" << it->first << ". Force timeout after " << HTTP_REQUEST_LIFETIME << " seconds");

                const auto alive = it->second.alive;
                const auto callback = it->second.callback;
                if (alive && callback && alive())
                    callback(HTTP_RESP_TIMEOUT, "");

                it = m_http_requests.erase(it);
            }
            else
                ++it;
        }
        last_dead_http_check = now;
    }
}

void GraphN_Flow::check_ws_message(const std::function<void(bool, std::string&)>& callback)
{
    if (get_is_quit()) [[unlikely]]
        return;

    if (!get_online())
    {
        g::pConnectManager->drop_all_messages();
        return;
    }

    constexpr auto max_processing_time = 0.003; // max 3ms per frame
    const auto start_time = ImGui::GetTime();

    bool is_binary;
    std::string msg;

    while (g::pConnectManager->pickup_ws_message(is_binary, msg))        
    {
        callback(is_binary, msg);
        if ((ImGui::GetTime() - start_time) > max_processing_time)
            break;
    }
}

namespace {
void host_open(CustomMediaWidget* wdgt, wid wf_id, iid node_id, HostType type)
{
    auto wf = g::pWfsManager->access(wf_id);
    LogRAssertS(wf, "Wf not found. w:" << wf_id, );

    auto node = wf->get_node_by_instance(node_id);
    LogRAssertS(wf, "Wf not found. w:" << wf_id << ", n:" << node_id, );

    std::filesystem::path source_path, dest_path, server_name;
    if (!wdgt->get_current_image_paths(node, source_path, server_name))
        return;

    dest_path = fs::get_temp_directory_path();
    dest_path /= server_name.stem();
    if (HostType::MaskEditor == type)
        dest_path += "_mask";
    else
        dest_path += "_3dpos";
    dest_path += server_name.extension();
    if (!tools::path::make_file(dest_path, true))
    {
        dest_path = tools::path::find_next_name(dest_path);
        if (!tools::path::make_file(dest_path, true))
        {
            LogWarnS("Can't create dest image file");
            return;
        }
    }

    if (HostType::MaskEditor == type)
        node->flags().add(NodeInstFlags::MaskEditorRunning);
    else
        node->flags().add(NodeInstFlags::PosEditorRunning);

    g::pHostManager->add_host(wf_id, node_id, type, std::move(source_path), std::move(dest_path), node->obj_flags().has(NodeObjFlags::CustomMediaFromSourceIn));
}
} // namespace

void GraphN_Flow::host_open_mask_editor(wid wf_id, iid node_id)
{
    stats_post_host(HostType::MaskEditor);
    host_open(m_custom_media_widget.get(), wf_id, node_id, HostType::MaskEditor);
}

void GraphN_Flow::host_open_3dpos_editor(wid wf_id, iid node_id)
{
    stats_post_host(HostType::PosEditor);
    host_open(m_custom_media_widget.get(), wf_id, node_id, HostType::PosEditor);
}

void GraphN_Flow::host_handle_node_removal(wid wf_id, iid node_id)
{
    g::pHostManager->kill_host(wf_id, node_id);
}

void GraphN_Flow::host_handle_wf_closure(wid wf_id)
{
    g::pHostManager->kill_hosts(wf_id);
}

void GraphN_Flow::host_handle_renaming(wid wf_id, iid node_id)
{
    auto wf = g::pWfsManager->access(wf_id);
    if (node_id != 0)
    {
        auto node = wf->get_node_by_instance(node_id);
        g::pHostManager->rename_host(wf_id, node_id, wf->get_data().name, node->get_name());
    }
    else
        g::pHostManager->rename_host(wf_id, node_id, wf->get_data().name, "");
}

void GraphN_Flow::check_host_reply()
{
    if (get_is_quit()) [[unlikely]]
        return;

    wid wf_id = 0;
    iid node_id = 0;
    bool result = false;
    HostType type;
    std::filesystem::path image_path;
    if (!g::pHostManager->pickup_finished_host(wf_id, node_id, result, image_path, type)) [[likely]]
        return;

    Log("Host reply to w:" << wf_id << ", n:" << node_id << ", res:" << result << ", img:" << image_path);

    data::Workflow* wf = g::pWfsManager->access(wf_id, true); // can be zero if already closed
    if (nullptr == wf)
        return;
    data::NodeInstance* node = wf->get_node_by_instance(node_id); // can be zero if already closed
    bool is_output_node = false;
    if (node != nullptr)
    {
        node->flags().remove(NodeInstFlags::MaskEditorRunning, NodeInstFlags::PosEditorRunning);
        is_output_node = node->obj_flags().has(NodeObjFlags::CustomMediaFromOutput);
    }

    const auto callback = [this, wf_id, node_id, is_output_node, image_path = tools::path::path_to_string(image_path)]
            (const std::string& image_name, const std::string& image_dir)
    {
        if (is_output_node)
        {
            g::pFlow->custom_media_widgets().add_output(wf_id, node_id, mngr::ExecTask::OutputType::Image, image_path, "", 0);
        }

        m_custom_js_node->handle_edited_image(wf_id, node_id, image_name, image_dir);
    };

    if (result)
        m_custom_media_widget->upload_edited_image(node, std::move(image_path), true, callback);
}

}

