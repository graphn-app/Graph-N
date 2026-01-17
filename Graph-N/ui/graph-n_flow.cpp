
#include "ui/graph-n_flow.h"
#include "ui/base_dlg.h"
#include "ui/options/opts_dlg.h"
#include "ui/panels/panel_nodes.h"
#include "ui/panels/panel_node_inspector.h"
#include "ui/panels/panel_stats.h"
#include "ui/panels/panel_wf.h"
#include "ui/panels/panel_wf_list.h"
#include "ui/panels/panel_models.h"
#include "ui/panels/panel_notes.h"
#include "ui/panels/panel_output.h"
#include "ui/panels/panel_templates.h"
#include "ui/workflow/workflow_drawer.h"
#include "core/managers/settings_manager.h"
#include "core/managers/connect_manager.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/comfyui_object_validity_listener.h"
#include "core/managers/nodes_parser.h"
#include "core/managers/db_manager.h"
#include "core/workflow/workflow.h"

namespace w
{
float Pin_l_offset = 0.0f;
float Pin_r_offset = 0.0f;
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

    w::Pin_l_offset = 4.0f;
    w::Pin_r_offset = 4.0f;
    w::ico_circle = ImGui::CalcTextSize(ICON_LC_CIRCLE_SMALL).x;
    w::ico_eye = ImGui::CalcTextSize(ICON_LC_EYE).x;
    w::ico_max = std::max(w::ico_circle, w::ico_eye);
    w::v_spacing = 4.0f;
    w::hidden_widgets_len = ImGui::CalcTextSize(w::hidden_widgets.c_str()).x;

    m_online = false;

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

    m_panel_output = std::make_unique<ui::PanelOutput>();
    m_panel_output->init();
    m_all_dialogs.insert(m_panel_output.get());

    m_panel_notes = std::make_unique<ui::PanelNotes>();
    m_panel_notes->init();
    m_all_dialogs.insert(m_panel_notes.get());

    m_panel_models = std::make_unique<ui::PanelModels>();
    m_panel_models->init();
    m_all_dialogs.insert(m_panel_models.get());

    init_workflow_panels();

    Log("Panels created");

    // start connect to ComfyUI
    reconnect();

    return true;
}

void GraphN_Flow::reconnect()
{
    LogF;
    m_windows |= ViewConnect;
    g::pConnectManager->connect();
}

void GraphN_Flow::init_workflow_panels()
{
    if (g::pDbManager->get_is_first_run())
    {
        auto wf = create_workflow();
        m_wf_focused = create_wf_panel(wf);
        m_panel_wf_list->add_wf(wf);
        m_panel_wf_list->set_active(wf);
    }
    else
    {
        auto wfs_data = g::pDbManager->wf_load_meta_delete_empty();
        for (auto& wf_data : wfs_data)
        {
            auto wf = create_workflow(&wf_data);
            auto panel = create_wf_panel(wf);
            m_panel_wf_list->add_wf(wf);
            if (g::pOptsManager->wf_active_load() == wf->get_id())
            {
                m_wf_focused = panel;
                m_panel_wf_list->set_active(wf);
            }
        }
    }
}

PanelWorkflow* GraphN_Flow::create_wf_panel(data::Workflow* wf)
{
    auto& wf_data = wf->get_workflow_data();
    wf_data.name = ICON_LC_WORKFLOW " Workflow " + std::to_string(++m_wf_name_counter);

    m_panels_wf.push_back(std::make_unique<ui::PanelWorkflow>(wf, wf->get_workflow_data().name.c_str(), wf->get_id().c_str(), m_main_dock_id));
    auto& panel_wf = m_panels_wf.back();
    panel_wf->init();

    m_all_dialogs.insert(panel_wf.get());
    return panel_wf.get();
}

PanelWorkflow* GraphN_Flow::get_active_wf_or_create_new()
{
    if (m_wf_focused != nullptr)
        return m_wf_focused;

    auto wf = create_workflow();
    m_wf_focused = create_wf_panel(wf);
    m_panel_wf_list->add_wf(wf);
    m_panel_wf_list->set_active(wf);
    return m_wf_focused;
}

