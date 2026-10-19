#pragma once
#include "core/listeners.h"
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
class WorkflowAction;
}

namespace ui
{
class BaseDlg;
class OptionsDlgs;
class ModalMessage;
class ActivityBar;
class SyncProjectsDlg;
class CustomMediaWidget;
class CustomJsNode;
class PanelNodes;
class PanelNodeInspector;
class PanelStats;
class PanelWorkflow;
class PanelWfList;
class PanelTemplates;
class PanelServerLogs;
class PanelNotes;
class PanelModels;
class PanelQueue;
class PanelTextEditor;
class PanelMarkdownPreview;
#ifdef _DEBUG
class DebugWfView;
#endif


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
    //ViewPanelTemplates          = 1ULL << 10,
    ViewPanelOutput             = 1ULL << 11,
    //ViewPanelNotes              = 1ULL << 12,
    ViewPanelTextEditor         = 1ULL << 13,
    //ViewPanelMarkdownPreview    = 1ULL << 14,
    ViewPanelQueue              = 1ULL << 27,

    // debug
    ViewDebugMetrics            = 1ULL << 15,
    ViewDebugDemo               = 1ULL << 16,
    ViewDebugLogs               = 1ULL << 17,
    ViewStackTool               = 1ULL << 18,
    ViewStyleEditor             = 1ULL << 19,
    ViewStyleSelector           = 1ULL << 20,
    ViewFontSelector            = 1ULL << 21,
    ViewWfDebug                 = 1ULL << 28,

    // top windows
    ViewModal                   = 1ULL << 22,
    ViewModalStop               = 1ULL << 23,

    // bar
    ViewActivityBarL            = 1ULL << 24,
    ViewActivityBarR            = 1ULL << 25,

    ViewSyncProjects            = 1ULL << 26,

    _ViewNextAvailable          = 1ULL << 29,   // UPDATE ON NEW DIALOG

    ViewAllMenuBar              = ViewConnect | ViewParsingNodes,
    ViewAllPanels               = ViewPanelNodes | ViewPanelNodeInpector | ViewPanelStats | ViewPanelWfList | ViewPanelModels | /*ViewPanelTemplates | */ViewPanelOutput | ViewPanelQueue /*| ViewPanelNotes*/,
    ViewAllDebug                = ViewDebugMetrics | ViewDebugDemo | ViewDebugLogs | ViewStackTool | ViewStyleEditor | ViewStyleSelector | ViewFontSelector | ViewWfDebug,
    ViewAlwaysClose             = ViewAllDebug | ViewModal | ViewOptions | ViewSyncProjects | ViewAllMenuBar | ViewPanelWf,
};

