
#pragma once

namespace mngr
{
class NodesParser;
}

namespace ui
{
class BaseDlg;
class OptionsDlgs;

enum ViewDialogs
{
	ViewNone			= 0x000000,
	ViewOptions			= 0x000002,
	ViewOptsComfyUi		= 0x000004,
	ViewConnect			= 0x000100,
	ViewParsingNodes	= 0x000200,
	ViewQueueCtrl		= 0x000400,
};

class GraphN_Flow final
{
public:
	GraphN_Flow();
	~GraphN_Flow();

	bool init();
	bool run();
	void exit();

	bool is_offline() const { return m_offline_mode; }

private:
	void stop_all_threads();
	void prepare_to_quit();

	void show_menu_bar();
	void show_menu_bar_connecting();
	void show_menu_bar_queue_controls();
	void show_options_dlg();

	void on_dlg_closed(VisibleObject dlg);
	void on_menu_quit();
	void on_menu_about();

private:
	bool m_quit = false;
	bool m_offline_mode = true;
	
	uint32_t m_windows = ViewNone;
	BaseDlg* m_focused_wnd = nullptr;
	std::set<BaseDlg*> m_all_dialogs;
	std::unique_ptr<OptionsDlgs> m_opts_dlg;
	std::unique_ptr<mngr::NodesParser> m_nodes_parser;
};
}