data::Workflow* GraphN_Flow::create_workflow(const WfData* wf_data /*= nullptr*/)
{
    std::unique_ptr<data::Workflow> wf;
    if (wf_data != nullptr)
        wf = std::make_unique<data::Workflow>(*wf_data);
    else
        wf = std::make_unique<data::Workflow>();

    auto wf_ptr = wf.get();
    wf_ptr->init();
    wf_data = &wf_ptr->get_workflow_data();
    m_workflows.emplace(wf_data->uuid, std::move(wf));
    Log("Workflow created, id=" << wf_data->uuid);
    return wf_ptr;
}

void GraphN_Flow::delete_workflow(data::Workflow* wf)
{
    for (auto& panel : m_panels_wf)
    {
        if (panel->get_workflow()->get_id() == wf->get_id())
        {
            wf->get_workflow_data().deleted = true;
            panel->close_dialog();
            break;
        }
    }
}

void GraphN_Flow::destroy_workflow(data::Workflow* wf)
{
    auto wf_it = m_workflows.find(wf->get_id());
    if (wf_it == m_workflows.end())
    {
        LogAndThrowInternalError(ExcepCategory::InconsistentMap, "Can't destroy workflow id=" << wf->get_id());
    }
    else
    {
        Log("Workflow destroyed, id=" << wf->get_id());
        m_panel_wf_list->remove_wf(wf);
        m_workflows.erase(wf_it);
    }
}

void GraphN_Flow::subscribe_comfyui_validity_state(mngr::ComfyUiObjectValidityListener* listener)
{
    m_comfyui_obj_listeners.push_back(listener);
}

void GraphN_Flow::invalidate_comfyui_objects()
{
    Log("Notify " << m_comfyui_obj_listeners.size() << " listeners");
    for (auto* listener : m_comfyui_obj_listeners)
    {
        listener->invalidate_comfyui_objects();
    }
}

void GraphN_Flow::resolve_comfyui_objects()
{
    Log("Notify " << m_comfyui_obj_listeners.size() << " listeners");
    for (auto* listener : m_comfyui_obj_listeners)
    {
        listener->begin_resolve_comfyui_objects();
    }
    for (auto* listener : m_comfyui_obj_listeners)
    {
        listener->finalize_resolve_comfyui_objects();
    }
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

    if (m_windows & ViewOptions)
    {
        show_options_dlg();
    }

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

    show_panels();
    return true;
}

void GraphN_Flow::prepare_to_quit()
{
    LogF_In;

    m_panel_wf_list->remove_all_wf();
    for (auto& panel : m_panels_wf)
    {
        save_workflow(panel.get(), true);
    }

    for (auto& dlg : m_all_dialogs)
    {
        if (dlg)
            dlg->stop_all_threads();
    }
    m_all_dialogs.clear();
    m_panels_wf.clear();
    m_workflows.clear();

    if (m_nodes_parser)
        m_nodes_parser->stop();
    m_nodes_parser.reset();

    stop_all_threads();

    LogF_Out;
}

void GraphN_Flow::show_menu_bar()
{
    const bool connecting_mode = (m_windows & (ViewConnect | ViewParsingNodes)) > 0 && !m_online;
    if (connecting_mode)
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, g::pOptsManager->color_load(AppColor::DisconnectedMenuBarBg));

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File##MainMenuBar"))
        {
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
                const auto wf_name = fmt::format(FMT_COMPILE("{}##PanelsMenuBar"), wf->get_panel_name());
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
            ImGui::EndMenu();
        }

        //////////////	status and controls
        if (m_windows & (ViewConnect | ViewParsingNodes))
            show_menu_bar_connecting();
        else if (m_windows & ViewQueueCtrl)
            show_menu_bar_queue_controls();
        //////////////////////	

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
        Log("Wf panel closed");
        break;
    case DlgObj::PanelWfList:
        m_windows &= ~ViewPanelWfList;
        Log("Wf list panel closed");
        break;
    case DlgObj::PanelTemplates:
        m_windows &= ~ViewPanelTemplates;
        Log("Templates panel closed");
        break;
    case DlgObj::PanelOutput:
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
        auto wnd = ImGui::FindWindowByName(wf_panel->get_panel_name());
        if (wnd != nullptr)
        {
            ImGui::BringWindowToDisplayFront(wnd);
            m_wf_focused = wf_panel;
            m_panel_wf_list->set_active(m_wf_focused->get_workflow());
        }
    }
    else
    {
        auto wf = create_workflow();
        m_wf_focused = create_wf_panel(wf);
        m_panel_wf_list->add_wf(wf);
        m_panel_wf_list->set_active(wf);
    }
}

