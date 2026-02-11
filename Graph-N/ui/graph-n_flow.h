
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
class PanelStats;
class PanelWorkflow;
class PanelWfList;
class PanelTemplates;
class PanelOutput;
class PanelNotes;
class PanelModels;
class PanelTextEditor;
class PanelMarkdownPreview;


enum ViewDialogs
{
    ViewNone                    = 0x00000000,
    // options
    ViewOptions                 = 0x00000001,
    // status & control bar
    ViewConnect                 = 0x00000004,
    ViewParsingNodes            = 0x00000008,
    ViewQueueCtrl               = 0x00000010,
    // panels
    ViewPanelsResetAll          = 0x00000020,
    ViewPanelNodes              = 0x00000040,
    ViewPanelNodeInpector       = 0x00000080,
    ViewPanelStats              = 0x00000100,
    ViewPanelWf                 = 0x00000800,
    ViewPanelWfList             = 0x00001000,
    ViewPanelModels             = 0x00002000,
    ViewPanelTemplates          = 0x00004000,
    ViewPanelOutput             = 0x00008000,
    ViewPanelNotes              = 0x00010000,
    ViewPanelTextEditor         = 0x00080000,
    ViewPanelMarkdownPreview    = 0x00100000,

    ViewDebugMetrics            = 0x01000000,
    ViewDebugDemo               = 0x02000000,
    ViewDebugLogs               = 0x04000000,
    ViewStackTool               = 0x08000000,
    ViewStyleEditor             = 0x10000000,
    ViewStyleSelector           = 0x20000000,
    ViewFontSelector            = 0x40000000,

    _ViewNextAvail              = 0x00200000,

    ViewAllMenuBar              = ViewConnect | ViewParsingNodes | ViewQueueCtrl,
    ViewAllPanels               = ViewPanelNodes | ViewPanelNodeInpector | ViewPanelStats | ViewPanelWf | ViewPanelWfList | ViewPanelModels |
                                  ViewPanelTemplates | ViewPanelOutput | ViewPanelNotes | ViewPanelTextEditor | ViewPanelMarkdownPreview,
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

    inline bool get_online() const { return m_online; }
    inline void set_online(bool online) { m_online = online; }

    void reconnect();
    void handle_comfyui_runserver_error(int32_t error, const std::string& title, const std::string& description);

    void open_settings_dlg(OptPane section = OptPane::_count);

    void subscribe_comfyui_validity_state(mngr::ComfyUiObjectValidityListener* listener);

    void invalidate_comfyui_objects();
    void resolve_comfyui_objects();

    // WF public
    void wf_create();
    void wf_activate(wid id);
    void wf_locate(wid id);
    void wf_delete(wid id);
    void wf_restore(wid id);
    void wf_rename(wid id, const std::string& new_name);
    void wf_add_node_to_active_panel(data::Node* node, const std::string& select = "");
    // WF public

private:
    void stop_all_threads();
    void prepare_to_quit();

    void show_menu_bar();
    void show_menu_bar_connecting();
    void show_menu_bar_queue_controls();
    void show_options_dlg();

    void show_panels();
    void show_panel(BaseDlg* panel, DlgObj obj);
    void show_panel_nodes();
    void show_panel_node_inspector();
    void show_panel_stats();
    void show_panel_wf();
    void show_panel_wf_list();
    void show_panel_models();
    void show_panel_output();
    void show_panel_templates();
    void show_panel_notes();

    void on_dlg_closed(DlgObj dlg);

    void on_menu_options();
    void on_menu_quit();
    void on_menu_about();
    void on_menu_panel_window(ViewDialogs wnd);
    void on_menu_wf(PanelWorkflow* wf_panel);
    void on_menu_import_json();
    void on_menu_export_json();

    // WF internal
    void init_workflow_panels();
    PanelWorkflow* find_wf_panel(wid id) const;
    PanelWorkflow* create_wf_panel(const WfInfo& wf);
    void activate_wf_panel(PanelWorkflow* wf_panel);
    void close_wf_panel(PanelWorkflow* wf_panel);
    void rename_wf_panel(PanelWorkflow* wf_panel, const std::string& new_name);
    // WF internal

private:
    bool m_online = false;

    ImGuiID m_main_dock_id = 0;

    std::unique_ptr<mngr::NodesParser> m_nodes_parser;

    uint64_t m_windows = ViewNone;
    std::set<BaseDlg*> m_all_dialogs;

    std::unique_ptr<OptionsDlgs> m_opts_dlg;
    std::unique_ptr<PanelNodes> m_panel_nodes;
    std::unique_ptr<PanelNodeInspector> m_panel_node_inspector;
    std::unique_ptr<PanelStats> m_panel_stats;
    std::unique_ptr<PanelTemplates> m_panel_templates;
    std::unique_ptr<PanelOutput> m_panel_output;
    std::unique_ptr<PanelNotes> m_panel_notes;
    std::unique_ptr<PanelModels> m_panel_models;
    std::unique_ptr<PanelWfList> m_panel_wf_list;

    PanelWorkflow* m_wf_focused = nullptr;
    std::list<std::unique_ptr<PanelWorkflow>> m_panels_wf;

    std::list<mngr::ComfyUiObjectValidityListener*> m_comfyui_obj_listeners;
};
}
