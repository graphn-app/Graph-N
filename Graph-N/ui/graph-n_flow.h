
#pragma once

namespace mngr
{
class NodesParser;
struct ComfyUiObjectValidityListener;
}

namespace data
{
struct Node;
}

namespace ui
{
class BaseDlg;
class OptionsDlgs;
class PanelNodes;
class PanelNodeInspector;

class Workflow;

enum ViewDialogs
{
    ViewNone = 0x00000000,
    // options
    ViewOptions = 0x00000001,
    ViewOptsComfyUi = 0x00000002,
    // status & control bar
    ViewConnect = 0x00000100,
    ViewParsingNodes = 0x00000200,
    ViewQueueCtrl = 0x00000400,
    // panels
    ViewPanelsResetAll = 0x00010000,
    ViewPanelNodes = 0x00020000,
    ViewPanelNodeInpector = 0x00040000,

    ViewAllOptions = ViewOptions | ViewOptsComfyUi,
    ViewAllMenuBar = ViewConnect | ViewParsingNodes | ViewQueueCtrl,
    ViewAllPanels = ViewPanelNodes | ViewPanelNodeInpector,

#ifdef _DEBUG
    ViewDebugMetrics = 0x80000000
#endif
};

class GraphN_Flow final
{
public:
    GraphN_Flow();
    ~GraphN_Flow();

    NO_COPY_MOVE(GraphN_Flow);

    bool init();
    bool run();
    void exit();

    bool get_online() const { return m_online; }
    void set_online(bool online) { m_online = online; }

    void reconnect();

    Workflow* create_workflow();
    void destroy_workflow(lid wf_id);
    Workflow* find_workflow(lid wf_id);

    void subscribe_comfyui_validity_state(mngr::ComfyUiObjectValidityListener* listener);

    void invalidate_comfyui_objects();
    void resolve_comfyui_objects();

private:
    void stop_all_threads();
    void prepare_to_quit();

    void show_menu_bar();
    void show_menu_bar_connecting();
    void show_menu_bar_queue_controls();
    void show_options_dlg();
    void show_panels();
    void show_panel_nodes();
    void show_panel_node_inspector();

    void on_dlg_closed(VisibleObject dlg);

    void on_menu_options();
    void on_menu_quit();
    void on_menu_about();
    void on_menu_panel_window(ViewDialogs wnd);

private:
    bool m_online = false;

    ImGuiID m_main_dock_id = 0;

    std::unique_ptr<mngr::NodesParser> m_nodes_parser;

    uint64_t m_windows = ViewNone;
    BaseDlg* m_focused_wnd = nullptr;
    std::set<BaseDlg*> m_all_dialogs;

    std::unique_ptr<OptionsDlgs> m_opts_dlg;
    std::unique_ptr<PanelNodes> m_nodes_panel;
    std::unique_ptr<PanelNodeInspector> m_node_inspector;

    std::map<lid, std::unique_ptr<Workflow>> m_workflows;

    std::list<mngr::ComfyUiObjectValidityListener*> m_comfyui_obj_listeners;
};
}
