
#pragma once
#include "ui/base_dlg.h"

namespace ui
{

struct StartupData : public DlgData
{
	std::string comfyui_address;
	bool edit_address = false;
};

class StartupDlg : public BaseDlg<StartupData>
{
public:
	StartupDlg() = default;
	~StartupDlg() override;

	DlgCode show(bool& focused) override;
	void init() override;

	void stop_all_threads() override;

private:
	void connect_to_comfyui();
	void connect_check_status();
	void draw_comfyui_address();
	void parse_nodes_check_status();
	void parse_nodes_failed();
	bool parse_new_address();

	enum class State
	{
		do_connect,
		connecting,
		connect_failed,
		connect_ok,
		reading,
		parsing_nodes,
		parse_failed,
		parse_ok,
		exit_ok,
		exit_cancel,
		aborted
	};

	std::pair<State, std::string> connect_thread();
	State parse_thread(std::string data);

private:
	std::atomic<State> m_state = State::do_connect;

	std::thread m_th_connect;
	std::thread m_th_parse;

	std::atomic_bool m_stop_thread = false;
	std::future<std::pair<State, std::string>> m_future_connect;
	std::future<State> m_future_parse;

	struct ParseData
	{
		size_t current = 0;
		size_t total = 0;
		std::string node_name;
	};
	ParseData m_parse_status;
	std::mutex m_mutex;
};

} // namespace ui