void GraphN_Flow::on_menu_import_json()
{
    // TODO
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
    show_panel(m_panel_output.get(), DlgObj::PanelOutput);
}

void GraphN_Flow::show_panel_templates()
{
    show_panel(m_panel_templates.get(), DlgObj::PanelTemplates);
}

void GraphN_Flow::show_panel_notes()
{
    show_panel(m_panel_notes.get(), DlgObj::PanelNotes);
}

void GraphN_Flow::show_panel_wf_list()
{
    show_panel(m_panel_wf_list.get(), DlgObj::PanelWfList);
}

void GraphN_Flow::show_panel_wf()
{
    for (auto it = m_panels_wf.begin(); it != m_panels_wf.end();)
    {
        auto& panel = *it;
        bool focused = false;
        if (panel->show_dialog(focused) != DlgCode::Progress)
        {
            if (panel.get() == m_wf_focused)
                m_wf_focused = nullptr;

            auto wf = panel->get_workflow();
            wf->stop_all_threads();

            if (wf->get_workflow_data().deleted)
            {
                g::pDbManager->wf_delete(wf->get_workflow_data());
            }
            else
            {
                save_workflow(panel.get(), true);
            }

            m_panel_wf_list->remove_wf(wf);
            panel->stop_all_threads();
            on_dlg_closed(DlgObj::PanelWf);

            m_all_dialogs.erase(panel.get());
            it = m_panels_wf.erase(it);
            destroy_workflow(wf);
        }
        else if (focused)
        {
            m_wf_focused = panel.get();
            m_panel_wf_list->set_active(m_wf_focused->get_workflow());
            ++it;
        }
        else
            ++it;
    }
}

