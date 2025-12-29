
#include "ui/graph-n_flow.h"
#include "ui/base_dlg.h"
#include "ui/options/opts_dlg.h"
#include "ui/panels/panel_nodes.h"
#include "ui/panels/panel_node_inspector.h"
#include "core/managers/settings_manager.h"
#include "core/managers/connect_manager.h"
#include "core/managers/nodes_manager.h"
#include "core/managers/comfyui_object_validity_listener.h"
#include "ui/workflow/workflow.h"

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

    m_online = false;

    // menu colors
    ImGuiStyle& style = ImGui::GetStyle();

    // Background: Dark Grey (Opaque) - Lighter than main window
    style.Colors[ImGuiCol_PopupBg] = ImColor(IM_COL32(50, 50, 50, 255));
    style.Colors[ImGuiCol_MenuBarBg] = ImColor(IM_COL32(50, 50, 50, 255));

    // open windows
    m_windows = g::pOptsManager->flow_get_open_wnds();

    // create panels
    m_nodes_panel = std::make_unique<ui::PanelNodes>();
    m_nodes_panel->init();
    m_all_dialogs.insert(m_nodes_panel.get());

    m_node_inspector = std::make_unique<ui::PanelNodeInspector>();
    m_node_inspector->init();
    m_all_dialogs.insert(m_node_inspector.get());

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

void GraphN_Flow::add_inspecting_node(data::Node* node)
{
    Log("Name = " << node->name << ", id=" << node->get_id());
    m_node_inspector->add_inspecting_node(node);
}

void GraphN_Flow::remove_inspecting_node(data::Node* node)
{
    Log("Name = " << node->name << ", id=" << node->get_id());
    m_node_inspector->remove_inspecting_node(node);
}

Workflow* GraphN_Flow::create_workflow()
{
    auto wf = std::make_unique<Workflow>();
    auto ptr = wf.get();
    ptr->init();
    m_workflows.emplace(ptr->get_id(), std::move(wf));
    Log("Workflow created, id=" << ptr->get_id());
    return ptr;
}

void GraphN_Flow::destroy_workflow(lid wf_id)
{
    auto it = m_workflows.find(wf_id);
    if (it == m_workflows.end())
    {
        LogAndThrowInternalError(ExcepCategory::inconsistent_map, "Can't destroy workflow id=" << wf_id);
    }
    else
    {
        m_workflows.erase(it);
        Log("Workflow destroyed, id=" << wf_id);
    }
}

Workflow* GraphN_Flow::find_workflow(lid wf_id)
{
    auto it = m_workflows.find(wf_id);
    if (it != m_workflows.end())
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
        listener->resolve_comfyui_objects();
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
            if (ImGui::MenuItem("Debug Metrics##HelpMenuBar", nullptr, (m_windows & ViewDebugMetrics)))
            {
                if (m_windows & ViewDebugMetrics)
                    m_windows &= ~ViewDebugMetrics;
                else
                    m_windows |= ViewDebugMetrics;
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
    default:
        LogError(ExcepCategory::switch_enum, "Unregistered dialog is trying to be closed " << cast::to_szt(dlg));
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
    if (m_nodes_panel->show_dialog(focused) != DlgCode::progress)
    {
        on_dlg_closed(VisibleObject::PanelNodes);
    }
    else if (focused)
        m_focused_wnd = m_nodes_panel.get();
}

void GraphN_Flow::show_panel_node_inspector()
{
    bool focused = false;
    if (m_node_inspector->show_dialog(focused) != DlgCode::progress)
    {
        on_dlg_closed(VisibleObject::PanelNodeInspector);
    }
    else if (focused)
        m_focused_wnd = m_node_inspector.get();
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

            m_nodes_parser = std::make_unique<mngr::NodesParser>();
            m_nodes_parser->parse(std::move(g::pConnectManager->get_nodes_raw()));
            m_online = true;
            return;
        }
        if (mngr::ConnectState::disconnected == status)
        {
            text = "ComfyUI disconnected";
        }
        else if (mngr::ConnectState::connecting == status)
        {
            text = "Connecting to ComfyUI on " + g::pOptsManager->comfyui_get_host() + ":" + std::to_string(g::pOptsManager->comfyui_get_port());
        }
    }
    else if (m_windows & ViewParsingNodes)
    {
        size_t parsed_item = 0, parse_total = 0;
        m_nodes_parser->check_status(parsed_item, parse_total);
        text = fmt::format(FMT_COMPILE("Parsing node {} of {}"), parsed_item, parse_total);

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

    const float text_width = ImGui::CalcTextSize(text.c_str()).x;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float right_align_pos = ImGui::GetWindowWidth() - (text_width + spacing + 10.0f);
    if (right_align_pos > ImGui::GetCursorPosX())
    {
        ImGui::SetCursorPosX(right_align_pos);
    }

    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleColor(ImGuiCol_Text, g::pOptsManager->flow_get_color(VisibleObject::DisconnectedText));
    ImGui::TextUnformatted(text.c_str());
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


            //    const ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(m_main_dock_id, ImGuiDir_Right, 0.25f, nullptr, &m_main_dock_id);
             //   const ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(m_main_dock_id, ImGuiDir_Down, 0.20f, nullptr, &m_main_dock_id);

            ImGui::DockBuilderDockWindow(m_nodes_panel->get_panel_name(), dock_id_left_top);
            ImGui::DockBuilderDockWindow(m_node_inspector->get_panel_name(), dock_id_left_btm);

            ImGui::DockBuilderFinish(dockspace_id);
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(3);

    if (m_windows & ViewPanelNodes)
        show_panel_nodes();
    if (m_windows & ViewPanelNodeInpector)
        show_panel_node_inspector();
}

}
