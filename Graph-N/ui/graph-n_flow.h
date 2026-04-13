
#pragma once
#include "core/managers/connect_manager.h"

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
class SyncProjectsDlg;
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


enum ViewDialogs : uint64_t
{
    ViewNone                    = 0,

    // options
    ViewOptions                 = 1ULL << 0,

    // status & control bar
    ViewConnect                 = 1ULL << 1,
    ViewParsingNodes            = 1ULL << 2,

    // panels
    ViewPanelsResetAll          = 1ULL << 3,
    ViewPanelNodes              = 1ULL << 4,
    ViewPanelNodeInpector       = 1ULL << 5,
    ViewPanelStats              = 1ULL << 6,
    ViewPanelWf                 = 1ULL << 7,
    ViewPanelWfList             = 1ULL << 8,
    ViewPanelModels             = 1ULL << 9,
    ViewPanelTemplates          = 1ULL << 10,
    ViewPanelOutput             = 1ULL << 11,
    ViewPanelNotes              = 1ULL << 12,
    ViewPanelTextEditor         = 1ULL << 13,
    ViewPanelMarkdownPreview    = 1ULL << 14,

    // debug
    ViewDebugMetrics            = 1ULL << 15,
    ViewDebugDemo               = 1ULL << 16,
    ViewDebugLogs               = 1ULL << 17,
    ViewStackTool               = 1ULL << 18,
    ViewStyleEditor             = 1ULL << 19,
    ViewStyleSelector           = 1ULL << 20,
    ViewFontSelector            = 1ULL << 21,

    // top windows
    ViewModal                   = 1ULL << 22,
    ViewModalStop               = 1ULL << 23,

    // bar
    ViewActivityBarL            = 1ULL << 24,
    ViewActivityBarR            = 1ULL << 25,

    ViewSyncProjects            = 1ULL << 26,

    _ViewNextAvailable          = 1ULL << 27,   // UPDATE ON NEW DIALOG
    _ViewLastAvailable          = 1ULL << 63,

    ViewAllMenuBar              = ViewConnect | ViewParsingNodes,
    ViewAllPanels               = ViewPanelNodes | ViewPanelNodeInpector | ViewPanelStats | ViewPanelWf | ViewPanelWfList | ViewPanelModels |
                                  ViewPanelTemplates | ViewPanelOutput | ViewPanelNotes | ViewPanelTextEditor | ViewPanelMarkdownPreview,
    ViewAllDebug                = ViewDebugMetrics | ViewDebugDemo | ViewDebugLogs | ViewStackTool | ViewStyleEditor | ViewStyleSelector | ViewFontSelector,
    ViewAlwaysClose             = ViewAllDebug | ViewModal | ViewOptions | ViewSyncProjects | ViewAllMenuBar | ViewPanelWf,
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

    inline static bool get_is_quit() { return m_quit; }
    inline bool get_online() const { return g::pConnectManager->get_status() == ConnectState::Connected; }

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

    void reload_nodes();
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
    void reparse_nodes();
    void import_user_projects();

    void modal_last_frame();

    void show_menu_bar();
    void show_menu_bar_net_status();

    void show_panels();

    void show_options_dlg();
    void show_sync_projs_dlg();
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
    void on_menu_export_json(bool api);
    void on_menu_export_json_all(bool api);
    void on_menu_close_all_projects();
    void on_menu_sync_projects();

    // WF internal
    void init_workflow_panels();
    PanelWorkflow* find_wf_panel(wid id) const;
    PanelWorkflow* create_wf_panel(const WfData& wf);
    void activate_wf_panel(PanelWorkflow* wf_panel);
    void close_wf_panel(PanelWorkflow* wf_panel);    
    void rename_wf_panel(PanelWorkflow* wf_panel, const std::string& new_name);
    PanelWorkflow* get_last_open_wf_panel(wid closed);
    // WF internal

    // text editor panel internal
    ImGuiID find_def_txt_panel_dock();
    void activate_txt_panels(wid wf);
    void destroy_txt_panels();
    void destroy_txt_panel(PanelTextEditor* panel);

private:
    static std::atomic_bool m_quit;
    std::atomic_bool m_nodes_ready = false;
    std::atomic_bool m_models_ready = false;

    std::unique_ptr<mngr::NodesParser> m_nodes_parser;

    ImGuiID m_main_dock_id = 0;
    uint64_t m_windows = ViewNone;
    std::set<BaseDlg*> m_all_dialogs;

    std::unique_ptr<ModalMessage> m_modal_message;
    std::function<void(ModalDlgCtrl)> m_msg_callback = nullptr;

    std::unique_ptr<OptionsDlgs> m_opts_dlg;
    std::unique_ptr<SyncProjectsDlg> m_sync_projs_dlg;
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
    std::list<wid> m_active_sequense;
    bool m_wf_imported = false;
};
}
