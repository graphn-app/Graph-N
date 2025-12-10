
#include "ui/startup_dlg.h"
#include "core/managers/settings_manager.h"
#include "core/managers/nodes_manager.h"

namespace ui
{

StartupDlg::~StartupDlg()
{
	stop_all_threads();
}

void StartupDlg::stop_all_threads()
{
	m_stop_thread = true;
	if (m_th_connect.joinable())
		m_th_connect.join();
	if (m_th_parse.joinable())
		m_th_parse.join();
}

void StartupDlg::init()
{
	LogF;
	BaseDlg::init();

	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	data()->width = 400;
	data()->height = 300;
	data()->x = (main_viewport->WorkSize.x / 2) - (data()->width / 2);
	data()->y = (main_viewport->WorkSize.y / 2) - (data()->height / 2);

	data()->open_popup = true;
	data()->edit_address = false;
	data()->comfyui_address = g::pOptsManager->comfyui_get_host() + ":" + std::to_string(g::pOptsManager->comfyui_get_port());
}

DlgCode StartupDlg::show(bool& focused)
{
	ImGui::SetNextWindowPos({ data()->x, data()->y }, ImGuiCond_Always);
	ImGui::SetNextWindowSize({ data()->width, data()->height }, ImGuiCond_Always);

	if (data()->open_popup)
		ImGui::OpenPopup("Initialization");

	if (!ImGui::BeginPopupModal("Initialization", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize))
		return cleanup_and_exit_popup(false, true, DlgCode::progress);	
	focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
	data()->open_popup = false;

	DlgCode ret = DlgCode::progress;
	switch (m_state)
	{
	case State::do_connect:
		connect_to_comfyui();
		break;
	case State::reading:
	case State::connecting:
		connect_check_status();
		break;
	case State::parsing_nodes:
		parse_nodes_check_status();
		break;
	case State::connect_failed:
	case State::parse_failed:
		parse_nodes_failed();
		break;
	case State::exit_ok:
		ret = DlgCode::ok;
		break;
	case State::exit_cancel:
	case State::aborted:
		ret = DlgCode::cancel;
		break;
	}

	return cleanup_and_exit_popup(true, true, ret);
}

void StartupDlg::connect_to_comfyui()
{
	LogF;
	draw_comfyui_address();

	m_stop_thread = false;

	std::packaged_task<std::pair<State, std::string>()> task(std::bind(&StartupDlg::connect_thread, this));
	m_future_connect = task.get_future();
	m_th_connect = std::thread(std::move(task));
	m_state = State::connecting;
}

void StartupDlg::draw_comfyui_address()
{
	ImGui::SetCursorPosY(20);
	ImGui::Indent(20);
	ImGui::Text("Connecting to ComfyUI on");

	if (data()->set_focus)
	{
		data()->set_focus = false;
		ImGui::SetKeyboardFocusHere();
	}
	const bool apply_addr = ImGui::InputTextWithHint("ComfyUIAddr", "hostname:port", &data()->comfyui_address, 
		ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue |
		(data()->edit_address ? ImGuiInputTextFlags_None : ImGuiInputTextFlags_ReadOnly));

	const auto allow_edit = m_state != State::reading; // if we're already connected and now reading
	if (!allow_edit)
		ImGui::BeginDisabled();
	ImGui::SameLine();
	if (!data()->edit_address)
	{
		if (ImGui::Button("Edit"))
		{
			data()->set_focus = true;
			data()->edit_address = true;
			Log("Edit address");
		}
	}
	else
	{
		if (ImGui::Button("Connect") || apply_addr)
		{
			if (parse_new_address())
			{
				data()->edit_address = false;
				m_state = State::do_connect;
				Log("Reconnect");
			}
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel and exit"))
	{
		stop_all_threads();
		m_state = State::exit_cancel;
		Log("User canceled");
	}
	if (!allow_edit)
		ImGui::EndDisabled();
}

bool StartupDlg::parse_new_address()
{
	const auto parsed = tools::str::split(data()->comfyui_address, ':', false /*add_empty*/);
	if (parsed.size() != 2)
		return false;
	const auto host = parsed.front();
	const auto port = strtoul(parsed.back().c_str(), nullptr, 10);
	if (!host.empty() && port > 0 && port <= USHRT_MAX)
	{
		stop_all_threads();
		g::pOptsManager->comfyui_set_host(host);
		g::pOptsManager->comfyui_set_port(cast::to_u16(port));
		Log("New address: " << g::pOptsManager->comfyui_get_host() << ":" << g::pOptsManager->comfyui_get_port());
		return true;
	}
	return false;
}

void StartupDlg::connect_check_status()
{
	draw_comfyui_address();

	using namespace std::chrono_literals;
	if (m_future_connect.wait_for(1ms) == std::future_status::ready)
	{
		if (m_th_connect.joinable())
			m_th_connect.join();

		auto data = m_future_connect.get();	
		if (State::connect_ok == data.first)
		{
			m_stop_thread = false;

			std::packaged_task<State()> task(std::bind(&StartupDlg::parse_thread, this, std::move(data.second)));
			m_future_parse = task.get_future();
			m_th_parse = std::thread(std::move(task));
			m_state = State::parsing_nodes;
		}

		Log("Next step " << cast::to_i32(m_state.load()));
	}
}

void StartupDlg::parse_nodes_check_status()
{
	ImGui::SetCursorPosY(20);
	ImGui::Indent(20);

	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (0 == m_parse_status.total)
			ImGui::Text("Parsing nodes...");
		else
			ImGui::Text("Parsing node %u of %u: %s", m_parse_status.current, m_parse_status.total, m_parse_status.node_name.c_str());
	}

	using namespace std::chrono_literals;
	if (m_future_parse.wait_for(1ms) == std::future_status::ready)
	{
		if (m_th_parse.joinable())
			m_th_parse.join();

		const State status = m_future_parse.get();
		if (State::parse_ok == status)
		{
			Log("Nodes parsed successfully");
			m_state = State::exit_ok;
			m_stop_thread = true;
		}

		Log("Next step " << cast::to_i32(m_state.load()));
	}
}

void StartupDlg::parse_nodes_failed()
{
	ImGui::SetCursorPosY(20);
	ImGui::Indent(20);

	ImGui::SetCursorPosY(20);
	ImGui::Indent(20);
	ImGui::Text("Failed to connect to ComfyUI or parse nodes data, received from ComfyUI.");
	ImGui::Text("ComfyUI might be located at a different address.");
	ImGui::Text("Please specify the correct ComfyUI address:");

	const bool apply_addr = ImGui::InputTextWithHint("ComfyUIAddr", "hostname:port", &data()->comfyui_address,
		ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);

	ImGui::SameLine();
	if (ImGui::Button("Connect") || apply_addr)
	{
		if (parse_new_address())
		{
			data()->edit_address = false;
			stop_all_threads();
			m_state = State::do_connect;
			Log("Reconnect");
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Exit"))
	{
		stop_all_threads();
		m_state = State::exit_cancel;
		Log("User canceled");
	}
}

std::pair<StartupDlg::State, std::string> StartupDlg::connect_thread()
{
	try
	{
		using namespace std::chrono_literals;
		while (!m_stop_thread)
		{
			const auto host = g::pOptsManager->comfyui_get_host();
			const auto port = g::pOptsManager->comfyui_get_port();

			httplib::Client cli(host, port);
			cli.set_keep_alive(true);
			cli.set_connection_timeout(g::pOptsManager->comfyui_get_connect_timeout());
			cli.set_read_timeout(g::pOptsManager->comfyui_get_read_timeout());

			Log("Connecting to ComfyUI on " << host << ":" << port << "...");
			auto res = cli.Get("/");
			if (!res)
			{
			//	Log("Connection failed. Error code " << httplib::to_string(res.error()));
				std::this_thread::sleep_for(1ms);
				continue;
			}			

			if (m_stop_thread)
			{
				Log("Connection thread aborted");
				return std::make_pair(State::aborted, "");
			}

			Log("Next step - reading nodes data");
			m_state = State::reading;

			res = cli.Get(comfyui::object_info);
			if (res && 200 == res->status)
			{
				Log("Read " << res->body.size() << " bytes");
				return std::make_pair(State::connect_ok, std::move(res->body));
			}
			Log("Reading failed. Error code " << httplib::to_string(res.error()));

			std::this_thread::sleep_for(1ms);
		} 
	}
	catch(const std::exception& e)
	{
		LogExcep("Connection failed with err " << e.what());
		return std::make_pair(State::connect_failed, "");
	}

	if (m_stop_thread)
	{
		Log("Connection thread aborted");
		return std::make_pair(State::aborted, "");
	}

	LogAndThrow("Unexpected exit flow");
}

StartupDlg::State StartupDlg::parse_thread(std::string data)
{
	using json = nlohmann::json;

	try
	{
		const json nodes = json::parse(data);

		m_parse_status.current = 0;
		m_parse_status.total = nodes.size();

		// UI is blocked and hidden, we can safely clear all nodes and add new
		g::pNodesManager->free();

		for (auto&[node_name, node_info] : nodes.items())
		{
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				m_parse_status.current++;
				m_parse_status.node_name = node_name;
			}

			g::pNodesManager->add_node(node_name, node_info.dump());

			if (m_stop_thread)
			{
				Log("Parsing thread aborted");
				return State::aborted;
			}
		}

		return State::parse_ok;
	}
	catch (json::exception& e)
	{
		LogExcep("Nodes parsing failed with err " << e.what());
		return State::parse_failed;
	}
}

} // namespace ui