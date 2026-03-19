
#pragma once

namespace mngr
{
class NodesParser;
}

namespace data
{
struct Node;
class NodeInstance;
class PinInstance;
}

namespace ui
{
class BaseDlg;
class OptionsDlgs;
class ModalMessage;
class ActivityBar;
class PanelNodes;
class PanelNodeInspector;
class PanelStats;
class PanelWorkflow;
class PanelWfList;
class PanelTemplates;
class PanelServerLogs;
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

    ViewModal                   = 0x00200000,
    ViewModalStop               = 0x00400000,

    ViewActivityBarL            = 0x00800000,
    ViewActivityBarR            = 0x01000000,

    _ViewNextAvail              = 0x02000000,

    ViewAllMenuBar              = ViewConnect | ViewParsingNodes | ViewQueueCtrl,
    ViewAllPanels               = ViewPanelNodes | ViewPanelNodeInpector | ViewPanelStats | ViewPanelWf | ViewPanelWfList | ViewPanelModels |
                                  ViewPanelTemplates | ViewPanelOutput | ViewPanelNotes | ViewPanelTextEditor | ViewPanelMarkdownPreview,
    ViewAllDebug                = ViewDebugMetrics | ViewDebugDemo | ViewDebugLogs | ViewStackTool | ViewStyleEditor | ViewStyleSelector | ViewFontSelector,
    ViewAlwaysClose             = ViewAllDebug | ViewModal | ViewOptions | ViewAllMenuBar | ViewPanelWf
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

    inline ImGuiID get_main_dockspace_id() { return m_main_dock_id; }

    void reconnect();
    void handle_comfyui_runserver_error(int32_t error, const std::string& title, const std::string& description);

    void open_settings_dlg(OptPane section = OptPane::_count);
    void show_logs_panel(bool show);

    void load_activity_bar_mode();

    bool is_panel_active(DlgObj panel_obj) const;
    const char* get_panel_icon(DlgObj panel_obj) const;
    const char* get_panel_name(DlgObj panel_obj) const;
    void toggle_panel_visibility(DlgObj panel_obj);

    inline void set_nodes_ready() { m_nodes_ready = true; }
    inline bool get_nodes_ready() const { return m_nodes_ready; }
    inline void set_models_ready() { m_models_ready = true; }
    inline bool get_models_ready() const { return m_models_ready; }

    // WF public
    void wf_create();
    void wf_create_copy_from(wid source, std::optional<ImRect> wf_rc, std::optional<float> wf_zoom);
    void wf_rebuild(wid id);
    void wf_activate(wid id);
    void wf_locate(wid id);
    void wf_delete(wid id);
    void wf_restore(wid id);
    void wf_rename(wid id, const std::string& new_name);
    void wf_add_node_to_active_panel(data::Node* node, const std::string& select = "");
    void wf_locate_node(wid id, data::NodeInstance* node);

    // text editor panel public
    void txt_panel_create(wid wf, data::NodeInstance* node, data::PinInstance* pin, std::string* buffer, const char* hint);
    void txt_panel_delete(wid wf, data::NodeInstance* node);
    void txt_panel_clear(wid wf);

    // modal msg box
    void modal_begin(std::string&& caption, std::string&& message, int32_t buttons, ModalDlgCtrl def, const std::function<void(ModalDlgCtrl)>& callback);
    void modal_show();
    void modal_update(std::string&& message);
    void modal_stop();

private:
    void stop_all_threads();
    void prepare_to_quit();

    void check_connection();
    void handle_connected();

    void modal_last_frame();

    void show_menu_bar();
    void show_menu_bar_connecting();
    void show_menu_bar_queue_controls();

    void show_panels();

    void show_options_dlg();
    void show_activity_bar();
    void show_debug_windows();
    void show_modal();

    void show_panel(BaseDlg* panel, DlgObj obj);
    void show_panel_nodes();
    void show_panel_node_inspector();
    void show_panel_stats();
    void show_panel_wf();
    void show_panel_text_editor();
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
    PanelWorkflow* create_wf_panel(const WfData& wf);
    void activate_wf_panel(PanelWorkflow* wf_panel);
    void close_wf_panel(PanelWorkflow* wf_panel);    
    void rename_wf_panel(PanelWorkflow* wf_panel, const std::string& new_name);
    // WF internal

    // text editor panel internal
    ImGuiID find_def_txt_panel_dock();
    void activate_txt_panels(wid wf);
    void destroy_txt_panels();
    void destroy_txt_panel(PanelTextEditor* panel);

private:
    bool m_online = false;

    std::atomic_bool m_nodes_ready = false;
    std::atomic_bool m_models_ready = false;

    std::unique_ptr<mngr::NodesParser> m_nodes_parser;

    ImGuiID m_main_dock_id = 0;
    uint64_t m_windows = ViewNone;
    std::set<BaseDlg*> m_all_dialogs;

    std::unique_ptr<ModalMessage> m_modal_message;
    std::function<void(ModalDlgCtrl)> m_msg_callback = nullptr;

    std::unique_ptr<OptionsDlgs> m_opts_dlg;
    std::unique_ptr<ActivityBar> m_activity_bar;
    ActivityBarMode m_activity_bar_mode;

    std::unique_ptr<PanelNodes> m_panel_nodes;
    std::unique_ptr<PanelNodeInspector> m_panel_node_inspector;
    std::unique_ptr<PanelStats> m_panel_stats;
    std::unique_ptr<PanelTemplates> m_panel_templates;
    std::unique_ptr<PanelServerLogs> m_panel_logs;
    //std::unique_ptr<PanelNotes> m_panel_notes;
    std::unique_ptr<PanelModels> m_panel_models;
    std::unique_ptr<PanelWfList> m_panel_wf_list;

    PanelWorkflow* m_wf_focused = nullptr;
    std::list<std::unique_ptr<PanelWorkflow>> m_panels_wf;
    std::list<std::unique_ptr<PanelTextEditor>> m_panels_text_editor;
};
}
