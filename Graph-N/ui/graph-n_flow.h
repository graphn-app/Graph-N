
#pragma once

namespace mngr
{
class NodesParser;
struct ComfyUiObjectValidityListener;
}

namespace data
{
struct Node;
class Workflow;
}

namespace ui
{
class BaseDlg;
class OptionsDlgs;
class PanelNodes;
class PanelNodeInspector;
class PanelStats;
class WorkflowDrawer;


enum ViewDialogs
{
    ViewNone                    = 0x00000000,
    // options
    ViewOptions                 = 0x00000001,
    ViewOptsComfyUi             = 0x00000002,
    // status & control bar
    ViewConnect                 = 0x00000004,
    ViewParsingNodes            = 0x00000008,
    ViewQueueCtrl               = 0x00000010,
    // panels
    ViewPanelsResetAll          = 0x00000020,
    ViewPanelNodes              = 0x00000040,
    ViewPanelNodeInpector       = 0x00000080,
    ViewPanelStats              = 0x00000100,

    ViewDebugMetrics            = 0x01000000,
    ViewDebugDemo               = 0x02000000,
    ViewDebugLogs               = 0x04000000,
    ViewStackTool               = 0x08000000,
    ViewStyleEditor             = 0x10000000,
    ViewStyleSelector           = 0x20000000,
    ViewFontSelector            = 0x40000000,

    ViewNextAvail               = 0x00000100,

    ViewAllOptions              = ViewOptions | ViewOptsComfyUi,
    ViewAllMenuBar              = ViewConnect | ViewParsingNodes | ViewQueueCtrl,
    ViewAllPanels               = ViewPanelNodes | ViewPanelNodeInpector | ViewPanelStats,
    ViewAllDebug                = ViewDebugMetrics | ViewDebugDemo | ViewDebugLogs | ViewStackTool | ViewStyleEditor | ViewStyleSelector | ViewFontSelector
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

    data::Workflow* create_workflow();
    void destroy_workflow(lid wf_id);

    WorkflowDrawer* create_workflow_drawer(data::Workflow* wf);

    data::Workflow* find_workflow(lid wf_id);
    WorkflowDrawer* find_workflow_drawer(lid wf_id);

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
    void show_panel_stats();

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
    std::unique_ptr<PanelNodes> m_panel_nodes;
    std::unique_ptr<PanelNodeInspector> m_panel_node_inspector;
    std::unique_ptr<PanelStats> m_panel_stats;

    std::map<lid, std::unique_ptr<data::Workflow>> m_workflows;
    std::map<lid, std::unique_ptr<WorkflowDrawer>> m_workflow_drawers;  // by wf id

    std::list<mngr::ComfyUiObjectValidityListener*> m_comfyui_obj_listeners;
};
}

namespace w
{
extern float pin_l_offset;
extern float pin_r_offset;
extern float ico_circle;
extern float ico_eye;
extern float ico_max;
extern float v_spacing;
extern std::string hidden_widgets;
extern float hidden_widgets_len;
}

#if 1
namespace debug
{
extern const char* test_json;
}
#endif
