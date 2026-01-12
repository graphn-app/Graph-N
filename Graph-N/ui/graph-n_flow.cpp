
#include "ui/graph-n_flow.h"
#include "ui/base_dlg.h"
#include "ui/options/opts_dlg.h"
#include "ui/panels/panel_nodes.h"
#include "ui/panels/panel_node_inspector.h"
#include "ui/panels/panel_stats.h"
#include "ui/workflow/workflow_drawer.h"
#include "core/managers/settings_manager.h"
#include "core/managers/connect_manager.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/comfyui_object_validity_listener.h"
#include "core/managers/nodes_parser.h"
#include "core/workflow/workflow.h"

namespace w
{
float pin_l_offset = 0.0f;
float pin_r_offset = 0.0f;
float ico_circle = 0.0f;
float ico_eye = 0.0f;
float ico_max = 0.0f;
float v_spacing = 0.0f;
std::string hidden_widgets = "Hidden widgets";
float hidden_widgets_len;
}

/*
|<------------------------------------------------------0------------------------------------------>|
|<------------------------------------------------------1/21--------------------------------------->|         ---->|
                                                                                                                   | 19
|                                     [             node name           ]                           |              |
|<---------------2------------------->|<--------------3/4-------------->|<-----------2------------->|         ---->| 
                                                                                                                   | 19
|     [       pin_ico    ]     [in_pin_name][    center      ][out_pin_name]     [ pin_ico    ]     |              |
|<-5->|<------6/13------>|<-7->|<---8/9--->|[<-----10------->|<---11/12--->|<-7->|<---6/13--->|<-5->|         ---->|
                                                                                                                   | 20
|     [    widget_ico    ]     [  widget_name       ][     widget_value    ]                        |              |
|<-5->|<------13-------->|<-7->|<-------14/15------>||<---------16/17----->|<-7->|<----6/13-->|<-5->|         ---->|

|     [                                            separator                                  |     |
|<-5->|<------13-------->|<-7->|<-----------------18-----------------------|<-7->|<----6/13-->|<-5->|


----------------------------------------------------------------------------------------------------------------------------------------------------------------

----------------------------------------------------------------------------------------------------------------------------------------------------------------
##  | what                   | equal to                                                         | variable                          | when
----------------------------------------------------------------------------------------------------------------------------------------------------------------
3   | node name              | CalcTextSize(Node::name)                                         | Node::w_name                      | on parse
5   | pin left offset        | 4.0                                                              | w::pin_l_offset                   | app start
6   | icon width             | CalcTextSize(ICON_CIRCLE)                                        | w::ico_circle                     | app start
7   | pin right offset       | 4.0                                                              | w::pin_r_offset                   | app start
8   | input pin name         | CalcTextSize(Pin::name))                                         | Pin::w_name                       | parse
11  | output pin name        | CalcTextSize(Pin::name))                                         | Pin::w_name                       | parse
13  | icon width             | CalcTextSize(ICON_EYE)                                           | w::ico_eye                        | app start
14  | widget name            | CalcTextSize(Pin::name))                                         | Pin::w_name                       | parse
16  | widget value           | CalcTextSize(Pin::value))                                        | Pin::w_value                      | parse
19  | vertical spacing       | 4.0                                                              | w::v_spacing                      | app start
20  | vertical spacing       | (19)*2                                                           |                                   | app start
----------------------------------------------------------------------------------------------------------------------------------------------------------------
9   | max input pin name     | max(!wigdet(8))                                                  | Node::w_pin_in_max                | first instance create
12  | max output pin name    | max(11)                                                          | Node::w_pin_out_max               | first instance create
15  | max widget name        | max(widget(14) + 2 * ImGui::GetStyle().ItemSpacing.x)            | Node::w_widget_name_max           | first instance create
17  | max widget value       | max(16)                                                          | Node::w_widget_value_max          | first instance create
21  | node ideal fit         | = (1 ideal)                                                      | Node::w_fit                       | first instance create
----------------------------------------------------------------------------------------------------------------------------------------------------------------
0   | available width        |                                                                  |                                   | every frame
1   | node width             | (5)*2 + (13)*2 + (7)*2 + (15) + (17)                             | NodeInstance::w_node              | every frame
2   | node name l/r margins  | (0)/2 - (4)/2                                                    | NodeInstance::w_node_name_off     | every frame
4   | node name              | CalcTextSize(Node::name)                                         | NodeInstance::w_name              | on node rename
10  | central zone           | (15) + (17) - (9) - (12)                                         | NodeInstance::w_center            | every frame
18  | hidden items separator | (0) - (5)*2 + (13)*2 + (7)*2                                     | NodeInstance:::w_separator        | every frame
----------------------------------------------------------------------------------------------------------------------------------------------------------------

- pin icon (16), widget ico (13) - take max depend on: tooltip, inspection, wf

- empty node:       (no in pins, no out pins, no wdg):     - (1) = (100)
- widgets only:     (no in pins, no out pins):             - (1) = (5)*2 + (13)*2 + (7)*2 + (15) + (17)
- input only:       (no out pins, no wdg):                 - (10) = 50, (1) = (5)*2 + (13)*2 + (7)*2 + (9) + (10)
- output only:      (no in pins, no wdg):                  - (10) = 50, (1) = (5)*2 + (13)*2 + (7)*2 + (10) + (12)
- wdg and out:      (no in pins):                          - (1) = (5)*2 + (13)*2 + (7)*2 + (15) + (17), (10) = (15) + (17) - (12)
- wdg and in:       (no out pins):                         - (1) = (5)*2 + (13)*2 + (7)*2 + (15) + (17), (10) = (15) + (17) - (9)
- in and out:       (no wdg):                              - (10) = 10, (1) = (5)*2 + (13)*2 + (7)*2 + (9) + (10) + (12)

- if (0) < (21):           - (15) = ((0) - (5)*2 - (13)*2 - (7)*2) / 3, but >= 0
                           - (17) = ((0) - (5)*2 - (13)*2 - (7)*2) / 3 * 2, but >= 0
                           - (10) = (15) + (17) - (9) - (12), but >= 0
                           - (9) = ((15) + (17) / 2), but >= 0
                           - (12) = ((15) + (17) / 2), but >= 0
                           - (1) = (0)
- if (0) > (21):           - (15) = fixed
                           - (17) = (0) - (5)*2 - (13)*2 - (7)*2 - (15)
                           - (9) = fixed
                           - (12) = fixed
                           - (10) = (0) - (5)*2 - (13)*2 - (7)*2 - (9) - (12)
                           - (1) = (0)

*/

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

    w::pin_l_offset = 4.0f;
    w::pin_r_offset = 4.0f;
    w::ico_circle = ImGui::CalcTextSize(ICON_LC_CIRCLE_SMALL).x;
    w::ico_eye = ImGui::CalcTextSize(ICON_LC_EYE).x;
    w::ico_max = std::max(w::ico_circle, w::ico_eye);
    w::v_spacing = 4.0f;
    w::hidden_widgets_len = ImGui::CalcTextSize(w::hidden_widgets.c_str()).x;

    m_online = false;

    // menu colors
    ImGuiStyle& style = ImGui::GetStyle();

    // Background: Dark Grey (Opaque) - Lighter than main window
    style.Colors[ImGuiCol_PopupBg] = ImColor(IM_COL32(50, 50, 50, 255));
    style.Colors[ImGuiCol_MenuBarBg] = ImColor(IM_COL32(50, 50, 50, 255));

    // open windows
    m_windows = g::pOptsManager->flow_get_open_wnds();

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