class GraphN_Flow final
    : public IsAlive
    , public listener::Timer
{
public:
    GraphN_Flow() noexcept;
    ~GraphN_Flow() noexcept;

    bool init();
    bool run();
    void prepare_to_quit();
    void quit();

    void cleanup_frame();

    static bool get_is_quit() noexcept { return g::app_global_stop_all_threads.stop_requested(); }
    bool get_online() const noexcept { return g::pConnectManager->get_status() == ConnectState::Connected; }

    ImGuiID get_main_dockspace_id() noexcept { return m_main_dock_id; }

    void reconnect();
    void handle_comfyui_runserver_error(int32_t error, std::string&& title, std::string&& description) noexcept;

    void open_settings_dlg(OptPane section = OptPane::_count);
    void show_logs_panel(bool show) noexcept;

    void load_activity_bar_mode() noexcept;
    void load_hotkeys();

    void set_toast(ImGuiEx::Toast&& toast) noexcept { m_toast = std::move(toast); }

    bool is_panel_active(DlgObj panel_obj) const noexcept;
    const char* get_panel_icon(DlgObj panel_obj) const noexcept;
    const char* get_panel_name(DlgObj panel_obj) const noexcept;
    void toggle_panel_visibility(DlgObj panel_obj);
    void set_panel_visibility(DlgObj panel_obj, bool show_panel);

    void reload_nodes();
    void set_nodes_ready() noexcept{ m_nodes_ready.store(true, std::memory_order_relaxed); }
    bool get_nodes_ready() const noexcept { return m_nodes_ready.load(std::memory_order_relaxed); }
    void set_models_ready() noexcept { m_models_ready.store(true, std::memory_order_relaxed); }
    bool get_models_ready() const noexcept { return m_models_ready.load(std::memory_order_relaxed); }
    bool get_projects_ready() const noexcept { return m_projects_ready.load(std::memory_order_relaxed); }

    // http requests
    uint32_t http_get(HttpRequestor requestor, const StillAliveCallback& alive, const char* request, const HttpResponseCb& callback, bool retry_on_timeout = false);
    uint32_t http_get(HttpRequestor requestor, const StillAliveCallback& alive, const char* request, httplib::Params&& params, httplib::Headers&& headers, const HttpResponseCb& callback, bool retry_on_timeout = false);
    int32_t http_get_and_wait(HttpRequestor requestor, const char* request, httplib::Params&& params, httplib::Headers&& headers, bool retry_on_timeout, const std::function<bool()>& stop_callback, std::string& ret_data);
    uint32_t http_post(HttpRequestor requestor, const StillAliveCallback& alive, const char* request, std::string&& payload, const HttpResponseCb& callback, bool retry_on_timeout = false);
    uint32_t http_post(HttpRequestor requestor, const StillAliveCallback& alive, const char* request, httplib::UploadFormDataItems&& payload, const HttpResponseCb& callback, bool retry_on_timeout = false);    

    void check_for_updates(const HttpResponseCb& callback);
    void stats_post_nodes(std::unordered_set<std::string>&& resolved_nodes, std::unordered_set<std::string>&& unresolved_nodes, std::unordered_set<std::string>&& python_modules);
    void stats_post_config(const char* property, const char* value);

    // host manager
    void host_open_mask_editor(wid wf_id, iid node_id);
    void host_open_3dpos_editor(wid wf_id, iid node_id);
    void host_handle_node_removal(wid wf_id, iid node_id);
    void host_handle_wf_closure(wid wf_id);
    void host_handle_renaming(wid wf_id, iid node_id);

    // WF
    PanelWorkflow* wf_get_active_panel() const noexcept { return m_wf_focused; }
    void wf_create();
    void wf_create_copy_from(wid source, std::optional<ImRect> wf_rc, std::optional<float> wf_zoom);
    void wf_reload_view(wid id) noexcept;
    void wf_activate(wid id);
    //void wf_locate(wid id);
    void wf_delete(wid id);
    void wf_restore(wid id);
    void wf_rename(wid id, const std::string& new_name);
    void wf_add_node_to_active_panel(data::Node* node, const std::string& select = "");
    void wf_locate_node(wid id, data::NodeInstance* node);
    std::set<data::NodeInstance*> wf_get_nodes_in_group(wid wf_id, data::NodeInstance* group);
    void wf_reload();
    void wf_rename() noexcept;
    wid wf_get_active(std::set<wid>* all_opened = nullptr) const noexcept;
    bool wf_can_run() const noexcept;
    void wf_run(bool add_front) const;
    void wf_num_of_runs(int32_t num) const noexcept;
    void wf_panel_activate_sequential(size_t sequential_number);
    bool wf_save(bool check_only);
    bool wf_undo(bool check_only);
    bool wf_redo(bool check_only);
    void wf_focus() noexcept;
    void wf_node_select_all() noexcept;
    void wf_panel_close(PanelWorkflow* wf_panel);
    void wf_new_node() noexcept;
    void wf_node_toggle_pin();
    void wf_node_toggle_collapse();
    void wf_node_group();
    void wf_node_always();
    void wf_node_toggle_mute();
    void wf_node_toggle_bypass();
    void wf_node_remove();
    void wf_node_copy();
    bool wf_node_paste(bool check_only);
    void wf_node_duplicate();
    void wf_node_move(bool left, bool right, bool up, bool down);
    void wf_node_align(bool left, bool top);
    void wf_node_unlink();
    void wf_insert_reroute();

    // menu
    void on_menu_import();
    void on_menu_quit() noexcept;
    void on_menu_fullscreen();
    void on_menu_command() noexcept;
    void on_menu_restart() noexcept;
    void on_menu_options();
    void on_menu_panel_toggle_active() noexcept;
    void on_menu_next_project(bool next);
    void on_menu_panel_window(ViewDialogs wnd, bool toggle = false);   
    void on_menu_activate_wf(const PanelWorkflow* wf_panel);
    void on_menu_wf_rename() noexcept;
    void on_menu_close_all_projects(bool keep_active);
    void on_menu_queue_pause() noexcept;
    void on_menu_queue_interrupt();

    // text editor panel public
    void txt_panel_create(wid wf, data::NodeInstance* node, data::PinInstance* pin, std::string* buffer, const char* hint);
    void txt_panel_update(wid wf, data::NodeInstance* node);
    void txt_panel_delete(wid wf, const data::NodeInstance* node = nullptr, const data::PinInstance* pin = nullptr) noexcept;
    void txt_panel_clear(wid wf) noexcept;

    // custom nodes handling
    CustomMediaWidget& custom_media_widgets() { return *m_custom_media_widget.get(); }
    CustomJsNode& custom_js_nodes() { return *m_custom_js_node.get(); }

    // modal msg box
    void modal_begin(std::string&& caption, std::string&& message, int32_t buttons, ModalDlgCtrl def, const std::function<void(ModalDlgCtrl)>& callback);
    void modal_show();
    void modal_update(std::string&& message);
    void modal_stop() noexcept;

    // web socket
    void check_ws_message(const std::function<void(bool, std::string&)>& callback);

    // stats
    mngr::StatsData& get_stats() { return m_stats; }

private:
    void check_connection();
    void check_http_reply();
    void check_host_reply();
    void handle_connected();
    void reparse_nodes();
    void import_user_projects();
    void import_files(std::list<std::filesystem::path>&& paths);
    bool check_wf_panels_initialized();

    void modal_last_frame();

    void show_menu_bar();
    void show_menu_bar_net_status();

    void show_panels();

    void show_options_dlg();
    void show_sync_projs_dlg();
    void show_activity_bar();
    void show_debug_windows() noexcept;
    void show_modal();
    void show_wf_debug_dlg();

    void show_panel(BaseDlg* panel, DlgObj obj);
    void show_panel_nodes();
    void show_panel_node_inspector();
    void show_panel_stats();
    void show_panel_wf();
    void show_panel_text_editor();
    void show_panel_wf_list();
    void show_panel_models();
    void show_panel_queue();
    void show_panel_output();
    //void show_panel_templates();
    //void show_panel_notes();

    void on_dlg_closed(DlgObj dlg);

    void on_menu_export_json(bool api);
    void on_menu_export_json_all(bool api);
    void on_menu_sync_projects();
    void on_menu_update() noexcept;
    void on_menu_about() noexcept;

    // WF & WF panels
    PanelWorkflow* wf_panel_find(wid id) const noexcept;
    PanelWorkflow* wf_panel_create(const mngr::WfData& wf);
    void wf_panel_activate(PanelWorkflow* wf_panel);
    bool wf_panel_activate_next(bool next);
    void wf_panel_rename(PanelWorkflow* wf_panel, const std::string& new_name);
    void wf_assign_sequential_numbers();
    void wf_node_get_any_selected(bool& sel_nodes, bool& sel_groups) noexcept;
    void wf_node_action_on_selected(
        const std::function<const char*(bool)>& action_name_cb,
        const std::function<bool(data::NodeInstance*)>& first_cb,
        const std::function<void(data::NodeInstance*, data::WorkflowAction*, bool)>& action_cb);

    // text editor panel internal
    ImGuiID find_def_txt_panel_dock() noexcept;
    void activate_txt_panels(wid wf);
    void destroy_txt_panel(PanelTextEditor* panel) noexcept;

    // timer
    void on_1s_lazy_timer(int64_t total_seconds_elapsed) override;

    // stats
    void post_stats_event(HttpRequestor requestor, int64_t session_id, const char* ev_type, uint64_t ev_time, const char* prop_type, nlohmann::json&& event_props, bool callback_needed, mngr::StatsData::Progress* progress = nullptr);
    void stats_post_session_start();
    void stats_save_session_end();
    void stats_post_session_end();
    void stats_post_monitor_data();
    void stats_post_user_data();
    void stats_post_host(HostType host_type);

private:
    std::thread::id m_ui_thread_id;

    std::atomic_bool m_nodes_ready = false;
    std::atomic_bool m_models_ready = false;
    std::atomic_bool m_projects_ready = false;

    std::unique_ptr<mngr::NodesParser> m_nodes_parser;
    std::unique_ptr<CustomMediaWidget> m_custom_media_widget;
    std::unique_ptr<CustomJsNode> m_custom_js_node;

    ImGuiEx::Toast m_toast;

    ImGuiID m_main_dock_id = 0;
    uint64_t m_windows = ViewNone;
    uint64_t m_windows_toggled = ViewNone;
    uint64_t m_panel_appeared = ViewNone;

    std::set<BaseDlg*> m_all_dialogs;

    std::unique_ptr<ModalMessage> m_modal_message;
    std::function<void(ModalDlgCtrl)> m_msg_callback = nullptr;

    std::unique_ptr<OptionsDlgs> m_opts_dlg;
    std::unique_ptr<SyncProjectsDlg> m_sync_projs_dlg;
    std::unique_ptr<ActivityBar> m_activity_bar;
    ActivityBarMode m_activity_bar_mode = ActivityBarMode::Smart;

    std::unique_ptr<PanelNodes> m_panel_nodes;
    std::unique_ptr<PanelNodeInspector> m_panel_node_inspector;
    std::unique_ptr<PanelStats> m_panel_stats;
    //std::unique_ptr<PanelTemplates> m_panel_templates;
    std::unique_ptr<PanelServerLogs> m_panel_logs;
    //std::unique_ptr<PanelNotes> m_panel_notes;
    std::unique_ptr<PanelModels> m_panel_models;
    std::unique_ptr<PanelWfList> m_panel_wf_list;
    std::unique_ptr<PanelQueue> m_panel_queue;
    std::list<std::unique_ptr<PanelTextEditor>> m_panels_text_editor;

    PanelWorkflow* m_wf_focused = nullptr;
    std::list<std::unique_ptr<PanelWorkflow>> m_panels_wf;
    bool m_wf_imported = false;

#ifdef _DEBUG
    std::list<std::unique_ptr<DebugWfView>> m_wf_debug_panels;
#endif

    std::mutex m_http_req_mutex;
    struct HttpRequestInfo final
    {
        StillAliveCallback alive = nullptr;
        HttpResponseCb callback = nullptr;
        double time = 0.0;
        bool retry = false;
        bool force_drop = false;
    };
    std::unordered_map<uint32_t, HttpRequestInfo> m_http_requests;

    struct PendingCleanup final
    {
        bool opts_dlg = false;
        bool sync_projs_dlg = false;
        bool modal_dlg = false;
        std::vector<PanelTextEditor*> text_editors;
        std::vector<PanelWorkflow*> wf_panels;
    }
    m_pending_cleanups;

    // stats
    int64_t m_next_stat_attempt = 10;
    mngr::StatsData m_stats;

    // update
    int64_t m_last_update_check = 0;
    app_version_type m_version_found = 0;
};
}
