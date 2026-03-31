
#include "ui/graph-n_flow.h"
#include "ui/base_dlg.h"
#include "main.h"
#include "ui/options/opts_dlg.h"
#include "ui/modal_message.h"
#include "ui/panels/activity_bar.h"
#include "ui/panels/panel_nodes.h"
#include "ui/panels/panel_node_inspector.h"
#include "ui/panels/panel_stats.h"
#include "ui/panels/panel_wf.h"
#include "ui/panels/panel_text_editor.h"
#include "ui/panels/panel_wf_list.h"
#include "ui/panels/panel_models.h"
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
#include "core/workflow/workflow.h"
#include "core/workflow/workflow_action.h"

#ifdef _DEBUG
static bool test_json = false;
#endif

namespace w
{
float pin_l_offset_ro = 0.0f;
float pin_l_offset_rw = 0.0f;
float pin_r_offset = 0.0f;
float ico_circle = 0.0f;
float ico_eye = 0.0f;
float ico_max = 0.0f;
float v_spacing = 0.0f;
std::string hidden_widgets = "Hidden widgets";
float hidden_widgets_len;
}

namespace ui
{

GraphN_Flow::GraphN_Flow()
{
    LogF;
}

GraphN_Flow::~GraphN_Flow()
{
    LogF;
}

bool GraphN_Flow::init()
{
    LogF;

    w::pin_l_offset_ro = 4.0f;
    w::pin_l_offset_rw = 0.0f;
    w::pin_r_offset = 4.0f;
    w::ico_circle = ImGui::CalcTextSize(ICON_LC_CIRCLE_SMALL).x;
    w::ico_eye = ImGui::CalcTextSize(ICON_LC_EYE).x;
    w::ico_max = std::max(w::ico_circle, w::ico_eye);
    w::v_spacing = 4.0f;
    w::hidden_widgets_len = ImGui::CalcTextSize(w::hidden_widgets.c_str()).x;

    // menu colors
    ImGuiStyle& style = ImGui::GetStyle();

    // Background: Dark Grey (Opaque) - Lighter than main window
    style.Colors[ImGuiCol_PopupBg] = ImColor(g::pOptsManager->color_load(AppColor::PopupBg));
    style.Colors[ImGuiCol_MenuBarBg] = ImColor(g::pOptsManager->color_load(AppColor::MenubarBg));

    // open windows
    m_windows = g::pOptsManager->ui_open_wnds_load();

    // create panels
    m_panel_node_inspector = std::make_unique<ui::PanelNodeInspector>();
    m_panel_node_inspector->init();
    m_all_dialogs.insert(m_panel_node_inspector.get());

    m_panel_nodes = std::make_unique<ui::PanelNodes>(m_panel_node_inspector.get());
    m_panel_nodes->init();
    m_all_dialogs.insert(m_panel_nodes.get());

    m_panel_stats = std::make_unique<ui::PanelStats>();
    m_panel_stats->init();
    m_all_dialogs.insert(m_panel_stats.get());

    m_panel_wf_list = std::make_unique<ui::PanelWfList>();
    m_panel_wf_list->init();
    m_all_dialogs.insert(m_panel_wf_list.get());

    m_panel_templates = std::make_unique<ui::PanelTemplates>();
    m_panel_templates->init();
    m_all_dialogs.insert(m_panel_templates.get());

    m_panel_logs = std::make_unique<ui::PanelServerLogs>();
    m_panel_logs->init();
    m_all_dialogs.insert(m_panel_logs.get());

    //m_panel_notes = std::make_unique<ui::PanelNotes>();
    //m_panel_notes->init();
    //m_all_dialogs.insert(m_panel_notes.get());

    m_panel_models = std::make_unique<ui::PanelModels>();
    m_panel_models->init();
    m_all_dialogs.insert(m_panel_models.get());

  //  init_workflow_panels();

    // activity bar
    m_activity_bar = std::make_unique<ui::ActivityBar>();
    load_activity_bar_mode();
    m_activity_bar->register_panels({ // except PanelServerLogs
        DlgObj::PanelNodes,
        DlgObj::PanelModels,
        DlgObj::PanelNodeInspector,
        DlgObj::PanelWfList,
        DlgObj::PanelTemplates,
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

    return true;
}

void GraphN_Flow::reconnect()
{
    LogF;
    m_windows |= ViewConnect;
    show_logs_panel(true);
    g::pConnectManager->reconnect();
}

void GraphN_Flow::load_activity_bar_mode()
{
    m_activity_bar_mode = g::pOptsManager->ui_activity_bar_mode_load();
    if (ActivityBarMode::Smart == m_activity_bar_mode || ActivityBarMode::Left == m_activity_bar_mode)
    {
        m_windows |= ViewActivityBarL;
        m_windows &= ~ViewActivityBarR;
    }
    else if (ActivityBarMode::Right == m_activity_bar_mode)
    {
        m_windows |= ViewActivityBarR;
        m_windows &= ~ViewActivityBarL;
    }
    else
    {
        m_windows &= ~(ViewActivityBarL | ViewActivityBarR);
    }
}

bool GraphN_Flow::is_panel_active(DlgObj panel_obj) const
{
    switch (panel_obj)
    {
    case DlgObj::PanelNodes:
        return (m_windows & ViewPanelNodes) != 0;
    case DlgObj::PanelModels:
        return (m_windows & ViewPanelModels) != 0;
    case DlgObj::PanelNodeInspector:
        return (m_windows & ViewPanelNodeInpector) != 0;
    case DlgObj::PanelWfList:
        return (m_windows & ViewPanelWfList) != 0;
    case DlgObj::PanelTemplates:
        return (m_windows & ViewPanelTemplates) != 0;
    case DlgObj::PanelStats:
        return (m_windows & ViewPanelStats) != 0;
    case DlgObj::PanelServerLogs:
        return (m_windows & ViewPanelOutput) != 0;
    case DlgObj::PanelNotes:
        return (m_windows & ViewPanelNotes) != 0;
    case DlgObj::PanelWf:
        return (m_windows & ViewPanelWf) != 0;
    case DlgObj::PanelTextEditor:
        return (m_windows & ViewPanelTextEditor) != 0;
    case DlgObj::PanelMarkdownPreview:
        return (m_windows & ViewPanelMarkdownPreview) != 0;
    }
    LogInternalLogic("Unhandled panel id " << cast::to_i32(panel_obj));
    return false;
}

const char* GraphN_Flow::get_panel_icon(DlgObj panel_obj) const
{
    switch (panel_obj)
    {
    case DlgObj::PanelNodes:
        return ICON_LC_BOOK_OPEN;
    case DlgObj::PanelModels:
        return ICON_LC_COMPONENT;
    case DlgObj::PanelNodeInspector:
        return ICON_LC_TANGENT;
    case DlgObj::PanelWfList:
        return ICON_LC_LIBRARY;
    case DlgObj::PanelWf:
        return ICON_LC_WORKFLOW;
    case DlgObj::PanelTemplates:
        return ICON_LC_LAYOUT_TEMPLATE;
    case DlgObj::PanelStats:
        return ICON_LC_CHART_LINE;
    case DlgObj::PanelServerLogs:
        return ICON_LC_TERMINAL;
    case DlgObj::PanelTextEditor:
        return ICON_LC_TEXT_INITIAL;
    case DlgObj::PanelMarkdownPreview:
        return ICON_LC_HEADING_1;
    case DlgObj::PanelNotes:
        return ICON_LC_SCROLL_TEXT;
    }
    LogInternalLogic("Unhandled panel id " << cast::to_i32(panel_obj));
    return false;
}

const char* GraphN_Flow::get_panel_name(DlgObj panel_obj) const
{
    switch (panel_obj)
    {
    case DlgObj::PanelNodes:
        return m_panel_nodes->get_panel_name();
    case DlgObj::PanelModels:
        return m_panel_models->get_panel_name();
    case DlgObj::PanelNodeInspector:
        return m_panel_node_inspector->get_panel_name();
    case DlgObj::PanelWfList:
        return m_panel_wf_list->get_panel_name();
    case DlgObj::PanelTemplates:
        return m_panel_templates->get_panel_name();
    case DlgObj::PanelStats:
        return m_panel_stats->get_panel_name();
    case DlgObj::PanelServerLogs:
        return m_panel_logs->get_panel_name();
//    case DlgObj::PanelMarkdownPreview:
//        return m_panel_nodes->get_panel_name();
//    case DlgObj::PanelNotes:
//        return m_panel_notes->get_panel_name();
    }
    LogInternalLogic("Unhandled panel id " << cast::to_i32(panel_obj));
    return nullptr;
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
    case DlgObj::PanelNodeInspector:
        panel_view = ViewPanelNodeInpector;
        break;
    case DlgObj::PanelWfList:
        panel_view = ViewPanelWfList;
        break;
    case DlgObj::PanelTemplates:
        panel_view = ViewPanelTemplates;
        break;
    case DlgObj::PanelStats:
        panel_view = ViewPanelStats;
        break;
    case DlgObj::PanelServerLogs:
        panel_view = ViewPanelOutput;
        break;
    case DlgObj::PanelMarkdownPreview:
        panel_view = ViewPanelMarkdownPreview;
        break;
    case DlgObj::PanelNotes:
        panel_view = ViewPanelNotes;
        break;
    default:
        {
            LogInternalLogic("Unhandled panel id " << cast::to_i32(panel_obj));
        }
    }

    if ((m_windows & panel_view) != 0)
        m_windows &= ~panel_view;
    else
        m_windows |= panel_view;
}

void GraphN_Flow::exit()
{
    LogF;
    prepare_to_quit();
}

void GraphN_Flow::stop_all_threads()
{
    LogF;
}

bool GraphN_Flow::run()
{
    show_menu_bar();
    show_activity_bar();
    show_options_dlg();
    show_debug_windows();
    show_panels();
    show_modal();

    check_connection();
    return true;
}

void GraphN_Flow::prepare_to_quit()
{
    LogF_In;

    g::pWfsManager->close_and_purge();

    for (auto& dlg : m_all_dialogs)
    {
        if (dlg)
        {
            dlg->stop_all_threads();
            dlg->free();
        }
    }
    m_all_dialogs.clear();
    m_panels_wf.clear();
    m_wf_focused = nullptr;

    if (m_nodes_parser)
        m_nodes_parser->stop();
    m_nodes_parser.reset();

    stop_all_threads();

    LogF_Out;
}

void GraphN_Flow::show_logs_panel(bool show)
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
    if (!get_online())
    {
        reconnect();
    }
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
    const auto parser = [this](bool result, std::string&& data)
    {
        if (!result)
        {
            Log("Failed to load object_info");
            return;
        }

        m_windows |= ViewParsingNodes;
        m_nodes_ready = false;
        m_models_ready = false;

        g::pModelsManager->load_models();

        m_nodes_parser = std::make_unique<mngr::NodesParser>();
#ifdef _DEBUG
        if (test_json)
        {
            auto test_json_path = fs::get_root_directory_path();
            test_json_path /= "test_json_broken.json";
            auto test_json_data = tools::path::read_file(test_json_path);
            std::string test_json_string;
            test_json_string.reserve(test_json_data.size() + 1);
            test_json_string = test_json_data.data();
            test_json_string.at(test_json_data.size()) = 0;
            m_nodes_parser->parse(std::move(test_json_string));
        }
        else
#endif
            m_nodes_parser->parse(std::move(data));
    };

    g::pConnectManager->send_http_request(comfyui_http::object_info, false, parser);
}

void GraphN_Flow::show_menu_bar()
{
    const bool connecting_mode = (m_windows & ViewConnect);
    if (connecting_mode)
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, g::pOptsManager->color_load(AppColor::DisconnectedMenuBarBg));

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File##MainMenuBar"))
        {
            if (ImGui::MenuItem("Import Project from file...##FileMenuBar"))
            {
                Log("Menu: Import Project");
                on_menu_import_json();
            }
            if (ImGui::MenuItem("Export Project to file...##FileMenuBar"))
            {
                Log("Menu: Export Project");
                on_menu_export_json();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Options##FileMenuBar"))
            {
                Log("Menu: Options");
                on_menu_options();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit " APP_CAPTION "##FileMenuBar", "Ctrl+Q"))
            {
                Log("Menu: Quit");
                on_menu_quit();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Projects##MainMenuBar"))
        {
            if (ImGui::MenuItem("Add new Project##WfMenuBar"))
            {
                Log("Menu: new wf");
                on_menu_wf(nullptr);
            }
            ImGui::Separator();
            for (auto& wf : m_panels_wf)
            {
                const auto wf_name = fmt::format(FMT_STRING("{}##PanelsMenuBar"), wf->get_panel_name_full());
                if (ImGui::MenuItem(wf_name.c_str(), nullptr, wf.get() == m_wf_focused))
                {
                    Log("Menu: wf " << wf_name);
                    on_menu_wf(wf.get());
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Panels##MainMenuBar"))
        {
            if (ImGui::MenuItem("Nodes Library##PanelsMenuBar", nullptr, (m_windows & ViewPanelNodes)))
            {
                Log("Menu: Nodes Library");
                on_menu_panel_window(ViewPanelNodes);
            }
            if (ImGui::MenuItem("Models Library##PanelsMenuBar", nullptr, (m_windows & ViewPanelModels)))
            {
                Log("Menu: models panel");
                on_menu_panel_window(ViewPanelModels);
            }
            if (ImGui::MenuItem("Node Inspector##PanelsMenuBar", nullptr, (m_windows & ViewPanelNodeInpector)))
            {
                Log("Menu: Node Inspector");
                on_menu_panel_window(ViewPanelNodeInpector);
            }
            if (ImGui::MenuItem("My Projects##PanelsMenuBar", nullptr, (m_windows & ViewPanelWfList)))
            {
                Log("Menu: wf list panel");
                on_menu_panel_window(ViewPanelWfList);
            }
            if (ImGui::MenuItem("Stats##PanelsMenuBar", nullptr, (m_windows & ViewPanelStats)))
            {
                Log("Menu: stats panel");
                on_menu_panel_window(ViewPanelStats);
            }
            if (ImGui::MenuItem("Notes##PanelsMenuBar", nullptr, (m_windows & ViewPanelNotes)))
            {
                Log("Menu: notes panel");
                on_menu_panel_window(ViewPanelNotes);
            }
            if (ImGui::MenuItem("Templates##PanelsMenuBar", nullptr, (m_windows & ViewPanelTemplates)))
            {
                Log("Menu: templates panel");
                on_menu_panel_window(ViewPanelTemplates);
            }
            if (ImGui::MenuItem("Server Log##PanelsMenuBar", nullptr, (m_windows & ViewPanelOutput)))
            {
                Log("Menu: output panel");
                on_menu_panel_window(ViewPanelOutput);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Reset default layout##PanelsMenuBar"))
            {
                Log("Menu: Reset default layout");
                on_menu_panel_window(ViewPanelsResetAll);
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
                Log("Menu: About");
                on_menu_about();
            }
#ifdef _DEBUG
            if (ImGui::MenuItem("Use test json##HelpMenuBar", nullptr, test_json))
            {
                test_json = !test_json;
            }
#endif
            ImGui::EndMenu();
        }

        show_menu_bar_net_status();

        ImGui::EndMainMenuBar();
    }

    if (connecting_mode)
        ImGui::PopStyleColor();
}

void GraphN_Flow::on_dlg_closed(DlgObj dlg)
{
    switch (dlg)
    {
    case DlgObj::OptionsDlg:
        m_windows &= ~ViewOptions;
        m_all_dialogs.erase(m_opts_dlg.get());
        m_opts_dlg.reset();
        Log("Options dlg closed");
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
    case DlgObj::PanelTemplates:
        m_windows &= ~ViewPanelTemplates;
        Log("Templates panel closed");
        break;
    case DlgObj::PanelServerLogs:
        m_windows &= ~ViewPanelOutput;
        Log("Output panel closed");
        break;
    case DlgObj::PanelNotes:
        m_windows &= ~ViewPanelNotes;
        Log("Notes panel closed");
        break;
    case DlgObj::PanelModels:
        m_windows &= ~ViewPanelModels;
        Log("Models panel closed");
        break;
    case DlgObj::ActivityBarL:
        m_windows &= ~ViewActivityBarL;
        Log("Left activity bar closed");
        break;
    case DlgObj::ActivityBarR:
        m_windows &= ~ViewActivityBarR;
        Log("Right activity bar closed");
        break;
    default:
        LogError(ExcepCategory::SwitchEnum, "Unregistered dialog is trying to be closed " << cast::to_u64(dlg));
    }

    g::pOptsManager->ui_open_wnds_save(m_windows);
}

void GraphN_Flow::on_menu_options()
{
    m_windows |= ViewOptions;
    g::pOptsManager->ui_open_wnds_save(m_windows);
}

void GraphN_Flow::on_menu_quit()
{
    LogF;
    g::pGraphN->handle_wnd_on_close();
}

void GraphN_Flow::on_menu_about()
{
    LogF;
}

void GraphN_Flow::on_menu_panel_window(ViewDialogs wnd)
{
    m_windows |= wnd;
    if (wnd != ViewPanelsResetAll)
        g::pOptsManager->ui_open_wnds_save(m_windows);
}

void GraphN_Flow::on_menu_wf(PanelWorkflow* wf_panel)
{
    LogF;
    if (wf_panel != nullptr)
    {
        activate_wf_panel(wf_panel);
    }
    else
    {
        wf_create();
    }
}

// WF public ->
void GraphN_Flow::wf_create()
{
    auto& data = g::pWfsManager->create(true /*generate_name*/, true /*dump_to_db*/, false /*ro*/);
    g::pWfsManager->set_active(data.id);
    auto panel = create_wf_panel(data);
    activate_wf_panel(panel);
    m_panel_wf_list->rebuild_items();
}

void GraphN_Flow::wf_create_copy_from(wid source, std::optional<ImRect> wf_rc, std::optional<float> wf_zoom)
{
    auto info = g::pWfsManager->create_copy(source);
    g::pWfsManager->set_active(info.id);
    auto panel = create_wf_panel(info);
    activate_wf_panel(panel);
    panel->set_wf_view(wf_rc, wf_zoom);
    m_panel_wf_list->rebuild_items();
}

void GraphN_Flow::wf_rebuild(wid id)
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
    auto panel = find_wf_panel(id);
    if (nullptr == panel && id != 0)
    {
        auto& data = g::pWfsManager->get_data(id);
        panel = create_wf_panel(data);
    }
    activate_wf_panel(panel);

    m_panel_wf_list->rebuild_items();
}

void GraphN_Flow::wf_locate(wid id)
{
    auto panel = find_wf_panel(id);
    if (nullptr == panel)
    {
        auto& data = g::pWfsManager->get_data(id);
        panel = create_wf_panel(data);
    }
    wf_activate(id);
    panel->restore_pos();
    m_panel_wf_list->rebuild_items();
}

void GraphN_Flow::wf_delete(wid id)
{
    auto wf = g::pWfsManager->get_active();
    if (wf == id)
    {
        g::pWfsManager->set_active(0);
        activate_wf_panel(nullptr);
    }

    g::pWfsManager->delete_wf(id);
    g::pHistoryManager->cleanup(id);

    auto panel = find_wf_panel(id);
    if (panel != nullptr)
    {
        close_wf_panel(panel);
    }
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

    auto panel = find_wf_panel(id);
    if (panel != nullptr)
        rename_wf_panel(panel, new_name);

    m_panel_wf_list->rebuild_items();
}

void GraphN_Flow::wf_add_node_to_active_panel(data::Node* node, const std::string& select /*= ""*/)
{
    if (nullptr == m_wf_focused)
        g::pFlow->wf_create();
    m_wf_focused->get_action()->add_node(node, select, {});
}

void GraphN_Flow::wf_locate_node(wid id, data::NodeInstance* node)
{
    wf_activate(id);
    if (m_wf_focused != nullptr)
        m_wf_focused->locate_node(node);
}

// <- WF public

// WF internal ->
void GraphN_Flow::init_workflow_panels()
{
    if (g::pDbManager->get_is_first_run())
    {
        wf_create();
    }
    else
    {
        auto active = g::pWfsManager->get_active();
        if (active != 0)
        {
            auto& data = g::pWfsManager->get_data(active);
            auto panel = create_wf_panel(data);
            activate_wf_panel(panel);
            m_panel_wf_list->rebuild_items();
        }
    }
}

PanelWorkflow* GraphN_Flow::find_wf_panel(wid id) const
{
    for (const auto& panel : m_panels_wf)
    {
        if (panel->get_workflow()->get_id() == id)
            return panel.get();
    }
    return nullptr;
}

PanelWorkflow* GraphN_Flow::create_wf_panel(const WfData& wf)
{
    m_panels_wf.emplace_back(std::make_unique<ui::PanelWorkflow>(wf.id, wf.name, wf.uuid, m_main_dock_id));
    auto& panel_wf = m_panels_wf.back();
    panel_wf->init();
    m_all_dialogs.insert(panel_wf.get());
    m_windows |= ViewPanelWf;
    return panel_wf.get();
}

void GraphN_Flow::activate_wf_panel(PanelWorkflow* wf_panel)
{
    if (m_wf_focused != wf_panel)
    {
        for (auto& panel : m_panels_wf)
        {
            panel->set_active(false);
            panel->mark_dirty(panel->get_workflow()->get_data().dirty);
        }
        if (wf_panel != nullptr)
        {
            wf_panel->set_active(true);
            activate_txt_panels(wf_panel->get_workflow()->get_id());
        }

        m_wf_focused = wf_panel;
    }
}

void GraphN_Flow::close_wf_panel(PanelWorkflow* wf_panel)
{
    wf_panel->stop_all_threads();
    m_all_dialogs.erase(wf_panel);
    
    txt_panel_clear(wf_panel->get_workflow()->get_id());
    g::pHistoryManager->cleanup(wf_panel->get_workflow()->get_id());
    g::pWfsManager->close(wf_panel->get_workflow()->get_id());

    if (m_panels_wf.empty())
        destroy_txt_panels();
    wf_panel->free();

    for (auto it = m_panels_wf.begin(); it != m_panels_wf.end(); ++it)
    {
        if ((*it).get() == wf_panel)
        {
            m_panels_wf.erase(it);
            break;
        }
    }

    if (m_wf_focused == wf_panel)
        wf_activate(0);

    on_dlg_closed(DlgObj::PanelWf);
}

void GraphN_Flow::rename_wf_panel(PanelWorkflow* wf_panel, const std::string& new_name)
{
    wf_panel->rename_panel(new_name);
}

// <- WF internal

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
        auto ptr = std::make_unique<PanelTextEditor>(DlgObjID[cast::to_szt(DlgObj::PanelTextEditor)], true);
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
    current_panel->set_focused();
    current_panel->update(wf, node, pin, buffer, hint);
}

void GraphN_Flow::txt_panel_delete(wid wf, data::NodeInstance* node)
{
    for (auto& ptr : m_panels_text_editor)
    {
        ptr->reset(wf, node);
    }
}

void GraphN_Flow::txt_panel_clear(wid wf)
{
    for (auto& ptr : m_panels_text_editor)
    {
        ptr->reset(wf);
    }
}

ImGuiID GraphN_Flow::find_def_txt_panel_dock()
{
    auto wnd = ImGui::FindWindowByName(DlgObjID[cast::to_szt(DlgObj::PanelTextEditor)]);
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

void GraphN_Flow::destroy_txt_panels()
{
    for (auto& ptr : m_panels_text_editor)
    {
        ptr->reset();
    }
    m_panels_text_editor.clear();
    m_windows &= ~ViewPanelTextEditor;
}

void GraphN_Flow::destroy_txt_panel(PanelTextEditor* panel)
{
    for (auto it = m_panels_text_editor.begin(); it != m_panels_text_editor.end(); ++it)
    {
        if (it->get() == panel)
        {
            panel->reset();
            m_panels_text_editor.erase(it);
            m_all_dialogs.erase(panel);
            break;
        }
    }
}

void GraphN_Flow::on_menu_import_json()
{
 //   tools::path::FdSelectResultMt callback = [this](bool, std::list<std::filesystem::path>&&)
 //   {};
 //   tools::path::select_files_open({ "ComfyUI Workflows (*.json, *.png, *.webp)", "*.json, *.png, *.webp" }, callback);
}

void GraphN_Flow::on_menu_export_json()
{
    // TODO ICON_LC_SHARE
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

    if (!m_opts_dlg)
    {
        m_opts_dlg = std::make_unique<ui::OptionsDlgs>();
        m_opts_dlg->init();
        m_all_dialogs.insert(m_opts_dlg.get());
        Log("OptionsDlg initialized");
    }

    bool focused = false;
    if (m_opts_dlg->show_dialog(focused) != DlgCode::Progress)
    {
        on_dlg_closed(DlgObj::OptionsDlg);
    }
}

void GraphN_Flow::show_activity_bar()
{
    if ((m_windows & ViewActivityBarL) || (m_windows & ViewActivityBarR))
    {
        m_activity_bar->show(m_activity_bar_mode);
    }
}

void GraphN_Flow::show_debug_windows()
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
#endif
}

void GraphN_Flow::show_modal()
{
    if (m_windows & ViewModal)
        modal_show();
    else if (m_windows & ViewModalStop)
        modal_last_frame();

}

void GraphN_Flow::show_panel(BaseDlg* panel, DlgObj obj)
{
    bool focused = false;
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

void GraphN_Flow::show_panel_output()
{
    show_panel(m_panel_logs.get(), DlgObj::PanelServerLogs);
}

void GraphN_Flow::show_panel_templates()
{
    show_panel(m_panel_templates.get(), DlgObj::PanelTemplates);
}

void GraphN_Flow::show_panel_notes()
{
    //show_panel(m_panel_notes.get(), DlgObj::PanelNotes);
}

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
    for (auto it = m_panels_wf.begin(); it != m_panels_wf.end();)
    {
        auto& panel = *it;
        ++it;

        bool focused = false;
        if (panel->show_dialog(focused) != DlgCode::Progress)
        {
            close_wf_panel(panel.get());
        }
        else if (focused && panel.get() != m_wf_focused)
        {
            wf_activate(panel->get_workflow()->get_id());
        }
    }
}

void GraphN_Flow::handle_comfyui_runserver_error(int32_t error, const std::string& title, const std::string& description)
{
    (void)error;
    (void)title;
    (void)description;
}

void GraphN_Flow::show_menu_bar_net_status()
{
    std::string text;
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
            text = APP_COMFYUI " disconnected";
        }
        else if (ConnectState::Connecting == status)
        {
            text = fmt::format(FMT_STRING("Connecting to " APP_COMFYUI " on {}:{}"), g::pOptsManager->comfyui_host_load(), g::pOptsManager->comfyui_port_load());
        }
    }
    else if (m_windows & ViewParsingNodes)
    {
        size_t parsed_item = 0, parse_total = 0;
        m_nodes_parser->check_status(parsed_item, parse_total);
        text = fmt::format(FMT_STRING("Parsing node {} of {}"), parsed_item, parse_total);

        if (m_nodes_parser->is_parse_finished())
        {
            if (m_nodes_parser->is_parse_failed())
            {
                text = "Failed to parse loaded nodes";
            }
            else
            {
                m_windows &= ~ViewParsingNodes;
                m_nodes_parser.reset();
            }
        }
    }
    else if (get_online())
    {
        text = fmt::format(FMT_STRING("Connected to " APP_COMFYUI " on {}:{}"), g::pOptsManager->comfyui_host_load(), g::pOptsManager->comfyui_port_load());
    }

    const float text_width = ImGui::CalcTextSize(text.c_str()).x;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float right_align_pos = ImGui::GetWindowWidth() - (text_width + spacing + 10.0f);
    if (right_align_pos > ImGui::GetCursorPosX())
    {
        ImGui::SetCursorPosX(right_align_pos);
    }

    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->color_load(AppColor::DisconnectedMenuBarText));
    ImGui::TextUnformatted(text.c_str(), text.c_str() + text.size());
    ImGui::PopStyleColor();
}

void GraphN_Flow::show_panels()
{
    const auto id_as_name = [](std::string& id_ret, const char* panel_id)
    {
        id_ret = fmt::format(FMT_STRING("###{}"), panel_id);
        return id_ret.c_str();
    };

    auto viewport = ImGui::GetMainViewport();
    m_main_dock_id = ImGui::DockSpaceOverViewport(viewport, ImGuiDockNodeFlags_PassthruCentralNode);

    static bool focus_fixed = false;

    if (m_windows & ViewPanelsResetAll)
    {
        focus_fixed = false;

        m_windows &= ~ViewPanelsResetAll;
        m_windows |= ViewAllPanels;
        g::pOptsManager->ui_open_wnds_save(m_windows);

        ImGui::DockBuilderRemoveNode(m_main_dock_id);
        ImGui::DockBuilderAddNode(m_main_dock_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(m_main_dock_id, viewport->WorkSize);

        ImGuiID dock_id_left_top = ImGui::DockBuilderSplitNode(m_main_dock_id, ImGuiDir_Left, 0.15f, nullptr, &m_main_dock_id);
        ImGuiID dock_id_left_btm = ImGui::DockBuilderSplitNode(dock_id_left_top, ImGuiDir_Down, 0.3f, nullptr, &dock_id_left_top);
        ImGuiID dock_id_right_top = ImGui::DockBuilderSplitNode(m_main_dock_id, ImGuiDir_Right, 0.15f, nullptr, &m_main_dock_id);
        ImGuiID dock_id_right_bottom = ImGui::DockBuilderSplitNode(dock_id_right_top, ImGuiDir_Down, 0.5f, nullptr, &dock_id_right_top);
        ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(m_main_dock_id, ImGuiDir_Down, 0.15f, nullptr, &m_main_dock_id);

        std::string id;

        // left top
        ImGui::DockBuilderDockWindow(id_as_name(id, m_panel_nodes->get_panel_id()), dock_id_left_top);
        ImGui::DockBuilderDockWindow(id_as_name(id, m_panel_models->get_panel_id()), dock_id_left_top);

        // left bottom
        ImGui::DockBuilderDockWindow(id_as_name(id, m_panel_wf_list->get_panel_id()), dock_id_left_btm);
        ImGui::DockBuilderDockWindow(id_as_name(id, m_panel_templates->get_panel_id()), dock_id_left_btm);

        // right top
        ImGui::DockBuilderDockWindow(id_as_name(id, m_panel_stats->get_panel_id()), dock_id_right_top);

        // right bottom
        //ImGui::DockBuilderDockWindow(id_as_name(id, m_panel_notes->get_panel_id()), dock_id_right_bottom);
        ImGui::DockBuilderDockWindow(id_as_name(id, m_panel_node_inspector->get_panel_id()), dock_id_right_bottom);

        // bottom
        ImGui::DockBuilderDockWindow(id_as_name(id, m_panel_logs->get_panel_id()), dock_id_bottom);
        ImGui::DockBuilderDockWindow(id_as_name(id, DlgObjID[cast::to_szt(DlgObj::PanelTextEditor)]), dock_id_bottom);

        for (auto& wf : m_panels_wf)
        {
            ImGui::DockBuilderDockWindow(id_as_name(id, wf->get_panel_id()), m_main_dock_id);
        }
        for (auto& editor : m_panels_text_editor)
        {
            ImGui::DockBuilderDockWindow(id_as_name(id, editor->get_panel_id()), dock_id_bottom);
        }

        ImGui::DockBuilderFinish(m_main_dock_id);

        static bool wf_panels_initilized = false;
        if (!wf_panels_initilized)
        {
            init_workflow_panels();
            wf_panels_initilized = true;
        }
    }

    if (ImGui::GetFrameCount() > 10 && !focus_fixed)
    {
        focus_fixed = true;
        std::string id;
        ImGui::SetWindowFocus(id_as_name(id, m_panel_nodes->get_panel_id()));
        ImGui::SetWindowFocus(id_as_name(id, m_panel_wf_list->get_panel_id()));
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
    if (m_windows & ViewPanelNotes)
        show_panel_notes();
    if (m_windows & ViewPanelOutput)
        show_panel_output();
    if (m_windows & ViewPanelTemplates)
        show_panel_templates();

    if (m_windows & ViewPanelWf)
        show_panel_wf();
    if (m_windows & ViewPanelTextEditor)
        show_panel_text_editor();
}

void GraphN_Flow::modal_begin(std::string&& caption, std::string&& message, int32_t buttons, ModalDlgCtrl def, const std::function<void(ModalDlgCtrl)>& callback)
{
    if (m_modal_message)
    {
        LogInternalLogic("Message dialog already open");
        return;
    }

    Log("Modal started: " << message);
    m_msg_callback = callback;
    m_modal_message = std::make_unique<ModalMessage>();
    m_modal_message->init(std::move(caption), std::move(message), buttons, def);
    m_windows |= ViewModal;
}

void GraphN_Flow::GraphN_Flow::modal_show()
{
    if (!m_modal_message)
    {
        LogInternalLogic("Message dialog wasn't initialized");
        return;
    }

    auto ret = m_modal_message->show();
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
    if (!m_modal_message)
    {
        LogInternalLogic("Message dialog wasn't initialized");
        return;
    }
    Log("Modal updated: " << message);
    m_modal_message->update(std::move(message));
}

void GraphN_Flow::modal_stop()
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
        LogInternalLogic("Message dialog wasn't initialized");
        return;
    }

    m_modal_message->stop();
    m_modal_message->show();
    m_windows &= ~(ViewModal | ViewModalStop);
    m_modal_message.reset();
}

}