data::Workflow* GraphN_Flow::create_workflow()
{
    auto wf = std::make_unique<data::Workflow>();
    auto wf_ptr = wf.get();
    wf_ptr->init();
    m_workflows.emplace(wf_ptr->get_id(), std::move(wf));
    Log("Workflow created, id=" << wf_ptr->get_id());
    return wf_ptr;
}

WorkflowDrawer* GraphN_Flow::create_workflow_drawer(data::Workflow* wf)
{
    auto drawer = std::make_unique<WorkflowDrawer>();
    auto drawer_ptr = drawer.get();
    drawer_ptr->init(wf);
    m_workflow_drawers.emplace(wf->get_id(), std::move(drawer));
    Log("Workflow drawer created for wf id=" << wf->get_id());
    return drawer_ptr;
}

void GraphN_Flow::destroy_workflow(lid wf_id)
{
    auto dr_it = m_workflow_drawers.find(wf_id);
    if (dr_it == m_workflow_drawers.end())
    {
        LogAndThrowInternalError(ExcepCategory::inconsistent_map, "Can't destroy workflow drawer id=" << wf_id);
    }
    else
    {
        m_workflow_drawers.erase(dr_it);
        Log("Workflow destroyed, id=" << wf_id);
    }

    auto wf_it = m_workflows.find(wf_id);
    if (wf_it == m_workflows.end())
    {
        LogAndThrowInternalError(ExcepCategory::inconsistent_map, "Can't destroy workflow id=" << wf_id);
    }
    else
    {
        m_workflows.erase(wf_it);
        Log("Workflow destroyed, id=" << wf_id);
    }
}