void GraphN_Flow::show_menu_bar_connecting()
{
    std::string text;
    if (m_windows & ViewConnect)
    {
        const auto status = g::pConnectManager->check_status();
        if (mngr::ConnectState::Connected == status)
        {
            m_windows &= ~ViewConnect;
            m_windows |= ViewParsingNodes;

            g::pOptsManager->node_options_preload();
            m_nodes_parser = std::make_unique<mngr::NodesParser>();
#if 1
            m_nodes_parser->parse(debug::test_json);
#else
            m_nodes_parser->parse(std::move(g::pConnectManager->get_nodes_raw()));
#endif
            m_online = true;
            return;
        }
        if (mngr::ConnectState::Disconnected == status)
        {
            text = APP_COMFYUI " disconnected";
        }
        else if (mngr::ConnectState::Connecting == status)
        {
            text = fmt::format(FMT_COMPILE("Connecting to " APP_COMFYUI " on {}:{}"), g::pOptsManager->comfyui_host_load(), g::pOptsManager->comfyui_port_load());
        }
    }
    else if (m_windows & ViewParsingNodes)
    {
        size_t parsed_item = 0, parse_total = 0;
        m_nodes_parser->check_status(parsed_item, parse_total);
        text = fmt::format(FMT_COMPILE("Parsing node {} of {}"), parsed_item, parse_total);

        if (m_nodes_parser->is_parse_finished())
        {
            g::pOptsManager->node_options_release();
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

void GraphN_Flow::show_menu_bar_queue_controls()
{
    // TODO
}

void GraphN_Flow::cleanup_empty_workflow_panels()
{
    // TODO
}

void GraphN_Flow::save_workflow(PanelWorkflow* /*wf*/, bool /*last_call*/)
{
    // TODO
}

void GraphN_Flow::show_panels()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    const ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

    if (ImGui::Begin("MainDockSpaceWindow", nullptr, window_flags))
    {
        const ImGuiID dockspace_id = ImGui::GetID(APP_CAPTION "_DockSpace");
        m_main_dock_id = dockspace_id;

        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        if (m_windows & ViewPanelsResetAll)
        {
            m_windows &= ~ViewPanelsResetAll;
            m_windows |= ViewAllPanels;
            g::pOptsManager->ui_open_wnds_save(m_windows);

            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

            ImGuiID dock_id_left_top = ImGui::DockBuilderSplitNode(m_main_dock_id, ImGuiDir_Left, 0.2f, nullptr, &m_main_dock_id);
            ImGuiID dock_id_left_btm = ImGui::DockBuilderSplitNode(dock_id_left_top, ImGuiDir_Down, 0.5f, nullptr, &dock_id_left_top);
            ImGuiID dock_id_right_top = ImGui::DockBuilderSplitNode(m_main_dock_id, ImGuiDir_Right, 0.2f, nullptr, &m_main_dock_id);
            ImGuiID dock_id_right_bottom = ImGui::DockBuilderSplitNode(dock_id_right_top, ImGuiDir_Down, 0.5f, nullptr, &dock_id_right_top);
            ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(m_main_dock_id, ImGuiDir_Down, 0.2f, nullptr, &m_main_dock_id);

            ImGui::DockBuilderDockWindow(m_panel_nodes->get_panel_name(), dock_id_left_top);
            ImGui::DockBuilderDockWindow(m_panel_models->get_panel_name(), dock_id_left_top);

            ImGui::DockBuilderDockWindow(m_panel_wf_list->get_panel_name(), dock_id_left_btm);
            ImGui::DockBuilderDockWindow(m_panel_node_inspector->get_panel_name(), dock_id_left_btm);

            ImGui::DockBuilderDockWindow(m_panel_stats->get_panel_name(), dock_id_right_top);

            ImGui::DockBuilderDockWindow(m_panel_notes->get_panel_name(), dock_id_right_bottom);
            ImGui::DockBuilderDockWindow(m_panel_templates->get_panel_name(), dock_id_right_bottom);

            ImGui::DockBuilderDockWindow(m_panel_output->get_panel_name(), dock_id_bottom);

            for (auto& wf : m_panels_wf)
            {
                ImGui::DockBuilderDockWindow(wf->get_panel_name(), m_main_dock_id);
            }

            ImGui::DockBuilderFinish(dockspace_id);
            m_fix_focus = true;
        }
        else if (m_fix_focus)
        {
            m_fix_focus = false;
            ImGui::SetWindowFocus(m_panel_nodes->get_panel_name());
            ImGui::SetWindowFocus(m_panel_node_inspector->get_panel_name());
            ImGui::SetWindowFocus(m_panel_templates->get_panel_name());
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(3);

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
}

}



#if 1
namespace debug
{
const char* test_json =
R"(
    {
	    "ImpactKSamplerAdvancedBasicPipe": {
		    "input": {
			    "required": {
				    "basic_pipe": [
					    "IMAGE",
					    {
						    "tooltip": "basic_pipe input for sampling"
					    }
				    ],
				    "add_noise": [
					    "BOOLEAN",
					    {
						    "default": true,
						    "label_on": "enable",
						    "label_off": "disable",
						    "tooltip": "Whether to add noise"
					    }
				    ],
				    "noise_seed": [
					    "INT",
					    {
						    "default": 0,
						    "min": 0,
						    "max": 18446744073709551615,
						    "tooltip": "Random seed to use for generating CPU noise for sampling."
					    }
				    ],
				    "steps": [
					    "INT",
					    {
						    "default": 20,
						    "min": 1,
						    "max": 10000,
						    "tooltip": "total sampling steps"
					    }
				    ],
				    "labels": [
					    "STRING",
					    {
						    "multiline": true,
						    "placeholder": "List the types of segments to be allowed, separated by commas"
					    }
				    ],
				    "cfg": [
					    "FLOAT",
					    {
						    "default": 8.0,
						    "min": 0.0,
						    "max": 100.0,
						    "tooltip": "classifier free guidance value",
                            "advanced": true
					    }
				    ],
				    "constant_color": [
					    "COLOR",
					    {
						    "default": "#123456"
					    }
				    ],
				    "constant_color_alpha": [
					    "COLOR",
					    {
						    "default": "#12345678",
						    "tooltip": "RGBA"
					    }
				    ],
				    "folder_path": [
					    "STRING",
					    {
						    "default": "",
						    "tooltip": "Path to the folder containing images. Relative paths are resolved to the ComfyUI output directory."
					    }
				    ],
				    "sampler_name": [
					    [
						    "euler",
						    "euler_cfg_pp",
						    "euler_ancestral",
						    "euler_ancestral_cfg_pp",
						    "heun",
						    "heunpp2",
						    "dpm_2",
						    "dpm_2_ancestral",
						    "lms",
						    "dpm_fast",
						    "dpm_adaptive",
						    "dpmpp_2s_ancestral",
						    "dpmpp_2s_ancestral_cfg_pp",
						    "dpmpp_sde",
						    "dpmpp_sde_gpu",
						    "dpmpp_2m",
						    "dpmpp_2m_cfg_pp",
						    "dpmpp_2m_sde",
						    "dpmpp_2m_sde_gpu",
						    "dpmpp_2m_sde_heun",
						    "dpmpp_2m_sde_heun_gpu",
						    "dpmpp_3m_sde",
						    "dpmpp_3m_sde_gpu",
						    "ddpm",
						    "lcm",
						    "ipndm",
						    "ipndm_v",
						    "deis",
						    "res_multistep",
						    "res_multistep_cfg_pp",
						    "res_multistep_ancestral",
						    "res_multistep_ancestral_cfg_pp",
						    "gradient_estimation",
						    "gradient_estimation_cfg_pp",
						    "er_sde",
						    "seeds_2",
						    "seeds_3",
						    "sa_solver",
						    "sa_solver_pece",
						    "ddim",
						    "uni_pc",
						    "uni_pc_bh2"
					    ],
					    {
						    "tooltip": "sampler"
					    }
				    ],
				    "scheduler": [
					    [
						    "simple",
						    "sgm_uniform",
						    "karras",
						    "exponential",
						    "ddim_uniform",
						    "beta",
						    "normal",
						    "linear_quadratic",
						    "kl_optimal",
						    "AYS SDXL",
						    "AYS SD1",
						    "AYS SVD",
						    "GITS[coeff=1.2]",
						    "LTXV[default]",
						    "OSS FLUX",
						    "OSS Wan",
						    "OSS Chroma"
					    ],
					    {
						    "tooltip": "noise schedule"
					    }
				    ],
				    "latent_image": [
					    "MASK",
					    {
						    "tooltip": "input latent image"
					    }
				    ],
				    "start_at_step": [
					    "INT",
					    {
						    "default": 0,
						    "min": 0,
						    "max": 10000,
						    "tooltip": "The starting step of the sampling to be applied at this node within the range of 'steps'."
					    }
				    ],
				    "end_at_step": [
					    "INT",
					    {
						    "default": 10000,
						    "min": 0,
						    "max": 10000,
						    "tooltip": "The step at which sampling applied at this node will stop within the range of steps (if greater than steps, sampling will continue only up to steps)."
					    }
				    ],
				    "return_with_leftover_noise": [
					    "BOOLEAN",
					    {
						    "default": false,
						    "label_on": "enable",
						    "label_off": "disable",
						    "tooltip": "Whether to return the latent with noise remaining if the noise has not been completely removed according to the noise schedule, or to completely remove the noise before returning it."
					    }
				    ]
			    },
			    "optional": {
				    "scheduler_func_opt": [
					    "SCHEDULER_FUNC",
					    {
						    "tooltip": "[OPTIONAL] Noise schedule generation function. If this is set, the scheduler widget will be ignored."
					    }
				    ]
			    }
		    },
		    "input_order": {
			    "required": [
				    "basic_pipe",
				    "add_noise",
				    "noise_seed",
				    "steps",
				    "cfg",
				    "sampler_name",
				    "scheduler",
				    "latent_image",
				    "start_at_step",
				    "end_at_step",
				    "return_with_leftover_noise"
			    ],
			    "optional": [
				    "scheduler_func_opt"
			    ]
		    },
		    "output": [
			    "CLIP",
			    "LATENT",
			    "CONDITIONING",
			    "MODEL"
		    ],
		    "output_is_list": [
			    false,
			    false,
			    false,
			    false
		    ],
		    "output_name": [
			    "CLIP",
			    "LATENT",
			    "CONDITIONING",
			    "MODEL"
		    ],
		    "name": "ImpactKSamplerAdvancedBasicPipe",
		    "display_name": "KSampler",
            "description": "",
            "python_module" : "custom_nodes.ComfyUI-Impact-Pack",
            "category" : "sampling",
            "output_node" : false,
            "output_tooltips" : [
            "passthrough input basic_pipe",
            "result latent",
            "VAE in basic_pipe"
        ]
    }
})";
} // namespace debug
#endif
