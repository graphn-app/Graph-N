
#pragma once

namespace mngr
{
class NodesParser;
}

namespace ui
{
class BaseDlg;
class OptionsDlgs;
class PanelNodes;

enum ViewDialogs
{
	ViewNone				= 0x00000000,
	// options
	ViewOptions				= 0x00000001,
	ViewOptsComfyUi			= 0x00000002,
	// status & control bar
	ViewConnect				= 0x00000100,
	ViewParsingNodes		= 0x00000200,
	ViewQueueCtrl			= 0x00000400,
	// panels
	ViewPanelsResetAll		= 0x00010000,
	ViewPanelNodes			= 0x00020000,

	ViewAllOptions			= ViewOptions | ViewOptsComfyUi,
	ViewAllMenuBar			= ViewConnect | ViewParsingNodes | ViewQueueCtrl,
    ViewAllPanels			= ViewPanelNodes
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
    void show_panels();
    void show_panel_nodes();

	void on_dlg_closed(VisibleObject dlg);
	void on_menu_quit();
	void on_menu_about();

private:
	bool m_offline_mode = true;

    ImGuiID m_main_dock_id = 0;
	
	uint64_t m_windows = ViewNone;
	BaseDlg* m_focused_wnd = nullptr;
	std::set<BaseDlg*> m_all_dialogs;
	std::unique_ptr<OptionsDlgs> m_opts_dlg;
    std::unique_ptr<PanelNodes> m_nodes_panel;

	std::unique_ptr<mngr::NodesParser> m_nodes_parser;
};
}