data::Workflow* GraphN_Flow::find_workflow(lid wf_id)
{
    auto it = m_workflows.find(wf_id);
    if (it != m_workflows.end())
        return it->second.get();
    return nullptr;
}

WorkflowDrawer* GraphN_Flow::find_workflow_drawer(lid wf_id)
{
    auto it = m_workflow_drawers.find(wf_id);
    if (it != m_workflow_drawers.end())
        return it->second.get();
    return nullptr;
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

    if (m_windows & ViewOptions || m_windows & ViewOptsComfyUi)
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
    LogF;

    for (auto& dlg : m_all_dialogs)
    {
        if (dlg)
            dlg->stop_all_threads();
    }
    m_all_dialogs.clear();

    if (m_nodes_parser)
        m_nodes_parser->stop();
    m_nodes_parser.reset();

    stop_all_threads();
}

void GraphN_Flow::show_menu_bar()
{
    const bool connecting_mode = (m_windows & (ViewConnect | ViewParsingNodes)) > 0 && !m_online;
    if (connecting_mode)
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, g::pOptsManager->flow_get_color(VisibleObject::DisconnectedMenuBar));

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
        if (ImGui::BeginMenu("Windows##MainMenuBar"))
        {
            if (ImGui::MenuItem("Nodes Library##WindowsMenuBar", nullptr, (m_windows & ViewPanelNodes)))
            {
                Log("Menu: Nodes Library");
                on_menu_panel_window(ViewPanelNodes);
            }
            if (ImGui::MenuItem("Node Inspector##WindowsMenuBar", nullptr, (m_windows & ViewPanelNodeInpector)))
            {
                Log("Menu: Node Inspector");
                on_menu_panel_window(ViewPanelNodeInpector);
            }
            if (ImGui::MenuItem("Stats##WindowsMenuBar", nullptr, (m_windows & ViewPanelStats)))
            {
                Log("Menu: stats panel");
                on_menu_panel_window(ViewPanelStats);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Reset default layout##WindowsMenuBar"))
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

void GraphN_Flow::on_dlg_closed(VisibleObject dlg)
{
    switch (dlg)
    {
    case VisibleObject::OptionsDlg:
        m_windows &= ~ViewAllOptions;
        m_all_dialogs.erase(m_opts_dlg.get());
        m_opts_dlg.reset();
        Log("Options dlg closed");
        break;
    case VisibleObject::PanelNodes:
        m_windows &= ~ViewPanelNodes;
        Log("Nones panel closed");
        break;
    case VisibleObject::PanelNodeInspector:
        m_windows &= ~ViewPanelNodeInpector;
        Log("Nones Inspector panel closed");
        break;
    case VisibleObject::PanelStats:
        m_windows &= ~ViewPanelStats;
        Log("Stats panel closed");
        break;
    default:
        LogError(ExcepCategory::switch_enum, "Unregistered dialog is trying to be closed " << cast::to_u64(dlg));
    }

    g::pOptsManager->flow_set_open_wnds(m_windows);
}

void GraphN_Flow::on_menu_options()
{
    m_windows |= ViewOptions;
    g::pOptsManager->flow_set_open_wnds(m_windows);
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
        g::pOptsManager->flow_set_open_wnds(m_windows);
}

void GraphN_Flow::show_options_dlg()
{
    if (!m_opts_dlg)
    {
        OptPane pane = OptPane::_none;
        if (m_windows & ViewOptsComfyUi)
            pane = OptPane::comfyui;

        m_opts_dlg = std::make_unique<ui::OptionsDlgs>(pane);
        m_opts_dlg->init();
        m_all_dialogs.insert(m_opts_dlg.get());
        Log("OptionsDlg initialized");
    }

    bool focused = false;
    if (m_opts_dlg->show_dialog(focused) != DlgCode::progress)
    {
        on_dlg_closed(VisibleObject::OptionsDlg);
    }
    else if (focused)
        m_focused_wnd = m_opts_dlg.get();
}

void GraphN_Flow::show_panel_nodes()
{
    bool focused = false;
    if (m_panel_nodes->show_dialog(focused) != DlgCode::progress)
    {
        on_dlg_closed(VisibleObject::PanelNodes);
    }
    else if (focused)
        m_focused_wnd = m_panel_nodes.get();
}

void GraphN_Flow::show_panel_node_inspector()
{
    bool focused = false;
    if (m_panel_node_inspector->show_dialog(focused) != DlgCode::progress)
    {
        on_dlg_closed(VisibleObject::PanelNodeInspector);
    }
    else if (focused)
        m_focused_wnd = m_panel_node_inspector.get();
}

void GraphN_Flow::show_panel_stats()
{
    bool focused = false;
    if (m_panel_stats->show_dialog(focused) != DlgCode::progress)
    {
        on_dlg_closed(VisibleObject::PanelStats);
    }
    else if (focused)
        m_focused_wnd = m_panel_stats.get();
}

void GraphN_Flow::show_menu_bar_connecting()
{
    std::string text;
    if (m_windows & ViewConnect)
    {
        const auto status = g::pConnectManager->check_status();
        if (mngr::ConnectState::connected == status)
        {
            m_windows &= ~ViewConnect;
            m_windows |= ViewParsingNodes;

            g::pOptsManager->nd_preload_node_options();
            m_nodes_parser = std::make_unique<mngr::NodesParser>();
#if 1
            m_nodes_parser->parse(debug::test_json);
#else
            m_nodes_parser->parse(std::move(g::pConnectManager->get_nodes_raw()));
#endif
            m_online = true;
            return;
        }
        if (mngr::ConnectState::disconnected == status)
        {
            text = APP_COMFYUI " disconnected";
        }
        else if (mngr::ConnectState::connecting == status)
        {
            text = "Connecting to " APP_COMFYUI " on " + g::pOptsManager->comfyui_get_host() + ":" + std::to_string(g::pOptsManager->comfyui_get_port());
        }
    }
    else if (m_windows & ViewParsingNodes)
    {
        size_t parsed_item = 0, parse_total = 0;
        m_nodes_parser->check_status(parsed_item, parse_total);
        text = fmt::format(FMT_COMPILE("Parsing node {} of {}"), parsed_item, parse_total);

        if (m_nodes_parser->is_parse_finished())
        {
            g::pOptsManager->nd_release_node_options();
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
    ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->flow_get_color(VisibleObject::DisconnectedText));
    ImGui::TextUnformatted(text.c_str(), text.c_str() + text.size());
    ImGui::PopStyleColor();
}

void GraphN_Flow::show_menu_bar_queue_controls()
{}


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
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        if (m_windows & ViewPanelsResetAll)
        {
            m_windows &= ~ViewPanelsResetAll;
            m_windows |= ViewAllPanels;
            g::pOptsManager->flow_set_open_wnds(m_windows);

            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

            m_main_dock_id = dockspace_id;

            ImGuiID dock_id_left_top = ImGui::DockBuilderSplitNode(m_main_dock_id, ImGuiDir_Left, 0.25f, nullptr, &m_main_dock_id);
            ImGuiID dock_id_left_btm = ImGui::DockBuilderSplitNode(dock_id_left_top, ImGuiDir_Down, 0.5f, nullptr, &dock_id_left_top);

            ImGuiID dock_id_right_top = ImGui::DockBuilderSplitNode(m_main_dock_id, ImGuiDir_Right, 0.25f, nullptr, &m_main_dock_id);
            ImGuiID dock_id_right_bottom = ImGui::DockBuilderSplitNode(dock_id_right_top, ImGuiDir_Down, 0.5f, nullptr, &dock_id_right_top);

            //    const ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(m_main_dock_id, ImGuiDir_Right, 0.25f, nullptr, &m_main_dock_id);
             //   const ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(m_main_dock_id, ImGuiDir_Down, 0.20f, nullptr, &m_main_dock_id);

            ImGui::DockBuilderDockWindow(m_panel_nodes->get_panel_name(), dock_id_left_top);
            ImGui::DockBuilderDockWindow(m_panel_node_inspector->get_panel_name(), dock_id_left_btm);
            ImGui::DockBuilderDockWindow(m_panel_stats->get_panel_name(), dock_id_right_bottom);

            ImGui::DockBuilderFinish(dockspace_id);
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
