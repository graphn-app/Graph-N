
#include "ui/graph-n_flow.h"
#include "ui/base_dlg.h"
#include "ui/options/opts_dlg.h"
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
	m_windows = g::pOptsManager->flow_get_open_wnds() | ViewQueueCtrl | ViewConnect;
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
		m_windows &= ~(ViewOptions | ViewOptsComfyUi);
		m_all_dialogs.erase(m_opts_dlg.get());
		m_opts_dlg.reset();
		Log("Options dlg closed");
		break;
	default:
		LogWarn("Unregistered dialog is trying to be closed " << cast::to_szt(dlg));
	}
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
		std::string status;
		auto items = m_nodes_parser->check_status(status);
		text = tools::str::format("Parsing node %u of %u: %s", items.first, items.second, status.c_str());

		if (m_nodes_parser->is_parse_finished())
		{
			m_windows &= ~ViewParsingNodes;
			m_nodes_parser.reset();
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

}
