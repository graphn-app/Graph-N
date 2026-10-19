#include "core/managers/settings_manager.h"
#include "core/managers/db_manager.h"
#include "core/managers/hotkeys_manager.h"
#include "ui/graph-n_flow.h"

namespace mngr
{

SettingsManager::SettingsManager() noexcept
    : m_items_wf_labels_id("{389DA1FA-80C5-45EF-93B8-6C87D3BD03B2}")
    , m_items_node_labels_id("{5E9A2E7E-D18E-448E-8D8D-D704484D42B8}")
    , m_items_model_favs_id("{85A87BC9-691D-44FA-AA16-1569E36ABE18}")
    , m_items_history_favs_id("{1424AD9F-BE57-437C-BB18-9B77D5B148BF}")
{
}

SettingsManager::~SettingsManager() noexcept = default;

bool SettingsManager::init()
{
    if (!Manager::init())
        return false;

    // app
    m_app_vsync =                                      { "vsync_enabled",           bool(true) };
    m_app_fps_limit =                                  { "fps_limit",               bool(false) };
    m_app_fps_limit_val =                              { "fps_limit_value",         int64_t(30) };
    m_app_scale =                                      { "ui_scale",                int64_t(100) };
    m_app_db_backups =                                 { "ui_backups",              int64_t(2) };
    m_app_metrics_on =                                 { "app_metrics",             bool(true) };
    m_app_upd_check_on =                               { "app_update_check",        bool(true) };

    // ui
    m_ui_open_wnds =                                  { "open_windows",             int64_t(ui::ViewPanelsResetAll) };
    m_ui_nodes_view =                                 { "nodes_view",               int64_t(NodeViewType::categories) };
    m_ui_theme =                                      { "theme",                    int64_t(0) };
    m_ui_activity_bar_mode =                          { "activity_bar_mode",        int64_t(cast::to_i64(ActivityBarMode::Smart)) };
    m_ui_activity_bar_big_icons =                     { "activity_bar_big_icons",   bool(false) };
    m_ui_activity_bar_show_restart =                  { "activity_bar_add_restart", bool(false) };    
    m_ui_hotkeys =                                    { "key_bindings",             format_hotkeys_to_json(g::pHotkeysManager->get_default_key_bindings()) };
    m_ui_host_settings =                /* safe */    { "host_settings",            std::string("{}"),                  true };

    // comfyui
    m_comfyui_host =                    /* safe */    { "comfyui_host",             std::string("127.0.0.1"),           true };
    m_comfyui_port =                    /* safe */    { "comfyui_port",             int64_t(8188),                      true };
    m_comfyui_conn_time =               /* safe */    { "comfyui_conn_time",        int64_t(1),                         true };
    m_comfyui_start_server =                          { "comfyui_start_server",     bool(false) };
    m_comfyui_python =                                { "comfyui_python",           std::filesystem::path("") };
    m_comfyui_python_args =                           { "comfyui_python_args",      std::string("") };
    m_comfyui_kill_server =                           { "comfyui_kill_server",      bool(false) };
    m_comfyui_parse_address =                         { "comfyui_parse_address",    bool(false) };
    m_comfyui_hide_console =                          { "comfyui_hide_console",     bool(false) };
    m_comfyui_capture_output =                        { "comfyui_capture_output",   bool(false) };
    m_comfyui_auto_hide_output =                      { "comfy_auto_hide_output",   bool(true) };
    m_comfyui_main_py =                               { "comfyui_main_py",          std::filesystem::path("") };
    m_comfyui_comfyui_args =                          { "comfyui_comfyui_args",     std::string("") };
    m_comfyui_workdir =                               { "comfyui_workdir",          std::filesystem::path("") };
    m_comfyui_output =                                { "comfyui_output",           std::filesystem::path("") };
    m_comfyui_ping_interval =                         { "comfyui_ping_interval",    int64_t(2) };

    // wf
    m_wf_widget_input_numbers =                       { "widget_input_numbers",     cast::to_i64(WidgetInput::Direct) };
    m_wf_widget_input_bools =                         { "widget_input_bools",       cast::to_i64(WidgetInput::Select) };
    m_wf_active =                                     { "active_wf",                std::string("") };
    m_wf_show_grid =                                  { "show_grid",                bool(true) };
    m_wf_snap_nodes =                                 { "snap_nodes",               bool(false) };
    m_wf_link_thickness =                             { "link_thickness",           int64_t(2) };
    m_wf_imported =                                   { "imported_wfs",             std::string("") };

    // queue
    m_queue_favs =                                    { "queue_favorited",          std::string("") };
    m_queue_history_num =                             { "queue_history_num",        int64_t(50) };

    // logs
    m_log_db_backup_num =                             { "db_backup_save_num",       int64_t(3) };
    m_log_app_log_num =                               { "app_log_save_num",         int64_t(3) };
    m_log_comfyui_log_num =                           { "comfyui_log_save_num",     int64_t(3) };
    m_log_comfyui_log_in_ram =                        { "comfyui_log_in_ram",       int64_t(1000) };

    // colors
    m_ui_colors.resize(cast::to_szt(AppColor::_count));

    m_ui_colors[cast::to_szt(AppColor::Pin_Int)] =                           { "color_pin_int", int64_t(IM_COL32(40, 200, 40, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_Float)] =                         { "color_pin_float", int64_t(IM_COL32(40, 200, 40, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_String)] =                        { "color_pin_string", int64_t(IM_COL32(200, 200, 200, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_Boolean)] =                       { "color_pin_boolean", int64_t(IM_COL32(180, 180, 180, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_Any)] =                           { "color_pin_any", int64_t(IM_COL32_WHITE) };
    m_ui_colors[cast::to_szt(AppColor::Pin_Combo)] =                         { "color_pin_combo", int64_t(IM_COL32(200, 200, 200, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_Color)] =                         { "color_pin_color", int64_t(IM_COL32(255, 235, 59, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_RerouteSplitted)] =               { "color_pin_reroute_splitted", int64_t(IM_COL32(255, 255, 255, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_Number)] =                        { "color_pin_number", int64_t(IM_COL32(46, 204, 113, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_Floats)] =                        { "color_pin_floats", int64_t(IM_COL32(26, 188, 156, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_Seed)] =                          { "color_pin_seed", int64_t(IM_COL32(52, 152, 219, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_Model)] =                         { "color_pin_model", int64_t(IM_COL32(111, 108, 155, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_Latent)] =                        { "color_pin_latent", int64_t(IM_COL32(176, 83, 112, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_Vae)] =                           { "color_pin_vae", int64_t(IM_COL32(166, 46, 46, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_Clip)] =                          { "color_pin_clip", int64_t(IM_COL32(227, 184, 54, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_Image)] =                         { "color_pin_image", int64_t(IM_COL32(91, 135, 163, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_Mask)] =                          { "color_pin_mask", int64_t(IM_COL32(93, 158, 89, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_Conditioning)] =                  { "color_pin_conditioning", int64_t(IM_COL32(195, 134, 52, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_ClipVision)] =                    { "color_pin_clipvision", int64_t(IM_COL32(100, 100, 100, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_ControlNet)] =                    { "color_pin_control_net", int64_t(IM_COL32(90, 160, 140, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_Audio)] =                         { "color_pin_audio", int64_t(IM_COL32(255, 160, 0, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_Video)] =                         { "color_pin_video", int64_t(IM_COL32(41, 128, 185, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_IPAdapter)] =                     { "color_pin_ipadapter", int64_t(IM_COL32(215, 189, 226, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_InsightFace)] =                   { "color_pin_insightface", int64_t(IM_COL32(231, 76, 60, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_ControlNetStack)] =               { "color_pin_control_net_stack", int64_t(IM_COL32(39, 174, 96, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_ClipVisionOutput)] =              { "color_pin_clipvision_output", int64_t(IM_COL32(241, 196, 15, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_Noise)] =                         { "color_pin_noise", int64_t(IM_COL32(149, 165, 166, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_Sigmas)] =                        { "color_pin_sigmas", int64_t(IM_COL32(140, 100, 80, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_Sampler)] =                       { "color_pin_sampler", int64_t(IM_COL32(230, 126, 34, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_PipeLine)] =                      { "color_pin_pipe_line", int64_t(IM_COL32(255, 215, 0, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_Lora)] =                          { "color_pin_lora_list", int64_t(IM_COL32(219, 112, 147, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_Segs)] =                          { "color_pin_segs", int64_t(IM_COL32(0, 255, 255, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_BBox)] =                          { "color_pin_bbox", int64_t(IM_COL32(127, 255, 0, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Pin_Bus)] =                           { "color_pin_bus", int64_t(IM_COL32(255, 215, 0, 255)) };

    m_ui_colors[cast::to_szt(AppColor::DisconnectedMenuBarBg)] =             { "color_disconnected_menubar_bg",  int64_t(IM_COL32(150, 0, 0, 255)) };
    m_ui_colors[cast::to_szt(AppColor::DisconnectedMenuBarText)] =           { "color_disconnected_menubar_text",  int64_t(IM_COL32(255, 255, 255, 255)) };
    m_ui_colors[cast::to_szt(AppColor::OptBtnNormal)] =                      { "color_opt_btn_normal",  int64_t(IM_COL32(0, 128, 128, 200)) };
    m_ui_colors[cast::to_szt(AppColor::OptBtnHover)] =                       { "color_opt_btn_hover",  int64_t(IM_COL32(0, 160, 160, 200)) };
    m_ui_colors[cast::to_szt(AppColor::OptBtnPressed)] =                     { "color_opt_btn_pressed",  int64_t(IM_COL32(0, 95, 95, 255)) };
    m_ui_colors[cast::to_szt(AppColor::MiscDlgTitleBg)] =                    { "color_misc_dlg_title_bg",  int64_t(IM_COL32(0, 128, 128, 255)) };
    m_ui_colors[cast::to_szt(AppColor::MiscDlgBg)] =                         { "color_misc_dlg_bg",  int64_t(IM_COL32(0, 0, 0, 255)) };
    m_ui_colors[cast::to_szt(AppColor::PanelBg)] =                           { "color_panel_bg",  int64_t(IM_COL32(24, 24, 27, 255)) };
    m_ui_colors[cast::to_szt(AppColor::PanelActiveWfTab)] =                  { "color_active_wf_tab",  int64_t(IM_COL32(223, 144, 18, 120)) };
    m_ui_colors[cast::to_szt(AppColor::PanelActiveHoveredWfTab)] =           { "color_active_hovered_wf_tab",  int64_t(IM_COL32(194, 126, 0, 140)) };
    m_ui_colors[cast::to_szt(AppColor::PanelNodesHoverGroup)] =              { "color_panel_nodes_hover_group",  int64_t(IM_COL32(220, 220, 220, 50)) };
    m_ui_colors[cast::to_szt(AppColor::PanelNodesHoverNode)] =               { "color_panel_nodes_hover_node",  int64_t(IM_COL32(211, 112, 0, 80)) };
    m_ui_colors[cast::to_szt(AppColor::PanelNodesSelNode)] =                 { "color_panel_nodes_sel_node",  int64_t(IM_COL32(255, 165, 0, 250)) };
    m_ui_colors[cast::to_szt(AppColor::PanelNodesInspDescrFrame)] =          { "color_panel_node_insp_descr_frame", int64_t(IM_COL32(220, 220, 220, 50)) };
    m_ui_colors[cast::to_szt(AppColor::NodeHeaderBg)] =                      { "color_node_header_bg", int64_t(IM_COL32(45, 50, 60, 255)) };
    m_ui_colors[cast::to_szt(AppColor::NodeHeaderText)] =                    { "color_node_header_text", int64_t(IM_COL32(255, 176, 50, 255)) };
    m_ui_colors[cast::to_szt(AppColor::NodeBodyBg)] =                        { "color_node_body_bg", int64_t(IM_COL32(32, 32, 32, 200)) };
    m_ui_colors[cast::to_szt(AppColor::NodePinText)] =                       { "color_node_pin_text", int64_t(IM_COL32(100, 100, 100, 255)) };
    m_ui_colors[cast::to_szt(AppColor::NodeBorder)] =                        { "color_node_border", int64_t(IM_COL32(100, 100, 100, 255)) };
    m_ui_colors[cast::to_szt(AppColor::NodeWidgetHiddenSep)] =               { "color_node_widget_hidden_sep", int64_t(IM_COL32(0, 180, 180, 200)) };
    m_ui_colors[cast::to_szt(AppColor::NodeDescription)] =                   { "color_node_description", int64_t(IM_COL32(0, 180, 180, 255)) };
    m_ui_colors[cast::to_szt(AppColor::NodeHeaderIcon)] =                    { "color_node_header_icon", int64_t(IM_COL32(0, 180, 180, 255)) };
    m_ui_colors[cast::to_szt(AppColor::NodeBorderMuted)] =                   { "color_node_border_muted", int64_t(IM_COL32(92, 92, 95, 255)) };
    m_ui_colors[cast::to_szt(AppColor::NodeBgMuted)] =                       { "color_node_bg_muted", int64_t(IM_COL32(0, 0, 0, 100)) };
    m_ui_colors[cast::to_szt(AppColor::NodeBorderBypassed)] =                { "color_node_border_bypassed", int64_t(IM_COL32(92, 92, 95, 255)) };
    m_ui_colors[cast::to_szt(AppColor::NodeBgBypassed)] =                    { "color_node_bg_bypassed", int64_t(IM_COL32(46, 13, 46, 255)) };
    m_ui_colors[cast::to_szt(AppColor::GroupBg)] =                           { "color_group_bg", int64_t(IM_COL32(38, 71, 93, 0)) };
    m_ui_colors[cast::to_szt(AppColor::StatNorm)] =                          { "color_stat_norm", int64_t(IM_COL32(255, 255, 255, 150)) };
    m_ui_colors[cast::to_szt(AppColor::StatWarn)] =                          { "color_stat_warn", int64_t(IM_COL32(255, 200, 50, 255)) };
    m_ui_colors[cast::to_szt(AppColor::StatCrit)] =                          { "color_stat_crit", int64_t(IM_COL32(255, 50, 50, 255)) };
    m_ui_colors[cast::to_szt(AppColor::StatSepCapt)] =                       { "color_stat_sep_capt", int64_t(IM_COL32(0, 180, 180, 200)) };
    m_ui_colors[cast::to_szt(AppColor::StatSepSubtitle)] =                   { "color_stat_sep_subtitle", int64_t(IM_COL32(255, 255, 255, 150)) };
    m_ui_colors[cast::to_szt(AppColor::PopupBg)] =                           { "color_popup_bg", int64_t(IM_COL32(50, 50, 50, 255)) };
    m_ui_colors[cast::to_szt(AppColor::MenubarBg)] =                         { "color_menubar_bg", int64_t(IM_COL32(50, 50, 50, 255)) };
    m_ui_colors[cast::to_szt(AppColor::OptSeparator)] =                      { "color_opt_separator", int64_t(IM_COL32(0, 180, 180, 200)) };
    m_ui_colors[cast::to_szt(AppColor::SelectedItem)] =                      { "color_selected_item", int64_t(IM_COL32(218, 122, 0, 255)) };
    m_ui_colors[cast::to_szt(AppColor::OptHelpMark)] =                       { "color_opt_help_mark", int64_t(IM_COL32(50, 50, 50, 255)) };
    m_ui_colors[cast::to_szt(AppColor::OptHelpText)] =                       { "color_opt_help_text", int64_t(IM_COL32(0, 128, 128, 200)) };
    m_ui_colors[cast::to_szt(AppColor::RestartButton)] =                     { "color_restart_button", int64_t(IM_COL32(120, 0, 0, 255)) };
    m_ui_colors[cast::to_szt(AppColor::UrlTextDefault)] =                    { "color_url_text_default", int64_t(IM_COL32(51, 127, 229, 255)) };
    m_ui_colors[cast::to_szt(AppColor::UrlTextHovered)] =                    { "color_url_text_hovered", int64_t(IM_COL32(102, 153, 255, 255)) };
    m_ui_colors[cast::to_szt(AppColor::FolderIcon)] =                        { "color_folder_icon", int64_t(IM_COL32(255, 210, 88, 255)) };
    m_ui_colors[cast::to_szt(AppColor::Starred)] =                           { "color_starred", int64_t(IM_COL32(255, 215, 0, 255)) };
    m_ui_colors[cast::to_szt(AppColor::TextEditorCaption1)] =                { "color_txt_editor_caption1", int64_t(IM_COL32(0, 180, 180, 255)) };
    m_ui_colors[cast::to_szt(AppColor::TextEditorCaption2)] =                { "color_txt_editor_caption2", int64_t(IM_COL32(100, 100, 100, 255)) };
    m_ui_colors[cast::to_szt(AppColor::MsgBoxError)] =                       { "color_msg_error", int64_t(IM_COL32(170, 15, 15, 255)) };
    m_ui_colors[cast::to_szt(AppColor::MsgBoxWarning)] =                     { "color_msg_warning", int64_t(IM_COL32(190, 150, 50, 255)) };
    m_ui_colors[cast::to_szt(AppColor::MsgBoxInfo)] =                        { "color_msg_info", int64_t(IM_COL32(0, 150, 200, 255)) };
    m_ui_colors[cast::to_szt(AppColor::MsgBoxQuestion)] =                    { "color_msg_question", int64_t(IM_COL32(50, 150, 50, 255)) };
    m_ui_colors[cast::to_szt(AppColor::ActivityBarBg)] =                     { "color_activity_bar_bg",  int64_t(IM_COL32(24, 24, 27, 255)) };
    m_ui_colors[cast::to_szt(AppColor::ActivityBarActive)] =                 { "color_activity_bar_active",  int64_t(IM_COL32(0, 150, 150, 255)) };
    m_ui_colors[cast::to_szt(AppColor::ActivityBarInactive)] =               { "color_activity_bar_inactive",  int64_t(IM_COL32(92, 92, 95, 255)) };
    m_ui_colors[cast::to_szt(AppColor::TabActive)] =                         { "color_tab_active",  int64_t(IM_COL32(0, 100, 100, 255)) };
    m_ui_colors[cast::to_szt(AppColor::TabInactive)] =                       { "color_tab_inactive",  int64_t(IM_COL32(17, 26, 38, 255)) };
    m_ui_colors[cast::to_szt(AppColor::TabHovered)] =                        { "color_tab_hovered",  int64_t(IM_COL32(0, 150, 150, 255)) };
    m_ui_colors[cast::to_szt(AppColor::DummyNode)] =                         { "color_dummy_node",  int64_t(IM_COL32(255, 0, 0, 255)) };
    m_ui_colors[cast::to_szt(AppColor::NodeNote)] =                          { "color_note_node",  int64_t(IM_COL32(30, 47, 64, 212)) };

    // stats
    m_stat_limits_warn.resize(cast::to_szt(MetricLimit::_count));
    m_stat_limits_crit.resize(cast::to_szt(MetricLimit::_count));
    m_stat_limits_warn[cast::to_szt(MetricLimit::LocalRAM)] =                 { "stat_local_ram_warn",      int64_t(2147483648) };     // 2gb
    m_stat_limits_crit[cast::to_szt(MetricLimit::LocalRAM)] =                 { "stat_local_ram_crit",      int64_t(524288000) };      // 500 mb
    m_stat_limits_warn[cast::to_szt(MetricLimit::LocalDisk)] =                { "stat_disk_warn",           int64_t(10737418240) };    // 10 gb
    m_stat_limits_crit[cast::to_szt(MetricLimit::LocalDisk)] =                { "stat_disk_crit",           int64_t(2147483648) };     // 2 gb
    //m_stat_limits_warn[cast::to_szt(MetricLimit::LocalFPS)] =                 { "stat_fps_warn",            int64_t(200) };
    //m_stat_limits_crit[cast::to_szt(MetricLimit::LocalFPS)] =                 { "stat_fps_crit",            int64_t(1000) };
    m_stat_limits_warn[cast::to_szt(MetricLimit::ServerRAM)] =                { "stat_server_ram_warn",     int64_t(3221225472) };     // 3 gb
    m_stat_limits_crit[cast::to_szt(MetricLimit::ServerRAM)] =                { "stat_server_ram_crit",     int64_t(1073741824) };     // 1 gb
    m_stat_limits_warn[cast::to_szt(MetricLimit::ServerVRAM)] =               { "stat_vram_warn",           int64_t(1073741824) };     // 1 gb
    m_stat_limits_crit[cast::to_szt(MetricLimit::ServerVRAM)] =               { "stat_vram_crit",           int64_t(209715200) };      // 200 mb
    m_stat_limits_warn[cast::to_szt(MetricLimit::ServerPyTorch)] =            { "stat_pytorch_warn",        int64_t(524288000) };      // 500 mb
    m_stat_limits_crit[cast::to_szt(MetricLimit::ServerPyTorch)] =            { "stat_pytorch_crit",        int64_t(104857600) };      // 100 mb
    m_stat_limits_warn[cast::to_szt(MetricLimit::ServerPing)] =               { "stat_ping_warn",           int64_t(30) };
    m_stat_limits_crit[cast::to_szt(MetricLimit::ServerPing)] =               { "stat_ping_crit",           int64_t(100) };
    m_stat_as_progress =                                                      { "stat_as_progress",         bool(true) };
    m_stat_enabled_disks =                                                    { "stat_enabled_disks",       std::string("") };

    // initialization
    std::vector<OptionValue*> values;
    values.reserve(200);
    values.push_back(&m_app_vsync);
    values.push_back(&m_app_fps_limit);
    values.push_back(&m_app_fps_limit_val);
    values.push_back(&m_app_scale);
    values.push_back(&m_app_db_backups);
    values.push_back(&m_app_metrics_on);
    values.push_back(&m_app_upd_check_on);
    values.push_back(&m_ui_open_wnds);
    values.push_back(&m_ui_nodes_view);
    values.push_back(&m_ui_theme);
    values.push_back(&m_ui_activity_bar_mode);
    values.push_back(&m_ui_activity_bar_big_icons);
    values.push_back(&m_ui_activity_bar_show_restart);
    values.push_back(&m_ui_hotkeys);
    values.push_back(&m_ui_host_settings);
    values.push_back(&m_comfyui_host);
    values.push_back(&m_comfyui_port);
    values.push_back(&m_comfyui_conn_time);
    values.push_back(&m_comfyui_start_server);
    values.push_back(&m_comfyui_python);
    values.push_back(&m_comfyui_python_args);
    values.push_back(&m_comfyui_kill_server);
    values.push_back(&m_comfyui_parse_address);
    values.push_back(&m_comfyui_hide_console);
    values.push_back(&m_comfyui_capture_output);
    values.push_back(&m_comfyui_auto_hide_output);
    values.push_back(&m_comfyui_main_py);
    values.push_back(&m_comfyui_comfyui_args);
    values.push_back(&m_comfyui_workdir);
    values.push_back(&m_comfyui_output);
    values.push_back(&m_comfyui_ping_interval);
    values.push_back(&m_log_db_backup_num);
    values.push_back(&m_log_app_log_num);
    values.push_back(&m_log_comfyui_log_num);
    values.push_back(&m_log_comfyui_log_in_ram);
    values.push_back(&m_wf_widget_input_numbers);
    values.push_back(&m_wf_widget_input_bools);
    values.push_back(&m_wf_active);
    values.push_back(&m_wf_show_grid);
    values.push_back(&m_wf_link_thickness);
    values.push_back(&m_wf_snap_nodes);
    values.push_back(&m_wf_imported);
    values.push_back(&m_queue_favs);
    values.push_back(&m_queue_history_num);
    values.push_back(&m_stat_as_progress);
    values.push_back(&m_stat_enabled_disks);

    for (auto& opt : m_stat_limits_warn)
    {
        values.push_back(&opt);
    }
    for (auto& opt : m_stat_limits_crit)
    {
        values.push_back(&opt);
    }
    for (auto& opt : m_ui_colors)
    {
        values.push_back(&opt);
    }

    init_options(values);
    init_items_defaults();
    init_disks();

    m_ready.store(true, std::memory_order_relaxed);
    return true;
}

void SettingsManager::init_options(std::vector<OptionValue*>& values)
{
    auto db_values = g::pDbManager->app_option_load_all();
    bool do_update = false;

    for (auto& val : values)
    {
        auto it = std::ranges::find_if(db_values, [&val](DbAppOptionData& db_opt) { return db_opt.name == val->db_name; });
        if (it != db_values.end())
        {
            if (std::holds_alternative<int64_t>(val->value))
                val->value = it->val.ival;
            else if (std::holds_alternative<std::string>(val->value))
                val->value = it->val.sval;
            else if (std::holds_alternative<std::filesystem::path>(val->value))
                val->value = std::filesystem::path(std::u8string(it->val.sval.begin(), it->val.sval.end()));
            else if (std::holds_alternative<bool>(val->value))
                val->value = bool(it->val.ival == 1);
            else
                LogError("Unhandled option type");
        }
        else
        {
            do_update = true;
            DbAppOptionData db_opt;
            db_opt.name = val->db_name;

            std::visit([&db_opt](const auto& v)
            {
                using T = std::decay_t<decltype(v)>;

                if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, std::string> || std::is_same_v<T, bool>)
                {
                    db_opt.val = v;
                }
                else if constexpr (std::is_same_v<T, std::filesystem::path>)
                {
                    db_opt.val = tools::path::path_to_string(v);
                }
                else
                {
                    static_assert(sizeof(T) == 0, "Unhandled option type");
                }
            }, val->value);

            db_values.push_back(std::move(db_opt));
        }
    }

    if (do_update)
    {
        g::pDbManager->app_option_save_all(std::move(db_values));
    }
}

void SettingsManager::init_disks()
{
    m_stat_enabled_disks_parsed.clear();
    const auto loaded = tools::str::split(*std::get_if<std::string>(&m_stat_enabled_disks.value), "|", true);
    const auto installed = tools::api::scan_for_disks(); // always scan for new disks
    for (auto& installed_name : installed)
    {
        auto pair = std::make_pair(installed_name, true);
        auto it = std::ranges::find_if(loaded, [&installed_name](const std::string& loaded_name) { return loaded_name == installed_name; });
        if (it != loaded.end())
        {
            ++it;
            if (it != loaded.end())
                pair.second = (*it != "-");
        }
        m_stat_enabled_disks_parsed.push_back(pair);
    }

    stat_disks_enabled_save();
}

template <typename OPT_T>
void save_option_impl(std::variant<std::string, int64_t, std::filesystem::path, bool>& option_value, const char* option_db_name,
    const OPT_T& new_value, DbValue&& db_value, const char* type_str)
{
    LogRAssertS(std::holds_alternative<OPT_T>(option_value), "Value " << option_db_name << " is not a " << type_str, );

    auto* ptr = std::get_if<OPT_T>(&option_value);
    LogRAssertS(ptr, "Value " << option_db_name << " zero ptr", );
    if (*ptr != new_value)
    {
        *ptr = new_value;
        g::pDbManager->app_option_save(option_db_name, std::move(db_value));
    }
}

void SettingsManager::save_option(OptionValue& option, const std::string& new_str)
{
    if (option.guard_required) [[unlikely]]
    {
        std::unique_lock<std::mutex> lock(m_guard);
        save_option_impl(option.value, option.db_name, new_str, { new_str }, "string");
    }
    else
    {
        save_option_impl(option.value, option.db_name, new_str, { new_str }, "string");
    }
}

void SettingsManager::save_option(OptionValue& option, int64_t new_int)
{
    if (option.guard_required) [[unlikely]]
    {
        std::unique_lock<std::mutex> lock(m_guard);
        save_option_impl(option.value, option.db_name, new_int, { new_int }, "int");
    }
    else
    {
        save_option_impl(option.value, option.db_name, new_int, { new_int }, "int");
    }
}

void SettingsManager::save_option(OptionValue& option, bool new_bool)
{
    if (option.guard_required) [[unlikely]]
    {
        std::unique_lock<std::mutex> lock(m_guard);
        save_option_impl(option.value, option.db_name, new_bool, { new_bool ? 1 : 0 }, "bool");
    }
    else
    {
        save_option_impl(option.value, option.db_name, new_bool, { new_bool ? 1 : 0 }, "bool");
    }
}

void SettingsManager::save_option(OptionValue& option, const std::filesystem::path& new_path)
{
    if (option.guard_required) [[unlikely]]
    {
        std::unique_lock<std::mutex> lock(m_guard);
        save_option_impl(option.value, option.db_name, new_path, { tools::path::path_to_string(new_path) }, "path");
    }
    else
    {
        save_option_impl(option.value, option.db_name, new_path, { tools::path::path_to_string(new_path) }, "path");
    }
}

//////////////////////////////////////////////////////////////////////
////////       SAFE LOAD                 /////////////////////////////
//////////////////////////////////////////////////////////////////////

std::string SettingsManager::comfyui_host_load_safe()
{
    std::string ret;
    {
        std::unique_lock<std::mutex> lock(m_guard);
        ret = comfyui_host_load();
    }
    return ret;
}

int64_t SettingsManager::comfyui_port_load_safe()
{
    int64_t ret = 0;
    {
        std::unique_lock<std::mutex> lock(m_guard);
        ret = comfyui_port_load();
    }
    return ret;
}

int64_t SettingsManager::comfyui_connect_timeout_load_safe()
{
    int64_t ret = 0;
    {
        std::unique_lock<std::mutex> lock(m_guard);
        ret = comfyui_connect_timeout_load();
    }
    return ret;
}

//////////////////////////////////////////////////////////////////////
////////       UI                        /////////////////////////////
//////////////////////////////////////////////////////////////////////

void SettingsManager::ui_open_wnds_save(int64_t windows)
{
    windows &= ~ui::ViewAlwaysClose;
    save_option(m_ui_open_wnds, windows);
}

std::string SettingsManager::format_hotkeys_to_json(const std::vector<ui::KeyBinding>& hotkeys) const
{
    auto j = u_json::array();
    for (const auto& hk : hotkeys)
    {
        u_json jhk;
        jhk["target"] = cast::to_i32(hk.target);
        jhk["ctrl"] = hk.ctrl;
        jhk["cmd"] = hk.cmd;
        jhk["alt"] = hk.alt;
        jhk["shift"] = hk.shift;
        jhk["key"] = cast::to_i32(hk.key);
        jhk["enabled"] = hk.enabled;
        jhk["node_related"] = hk.node_related;
        jhk["repeatable"] = hk.repeatable;
        j.push_back(std::move(jhk));
    }

    return j.dump();
}

std::vector<ui::KeyBinding> SettingsManager::ui_hotkeys_load() noexcept
{
    const std::string* val = std::get_if<std::string>(&m_ui_hotkeys.value);
    if (nullptr == val || val->empty())
        return {};

    try
    {
        std::vector<ui::KeyBinding> hotkeys;
        const auto j = u_json::parse(*val);

        const auto& key_names = g::pHotkeysManager->get_key_names();
        const auto is_supported = [&key_names](int32_t test)
        {
            for (auto key : key_names)
            {
                if (key.first == test)
                    return true;
            }
            return false;
        };

        if (j.is_array())
        {
            for (auto& jval : j)
            {
                ui::KeyBinding kb;
                if (jval.contains("target") && jval["target"].is_number_integer())
                {
                    const auto target = jval["target"].get<int32_t>();
                    if (target >= cast::to_i32(KeyTarget::_first) && target <= cast::to_i32(KeyTarget::_last))
                        kb.target = cast::to_enum<KeyTarget>(target);
                }
                if (jval.contains("key") && jval["key"].is_number_integer())
                {
                    const auto key = jval["key"].get<int32_t>();
                    if (key >= ImGuiKey_NamedKey_BEGIN && key <= ImGuiKey_NamedKey_END && is_supported(key))
                        kb.key = cast::to_imguikey(key);
                }
                if (jval.contains("ctrl") && jval["ctrl"].is_boolean())
                    kb.ctrl = jval["ctrl"];
                if (jval.contains("cmd") && jval["cmd"].is_boolean())
                    kb.cmd = jval["cmd"];
                if (jval.contains("alt") && jval["alt"].is_boolean())
                    kb.alt = jval["alt"];
                if (jval.contains("shift") && jval["shift"].is_boolean())
                    kb.shift = jval["shift"];
                if (jval.contains("node_related") && jval["node_related"].is_boolean())
                    kb.node_related = jval["node_related"];
                if (jval.contains("repeatable") && jval["repeatable"].is_boolean())
                    kb.repeatable = jval["repeatable"];
                if (jval.contains("enabled") && jval["enabled"].is_boolean())
                    kb.enabled = jval["enabled"];
                else
                    kb.enabled = false;

                if (kb.key != ImGuiKey_None && kb.target != KeyTarget::_none)
                    hotkeys.push_back(kb);
            }
        }

        return hotkeys;
    }
    catch (const std::exception& ex)
    {
        LogExcep("Failed to parse key bindings: " << ex.what());        
    }
    return {};
}

void SettingsManager::ui_host_settings_save(const std::string& settings)
{
    try
    {
        const auto& existing = *std::get_if<std::string>(&m_ui_host_settings.value);
        if (existing.empty())
        {
            save_option(m_ui_host_settings, settings);
            return;
        }
        auto jold = u_json::parse(existing);
        if (jold.empty())
        {
            save_option(m_ui_host_settings, settings);
            return;
        }

        const auto jnew = u_json::parse(settings);
        if (jold.is_object() && jnew.is_object())
        {
            jold.update(jnew);
            save_option(m_ui_host_settings, jold.dump());
        }
        else
            save_option(m_ui_host_settings, settings);
    }
    catch (const std::exception& ex)
    {
        LogExcep("Failed to parse host settings: " << ex.what());
        save_option(m_ui_host_settings, settings);
    }
}

//////////////////////////////////////////////////////////////////////
////////       WF                        /////////////////////////////
//////////////////////////////////////////////////////////////////////

std::list<std::pair<std::string, bool>> SettingsManager::wf_active_load() const noexcept
{
    const std::string* val = std::get_if<std::string>(&m_wf_active.value);
    if (nullptr == val || val->empty())
        return {};

    try
    {
        std::list<std::pair<std::string, bool>> ret;
        const auto j = u_json::parse(*val);
        if (j.is_array())
        {
            for (auto& jval : j)
            {                
                if (jval.contains("id") && jval["id"].is_string() &&
                    jval.contains("active") && jval["active"].is_boolean())
                {
                    ret.push_back(std::make_pair(jval["id"].get<std::string>(), jval["active"].get<bool>()));
                }
            }
        }
        return ret;
    }
    catch (const std::exception& ex)
    {
        LogExcep("Failed to parse active workflows: " << ex.what());        
    }
    return {};
}

void SettingsManager::wf_active_save(const std::list<std::pair<std::string, bool>>& active_wfs)
{
    auto j = u_json::array();
    for (const auto& wf : active_wfs)
    {
        u_json jhk;
        jhk["id"] = wf.first;
        jhk["active"] = wf.second;
        j.push_back(std::move(jhk));
    }

    save_option(m_wf_active, j.dump());
}

bool SettingsManager::wf_imported_load(const std::string& project) const noexcept
{
    const auto& imported = *std::get_if<std::string>(&m_wf_imported.value);
    if (imported.empty())
        return false;

    try
    {
        const auto j = u_json::parse(imported);
        if (j.is_object())
        {
            const std::string addr = comfyui_host_load() + ":" + std::to_string(comfyui_port_load());

            for (auto& [jaddr, jprojects] : j.items())
            {
                if (jaddr == addr && jprojects.is_array())
                {
                    for (auto& jproject : jprojects)
                    {
                        if (jproject.is_string() && jproject == project)
                            return true;
                    }
                }
            }
        }
    }
    catch (const std::exception& ex)
    {
        LogExcep("Failed to parse imported projects: " << ex.what());
    }

    return false;
}

void SettingsManager::wf_imported_save(const std::string& project)
{
    const std::string addr = comfyui_host_load() + ":" + std::to_string(comfyui_port_load());
    const auto& imported = *std::get_if<std::string>(&m_wf_imported.value);
    if (imported.empty())
    {
        u_json j;
        j[addr] = u_json::array();
        j[addr].push_back(project);
        save_option(m_wf_imported, j.dump());
        return;
    }

    try
    {
        auto j = u_json::parse(imported);
        if (j.is_object())
        {
            for (auto& [jaddr, jprojects] : j.items())
            {
                if (jaddr == addr && jprojects.is_array())
                {
                    for (const auto& jproject : jprojects)
                    {
                        if (jproject.is_string() && jproject == project)
                            return;
                    }

                    jprojects.push_back(project);
                    save_option(m_wf_imported, j.dump());
                    return;
                }
            }
        }
    }
    catch (const std::exception& ex)
    {
        LogExcep("Failed to parse imported projects: " << ex.what());
    }
}

//////////////////////////////////////////////////////////////////////
////////       Items                     /////////////////////////////
//////////////////////////////////////////////////////////////////////

void SettingsManager::init_items_defaults()
{
    if (!g::pDbManager->get_is_first_run())
        return;

    using namespace tools::str;
    item_save(hash_stable("LoadImage"), DbItemOptionType::Nodes, R"({"display_source":"image","flags":335872})");
    item_save(hash_stable("LoadImageMask"), DbItemOptionType::Nodes, R"({"display_source":"image","flags":335872})");
    item_save(hash_stable("LoadImage //Inspire"), DbItemOptionType::Nodes, R"({"display_source":"image","flags":335872})");
    item_save(hash_stable("Image Load"), DbItemOptionType::Nodes, R"({"display_source":"image_path","flags":335872})");
    item_save(hash_stable("LayerUtility: LoadImage"), DbItemOptionType::Nodes, R"({"display_source":"image","flags":335872})");
    item_save(hash_stable("LayerUtility: LoadMask"), DbItemOptionType::Nodes, R"({"display_source":"image","flags":335872})");
    item_save(hash_stable("LoadImageBase64"), DbItemOptionType::Nodes, R"({"display_source":"image","flags":335872})");

    item_save(hash_stable("LoadImageOutput"), DbItemOptionType::Nodes, R"({"display_source":"image","flags":663552})");

    item_save(hash_stable("LoadVideo"), DbItemOptionType::Nodes, R"({"display_source":"file","flags":331776})");
    item_save(hash_stable("VHS_LoadVideo"), DbItemOptionType::Nodes, R"({"display_source":"video","flags":331776})");
    item_save(hash_stable("VHS_LoadVideoFFmpeg"), DbItemOptionType::Nodes, R"({"display_source":"video","flags":331776})");

    item_save(hash_stable("LoadAudio"), DbItemOptionType::Nodes, R"({"display_source":"audio","flags":329728})");
    item_save(hash_stable("VHS_LoadAudioUpload"), DbItemOptionType::Nodes, R"({"display_source":"audio","flags":329728})");
    item_save(hash_stable("LoadAudio //AudioLDM"), DbItemOptionType::Nodes, R"({"display_source":"audio","flags":329728})");
}

template<typename T>
void items_load_all(std::unordered_map<hid, T>& opts, DbItemOptionType type)
{
    const auto vals = g::pDbManager->item_option_load_all(type);
    for (auto& val : vals)
    {
        opts[val.first].deserialize(val.second.sval);
    }
}

template <typename T>
T items_load(hid type_hash, DbItemOptionType type)
{
    auto val = g::pDbManager->item_option_load(type, type_hash);
    if (!val.has_value())
        return {};
    T items;
    items.deserialize(val.value().sval);
    return items;
}

template <typename T>
T items_load(const char* type_hash, DbItemOptionType type)
{
    return items_load<T>(tools::str::hash_stable(type_hash), type);
}

void SettingsManager::items_pins_preload_all()
{
    items_load_all(m_item_pin_opts, DbItemOptionType::Pins);
    m_item_pin_opts_preloaded = true;
}

const PinOptions& SettingsManager::items_pin_load(hid pin_hash)
{
    if (m_item_pin_opts_preloaded)
        return m_item_pin_opts[pin_hash];

    m_item_pin_opts[pin_hash] = items_load<PinOptions>(pin_hash, DbItemOptionType::Pins);
    return m_item_pin_opts[pin_hash];
}

void SettingsManager::items_nodes_preload_all()
{
    items_load_all(m_item_node_opts, DbItemOptionType::Nodes);
    m_item_node_opts_preloaded = true;
}

const NodeOptions& SettingsManager::items_node_load(hid node_hash)
{
    if (m_item_node_opts_preloaded)
        return m_item_node_opts[node_hash];

    m_item_node_opts[node_hash] = items_load<NodeOptions>(node_hash, DbItemOptionType::Nodes);
    return m_item_node_opts[node_hash];
}

void SettingsManager::items_module_colors_load_all()
{
    const auto vals = g::pDbManager->item_option_load_all(DbItemOptionType::ModuleColors);
    if (vals.empty())
        return;
    m_module_colors.deserialize(vals.front().second.sval);
}

ImU32 SettingsManager::items_module_color_load_bg(hid module_hash) const
{
    uint32_t body_bg = 0, head_bg = 0, head_capt = 0;
    if (m_module_colors.get_colors(module_hash, body_bg, head_bg, head_capt))
        return body_bg;
    return 0;
}

ImU32 SettingsManager::items_module_color_load_capt_bg(hid module_hash) const
{
    uint32_t body_bg = 0, head_bg = 0, head_capt = 0;
    if (m_module_colors.get_colors(module_hash, body_bg, head_bg, head_capt))
        return head_bg;
    return 0;
}

ImU32 SettingsManager::items_module_color_load_capt_text(hid module_hash) const
{
    uint32_t body_bg = 0, head_bg = 0, head_capt = 0;
    if (m_module_colors.get_colors(module_hash, body_bg, head_bg, head_capt))
        return head_capt;
    return 0;
}

void SettingsManager::items_module_color_save(const std::list<hid>& modules_hash, ImU32 color)
{
    LogRAssert(!modules_hash.empty(), "Empty colors list", );
    ImU32 bg_color = 0, head_bg = 0, caption = 0;
    ModuleColors::make_colors(ImGui::ColorConvertU32ToFloat4(color), bg_color, head_bg, caption, 0 == modules_hash.front());
    m_module_colors.set_colors(modules_hash, bg_color, head_bg, caption);
    item_save(0, DbItemOptionType::ModuleColors, m_module_colors.serialize());
}

void SettingsManager::items_module_color_reset(const std::list<hid>& modules_hash)
{
    LogRAssert(!modules_hash.empty(), "Empty colors list", );
    m_module_colors.delete_colors(modules_hash);
    item_save(0, DbItemOptionType::ModuleColors, m_module_colors.serialize());
}

LabelsList SettingsManager::items_wf_labels_load() const
{
    return items_load<LabelsList>(m_items_wf_labels_id, DbItemOptionType::LabelWfs);
}

LabelsList SettingsManager::items_node_labels_load() const
{
    return items_load<LabelsList>(m_items_node_labels_id, DbItemOptionType::LabelNodes);
}

StarredList SettingsManager::items_model_favs_load() const
{
    return items_load<StarredList>(m_items_model_favs_id, DbItemOptionType::StarredModels);
}

StarredList SettingsManager::items_history_favs_load() const
{
    return items_load<StarredList>(m_items_history_favs_id, DbItemOptionType::StarredHistory);
}

void SettingsManager::item_save(hid hash, DbItemOptionType type, int64_t value) const
{
    g::pDbManager->item_option_save(type, hash, DbValue(value));
}

void SettingsManager::item_save(hid hash, DbItemOptionType type, std::string&& value) const
{
    g::pDbManager->item_option_save(type, hash, DbValue(std::move(value)));
}

//////////////////////////////////////////////////////////////////////
////////       Stats                     /////////////////////////////
//////////////////////////////////////////////////////////////////////

void SettingsManager::stat_disks_enabled_save()
{
    std::vector<std::string> merged;
    for (auto& pair : m_stat_enabled_disks_parsed)
    {
        merged.push_back(pair.first);
        merged.push_back(pair.second ? "+" : "-");
    }

    const auto opt = tools::str::merge(merged, '|');
    save_option(m_stat_enabled_disks, opt);
}

} // namespace mngr
