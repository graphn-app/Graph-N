
#include "ui/graph-n_flow.h"
#include "ui/base_dlg.h"
#include "ui/options/opts_dlg.h"
#include "ui/panels/panel_nodes.h"
#include "core/managers/settings_manager.h"
#include "core/managers/connect_manager.h"
#include "core/managers/nodes_manager.h"

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

    m_offline_mode = true;
    m_windows = g::pOptsManager->flow_get_open_wnds();
    g::pConnectManager->connect();
    return true;
}

void GraphN_Flow::exit()
{
    LogF;
    m_quit = true;
}

void GraphN_Flow::stop_all_threads()
{
    LogF;
}

bool GraphN_Flow::run()
{
    if (m_quit)
    {
        prepare_to_quit();
        return false;
    }

    show_panels();
    show_menu_bar();

    if (m_windows & ViewOptions || m_windows & ViewOptsComfyUi)
    {
        show_options_dlg();
    }

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
    const bool connecting_mode = (m_windows & (ViewConnect | ViewParsingNodes)) > 0;
    if (connecting_mode)
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, g::pOptsManager->flow_get_color(VisibleObject::DisconnectedMenuBar));

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File##MainMenuBar"))
        {
            if (ImGui::MenuItem("Quit " APP_CAPTION "##FileMenuBar", "Ctrl+Q"))
            {
                Log("Menu: Quit");
                on_menu_quit();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help##MainMenuBar"))
        {
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
        m_all_dialogs.erase(m_nodes_panel.get());
        m_nodes_panel.reset();
        Log("Nones panel closed");
        break;
    default:
        LogWarn("Unregistered dialog is trying to be closed " << cast::to_szt(dlg));
    }

    g::pOptsManager->flow_set_open_wnds(m_windows);
}

void GraphN_Flow::on_menu_quit()
{
    LogF;
    m_quit = true;
}

void GraphN_Flow::on_menu_about()
{
    LogF;
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
    if (!m_nodes_panel)
    {
        m_nodes_panel = std::make_unique<ui::PanelNodes>();
        m_nodes_panel->init();
        m_all_dialogs.insert(m_nodes_panel.get());
        Log("Nodes panel initialized");
    }

    bool focused = false;
    if (m_nodes_panel->show_dialog(focused) != DlgCode::progress)
    {
        on_dlg_closed(VisibleObject::PanelNodes);
    }
    else if (focused)
        m_focused_wnd = m_nodes_panel.get();
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
        text = tools::str::format("Parsing node %u of %u", parsed_item, parse_total);

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
    ImGui::Text(text.c_str());
    ImGui::PopStyleColor();
}

void GraphN_Flow::show_menu_bar_queue_controls()
{}

void GraphN_Flow::show_panels()
{
    // Use DockSpaceOverViewport - this is the key fix for menu overlap
    // It automatically attaches to the main viewport and respects menu bars/status bars.
    const ImGuiID dockspace_id = ImGui::GetID(APP_CAPTION "DockSpace");

    // ImGuiDockNodeFlags_PassthruCentralNode makes the background of the central node transparent
    // so you can see the window background if no windows are docked there.
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    if (m_windows & ViewPanelsResetAll)
    {
        m_windows |= ViewAllPanels;
        m_windows &= ~ViewPanelsResetAll;

        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

        // Create the split hierarchy
        ImGuiID dock_main_id = dockspace_id;

        // Split Left
        const ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, nullptr, &dock_main_id);

        // Dock Windows
        ImGui::DockBuilderDockWindow("Node Library", dock_id_left);

        ImGui::DockBuilderFinish(dockspace_id);
    }

    if (m_windows & ViewPanelNodes)
        show_panel_nodes();
}

}
