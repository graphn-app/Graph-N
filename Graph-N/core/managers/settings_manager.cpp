
#include "core/managers/settings_manager.h"
#include "core/managers/db_manager.h"
#include "ui/graph-n_flow.h"

namespace mngr
{

SettingsManager::~SettingsManager()
{
    LogF;
}

bool SettingsManager::init()
{
    // app
    m_app_vsync =                                      { "vsync_enabled",           bool(true) };
    m_app_fps_limit =                                  { "fps_limit",               bool(false) };
    m_app_fps_limit_val =                              { "fps_limit_value",         int64_t(30) };
    m_app_autoupdate =                                 { "autoupdate",              bool(true) };
    m_app_scale =                                      { "ui_scale",                int64_t(100) };

    // ui
    m_ui_open_wnds =                                  { "open_windows",             int64_t(ui::ViewPanelsResetAll) };
    m_ui_nodes_tree_view =                            { "nodes_tree_view",          bool(true) };
    m_ui_theme =                                      { "theme",                    int64_t(0) };
    m_ui_activity_bar_mode =                          { "activity_bar_mode",        int64_t(cast::to_i64(ActivityBarMode::Smart)) };
    m_ui_activity_bar_big_icons =                     { "activity_bar_big_icons",   bool(false) };
    m_ui_activity_bar_show_restart =                  { "activity_bar_add_restart", bool(false) };

    // comfyui
    m_comfyui_host =                                  { "comfyui_host",             std::string("127.0.0.1") };
    m_comfyui_port =                                  { "comfyui_port",             int64_t(8188) };
    m_comfyui_conn_time =                             { "comfyui_conn_time",        int64_t(1) };
    m_comfyui_read_time =                             { "comfyui_read_time",        int64_t(5) };
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
    m_wf_widget_input_numbers =                       { "widget_input_numbers",     cast::to_i64(WidgetInput::Slider) };
    m_wf_widget_input_bools =                         { "widget_input_bools",       cast::to_i64(WidgetInput::Select) };
    m_wf_active =                                     { "active_wf",                std::string("") };
    m_wf_show_grid =                                  { "show_grid",                bool(true) };
    m_wf_snap_nodes =                                 { "snap_nodes",               bool(false) };
    m_wf_link_thickness =                             { "link_thickness",           int64_t(2) };

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
    m_ui_colors[cast::to_szt(AppColor::OptDlgTitleBg)] =                     { "color_opt_dlg_title_bg",  int64_t(IM_COL32(0, 128, 128, 255)) };
    m_ui_colors[cast::to_szt(AppColor::OptDlgBg)] =                          { "color_opt_dlg_bg",  int64_t(IM_COL32(0, 0, 0, 255)) };
    m_ui_colors[cast::to_szt(AppColor::PanelBg)] =                           { "color_panel_bg",  int64_t(IM_COL32(24, 24, 27, 255)) };
    m_ui_colors[cast::to_szt(AppColor::PanelActiveWfTab)] =                  { "color_active_wf_tab",  int64_t(IM_COL32(194, 126, 0, 120)) };
    m_ui_colors[cast::to_szt(AppColor::PanelActiveHoveredWfTab)] =           { "color_active_hovered_wf_tab",  int64_t(IM_COL32(194, 126, 0, 140)) };
    m_ui_colors[cast::to_szt(AppColor::PanelNodesHoverGroup)] =              { "color_panel_nodes_hover_group",  int64_t(IM_COL32(220, 220, 220, 50)) };
    m_ui_colors[cast::to_szt(AppColor::PanelNodesHoverNode)] =               { "color_panel_nodes_hover_node",  int64_t(IM_COL32(211, 112, 0, 80)) };
    m_ui_colors[cast::to_szt(AppColor::PanelNodesSelNode)] =                 { "color_panel_nodes_sel_node",  int64_t(IM_COL32(255, 165, 0, 250)) };
    m_ui_colors[cast::to_szt(AppColor::PanelNodesInspDescrFrame)] =          { "color_panel_node_insp_descr_frame", int64_t(IM_COL32(220, 220, 220, 50)) };
    m_ui_colors[cast::to_szt(AppColor::NodeHeaderBg)] =                      { "color_node_header_bg", int64_t(IM_COL32(45, 50, 60, 255)) };
    m_ui_colors[cast::to_szt(AppColor::NodeHeaderText)] =                    { "color_node_header_text", int64_t(IM_COL32(255, 176, 50, 255)) };
    m_ui_colors[cast::to_szt(AppColor::NodeBodyBg)] =                        { "color_node_body_bg", int64_t(IM_COL32(30, 30, 30, 255)) };
    m_ui_colors[cast::to_szt(AppColor::NodePinText)] =                       { "color_node_pin_text", int64_t(IM_COL32(100, 100, 100, 255)) };
    m_ui_colors[cast::to_szt(AppColor::NodeBorder)] =                        { "color_node_border", int64_t(IM_COL32(100, 100, 100, 255)) };
    m_ui_colors[cast::to_szt(AppColor::NodeWidgetHiddenSep)] =               { "color_node_widget_hidden_sep", int64_t(IM_COL32(0, 180, 180, 200)) };
    m_ui_colors[cast::to_szt(AppColor::NodeDescription)] =                   { "color_node_description", int64_t(IM_COL32(0, 180, 180, 255)) };
    m_ui_colors[cast::to_szt(AppColor::NodeHeaderIcon)] =                    { "color_node_header_icon", int64_t(IM_COL32(0, 180, 180, 255)) };
    m_ui_colors[cast::to_szt(AppColor::NodeBorderMuted)] =                   { "color_node_border_muted", int64_t(IM_COL32(255, 50, 50, 255)) };
    m_ui_colors[cast::to_szt(AppColor::NodeBorderBypassed)] =                { "color_node_border_bypassed", int64_t(IM_COL32(200, 50, 255, 255)) };
    m_ui_colors[cast::to_szt(AppColor::StatNorm)] =                          { "color_stat_norm", int64_t(IM_COL32(255, 255, 255, 150)) };
    m_ui_colors[cast::to_szt(AppColor::StatWarn)] =                          { "color_stat_warn", int64_t(IM_COL32(255, 200, 50, 255)) };
    m_ui_colors[cast::to_szt(AppColor::StatCrit)] =                          { "color_stat_crit", int64_t(IM_COL32(255, 50, 50, 255)) };
    m_ui_colors[cast::to_szt(AppColor::StatSepCapt)] =                       { "color_stat_sep_capt", int64_t(IM_COL32(0, 180, 180, 200)) };
    m_ui_colors[cast::to_szt(AppColor::StatSepSubtitle)] =                   { "color_stat_sep_subtitle", int64_t(IM_COL32(255, 255, 255, 150)) };
    m_ui_colors[cast::to_szt(AppColor::PopupBg)] =                           { "color_popup_bg", int64_t(IM_COL32(50, 50, 50, 255)) };
    m_ui_colors[cast::to_szt(AppColor::MenubarBg)] =                         { "color_menubar_bg", int64_t(IM_COL32(50, 50, 50, 255)) };
    m_ui_colors[cast::to_szt(AppColor::OptSeparator)] =                      { "color_opt_separator", int64_t(IM_COL32(0, 180, 180, 255)) };
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

    // stats
    m_stat_limits_warn.resize(cast::to_szt(MetricLimit::_count));
    m_stat_limits_crit.resize(cast::to_szt(MetricLimit::_count));
    m_stat_limits_warn[cast::to_szt(MetricLimit::LocalRAM)] =                 { "stat_local_ram_warn",      int64_t(2147483648) };     // 2gb
    m_stat_limits_crit[cast::to_szt(MetricLimit::LocalRAM)] =                 { "stat_local_ram_crit",      int64_t(524288000) };      // 500 mb
    m_stat_limits_warn[cast::to_szt(MetricLimit::LocalDisk)] =                { "stat_disk_warn",           int64_t(10737418240) };    // 10 gb
    m_stat_limits_crit[cast::to_szt(MetricLimit::LocalDisk)] =                { "stat_disk_crit",           int64_t(2147483648) };     // 2 gb
    m_stat_limits_warn[cast::to_szt(MetricLimit::LocalFPS)] =                 { "stat_fps_warn",            int64_t(200) };
    m_stat_limits_crit[cast::to_szt(MetricLimit::LocalFPS)] =                 { "stat_fps_crit",            int64_t(1000) };
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
    values.push_back(&m_app_autoupdate);
    values.push_back(&m_ui_open_wnds);
    values.push_back(&m_ui_nodes_tree_view);
    values.push_back(&m_ui_theme);
    values.push_back(&m_ui_activity_bar_mode);
    values.push_back(&m_ui_activity_bar_big_icons);
    values.push_back(&m_ui_activity_bar_show_restart);
    values.push_back(&m_comfyui_host);
    values.push_back(&m_comfyui_port);
    values.push_back(&m_comfyui_conn_time);
    values.push_back(&m_comfyui_read_time);
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
    values.push_back(&m_wf_snap_nodes);
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
    init_disks();
    return true;
}

void SettingsManager::init_options(std::vector<OptionValue*>& values)
{
    auto db_values = g::pDbManager->app_option_load_all();
    bool do_update = false;

    for (auto& val : values)
    {
        auto it = std::find_if(db_values.begin(), db_values.end(), [&val](DbAppOptionData& db_opt) { return db_opt.name == val->db_name; });
        if (it != db_values.end())
        {
            if (std::holds_alternative<int64_t>(val->value))
                val->value = it->val.ival;
            else if (std::holds_alternative<std::string>(val->value))
                val->value = it->val.sval;
            else if (std::holds_alternative<std::filesystem::path>(val->value))
                val->value = std::filesystem::u8path(it->val.sval);
            else if (std::holds_alternative<bool>(val->value))
                val->value = bool(it->val.ival == 1);
            else
                LogError(ExcepCategory::Logic, "Unhandled option type");
        }
        else
        {
            do_update = true;
            DbAppOptionData db_opt;
            db_opt.name = val->db_name;

            if (std::holds_alternative<int64_t>(val->value))
                db_opt.val = *std::get_if<int64_t>(&val->value);
            else if (std::holds_alternative<std::string>(val->value))
                db_opt.val = *std::get_if<std::string>(&val->value);
            else if (std::holds_alternative<std::filesystem::path>(val->value))
                db_opt.val = std::get_if<std::filesystem::path>(&val->value)->u8string();
            else if (std::holds_alternative<bool>(val->value))
                db_opt.val = int64_t(*std::get_if<bool>(&val->value));
            else
                LogError(ExcepCategory::Logic, "Unhandled option type");

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
    const auto loaded = tools::str::split(*std::get_if<std::string>(&m_stat_enabled_disks.value), '|');
    const auto installed = tools::api::scan_for_disks(); // always scan for new disks
    for (auto& installed_name : installed)
    {
        auto pair = std::make_pair(installed_name, true);
        auto it = std::find_if(loaded.begin(), loaded.end(), [&installed_name](const std::string& loaded_name) { return loaded_name == installed_name; });
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

void SettingsManager::save_option(OptionValue& option, const std::string& new_str)
{
    if (!std::holds_alternative<std::string>(option.value))
    {
        LogExcep(ExcepCategory::Logic, "Value " << option.db_name << " is not a string");
    }
    else if (*std::get_if<std::string>(&option.value) != new_str)
    {
        *std::get_if<std::string>(&option.value) = new_str;
        g::pDbManager->app_option_save(option.db_name, { new_str });
    }
}

void SettingsManager::save_option(OptionValue& option, int64_t new_int)
{
    if (!std::holds_alternative<int64_t>(option.value))
    {
        LogExcep(ExcepCategory::Logic, "Value " << option.db_name << " is not a number");
    }
    else if (*std::get_if<int64_t>(&option.value) != new_int)
    {
        *std::get_if<int64_t>(&option.value) = new_int;
        g::pDbManager->app_option_save(option.db_name, { new_int });
    }
}

void SettingsManager::save_option(OptionValue& option, bool new_bool)
{
    if (!std::holds_alternative<bool>(option.value))
    {
        LogExcep(ExcepCategory::Logic, "Value " << option.db_name << " is not a boolean");
    }
    else if (*std::get_if<bool>(&option.value) != new_bool)
    {
        *std::get_if<bool>(&option.value) = new_bool;
        g::pDbManager->app_option_save(option.db_name, { new_bool ? 1 : 0 });
    }
}

void SettingsManager::save_option(OptionValue& option, const std::filesystem::path& new_path)
{
    if (!std::holds_alternative<std::filesystem::path>(option.value))
    {
        LogExcep(ExcepCategory::Logic, "Value " << option.db_name << " is not a path");
    }
    else if (*std::get_if<std::filesystem::path>(&option.value) != new_path)
    {
        *std::get_if<std::filesystem::path>(&option.value) = new_path;
        g::pDbManager->app_option_save(option.db_name, { new_path.u8string() });
    }
}

//////////////////////////////////////////////////////////////////////
////////       UI                        /////////////////////////////
//////////////////////////////////////////////////////////////////////

void SettingsManager::ui_open_wnds_save(int64_t windows)
{
    windows &= ~ui::ViewAlwaysClose;
    save_option(m_ui_open_wnds, windows);
}

//////////////////////////////////////////////////////////////////////
////////       Items                     /////////////////////////////
//////////////////////////////////////////////////////////////////////

void SettingsManager::pin_advanced_preload()
{
    auto vals = g::pDbManager->item_option_load_all(DbItemOptionType::PinAdvanced);
    for (auto& val : vals)
    {
        m_item_preload_pin_adv[val.first] = val.second.ival == 1;
    }
}

void SettingsManager::model_starred_preload()
{
    auto vals = g::pDbManager->item_option_load_all(DbItemOptionType::ModelStarred);
    for (auto& val : vals)
    {
        m_item_preload_model_starred[val.first] = val.second.ival == 1;
    }
}

std::optional<DbValue> SettingsManager::item_load(hid id, DbItemOptionType type) const
{
    return g::pDbManager->item_option_load(id, type);
}

void SettingsManager::item_save(hid id, DbItemOptionType type, int64_t value) const
{
    g::pDbManager->item_option_save(id, type, DbValue(value));
}

void SettingsManager::item_save(hid id, DbItemOptionType type, std::string&& value) const
{
    g::pDbManager->item_option_save(id, type, DbValue(std::move(value)));
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
