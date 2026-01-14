
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
    m_app_vsync =                                     { "app_vsync_enabled",        bool(true) };

    // ui
    m_ui_open_wnds =                                  { "flow_open_windows",        int64_t(ui::ViewOptsComfyUi | ui::ViewPanelsResetAll) };
    m_ui_nodes_tree_view =                            { "flow_nodes_tree_view",     bool(true) };

    // comfyui
    m_comfyui_host =                                  { "comfyui_host",             std::string("127.0.0.1") };
    m_comfyui_port =                                  { "comfyui_port",             int64_t(8188) };
    m_comfyui_conn_time =                             { "comfyui_conn_time",        int64_t(1) };
    m_comfyui_read_time =                             { "comfyui_read_time",        int64_t(5) };
    m_comfyui_start_server =                          { "comfyui_start_server",     bool(false) };
    m_comfyui_kill_server =                           { "comfyui_kill_server",      bool(false) };
    m_comfyui_parse_address =                         { "comfyui_parse_address",    bool(false) };
    m_comfyui_hide_console =                          { "comfyui_hide_console",     bool(false) };
    m_comfyui_capture_output =                        { "comfyui_capture_output",   bool(false) };
    m_comfy_main_py =                                 { "comfyui_main_py",          std::filesystem::path("") };
    m_comfy_command =                                 { "comfyui_command",          std::string("") };
    m_wf_widget_input_numbers =                       { "wf_widget_input_numbers",  cast::to_i64(WidgetInput::slider) };
    m_wf_widget_input_bools =                         { "wf_widget_input_bools",    cast::to_i64(WidgetInput::select) };

    m_ui_colors.resize(cast::to_szt(AppColor::_count));
    m_ui_colors[cast::to_szt(AppColor::disconnected_menubar_bg)] =             { "ui_color_0",  int64_t(IM_COL32(150, 0, 0, 255)) };
    m_ui_colors[cast::to_szt(AppColor::disconnected_menubar_text)] =           { "ui_color_1",  int64_t(IM_COL32(255, 255, 255, 255)) };
    m_ui_colors[cast::to_szt(AppColor::queue_button)] =                        { "ui_color_2",  int64_t(IM_COL32(0, 0, 150, 255)) };
    m_ui_colors[cast::to_szt(AppColor::queue_text)] =                          { "ui_color_3",  int64_t(IM_COL32(0, 0, 0, 255)) };
    m_ui_colors[cast::to_szt(AppColor::opt_dlg_title_bg)] =                    { "ui_color_4",  int64_t(IM_COL32(0, 128, 128, 255)) };
    m_ui_colors[cast::to_szt(AppColor::opt_dlg_bg)] =                          { "ui_color_5",  int64_t(IM_COL32(0, 0, 0, 255)) };
    m_ui_colors[cast::to_szt(AppColor::panel_bg)] =                            { "ui_color_6",  int64_t(IM_COL32(24, 24, 27, 255)) };
    m_ui_colors[cast::to_szt(AppColor::panel_nodes_hover_group)] =             { "ui_color_7",  int64_t(IM_COL32(220, 220, 220, 50)) };
    m_ui_colors[cast::to_szt(AppColor::panel_nodes_hover_node)] =              { "ui_color_8",  int64_t(IM_COL32(211, 112, 0, 80)) };
    m_ui_colors[cast::to_szt(AppColor::panel_nodes_sel_node)] =                { "ui_color_9",  int64_t(IM_COL32(255, 165, 0, 250)) };
    m_ui_colors[cast::to_szt(AppColor::panel_node_insp_descr_frame)] =         { "ui_color_10", int64_t(IM_COL32(220, 220, 220, 50)) };
    m_ui_colors[cast::to_szt(AppColor::node_header_bg)] =                      { "ui_color_11", int64_t(IM_COL32(45, 50, 60, 255)) };
    m_ui_colors[cast::to_szt(AppColor::node_header_text)] =                    { "ui_color_12", int64_t(IM_COL32(255, 176, 50, 255)) };
    m_ui_colors[cast::to_szt(AppColor::node_body_bg)] =                        { "ui_color_13", int64_t(IM_COL32(30, 30, 30, 200)) };
    m_ui_colors[cast::to_szt(AppColor::node_pin_text)] =                       { "ui_color_14", int64_t(IM_COL32(100, 100, 100, 255)) };
    m_ui_colors[cast::to_szt(AppColor::node_border)] =                         { "ui_color_15", int64_t(IM_COL32(100, 100, 100, 255)) };
    m_ui_colors[cast::to_szt(AppColor::node_widget_hidden_sep)] =              { "ui_color_16", int64_t(IM_COL32(0, 180, 180, 200)) };
    m_ui_colors[cast::to_szt(AppColor::stat_warn)] =                           { "ui_color_17", int64_t(IM_COL32(255, 200, 50, 255)) };
    m_ui_colors[cast::to_szt(AppColor::stat_crit)] =                           { "ui_color_18", int64_t(IM_COL32(255, 50, 50, 255)) };
    m_ui_colors[cast::to_szt(AppColor::stat_sep_capt)] =                       { "ui_color_19", int64_t(IM_COL32(0, 180, 180, 200)) };
    m_ui_colors[cast::to_szt(AppColor::stat_sep_subtitle)] =                   { "ui_color_20", int64_t(IM_COL32(255, 255, 255, 200)) };
    m_ui_colors[cast::to_szt(AppColor::popup_bg)] =                            { "ui_color_21", int64_t(IM_COL32(50, 50, 50, 255)) };
    m_ui_colors[cast::to_szt(AppColor::menubar_bg)] =                          { "ui_color_22", int64_t(IM_COL32(50, 50, 50, 255)) };
    m_ui_colors[cast::to_szt(AppColor::opt_subtitle_sep)] =                    { "ui_color_23", int64_t(IM_COL32(0, 128, 128, 255)) };
    m_ui_colors[cast::to_szt(AppColor::opt_separator)] =                       { "ui_color_24", int64_t(IM_COL32(0, 180, 180, 255)) };
    m_ui_colors[cast::to_szt(AppColor::opt_selected_item)] =                   { "ui_color_25", int64_t(IM_COL32(255, 165, 0, 255)) };
    m_ui_colors[cast::to_szt(AppColor::opt_help_mark)] =                       { "ui_color_26", int64_t(IM_COL32(50, 50, 50, 255)) };
    m_ui_colors[cast::to_szt(AppColor::opt_help_text)] =                       { "ui_color_27", int64_t(IM_COL32(0, 128, 128, 200)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_int)] =                       { "ui_color_28", int64_t(IM_COL32(40, 200, 40, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_float)] =                     { "ui_color_29", int64_t(IM_COL32(40, 200, 40, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_string)] =                    { "ui_color_30", int64_t(IM_COL32(200, 200, 200, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_boolean)] =                   { "ui_color_31", int64_t(IM_COL32(180, 180, 180, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_any)] =                       { "ui_color_32", int64_t(IM_COL32_WHITE) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_combo)] =                     { "ui_color_33", int64_t(IM_COL32(200, 200, 200, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_color)] =                     { "ui_color_34", int64_t(IM_COL32(255, 235, 59, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_number)] =                    { "ui_color_35", int64_t(IM_COL32(46, 204, 113, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_floats)] =                    { "ui_color_36", int64_t(IM_COL32(26, 188, 156, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_seed)] =                      { "ui_color_37", int64_t(IM_COL32(52, 152, 219, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_model)] =                     { "ui_color_38", int64_t(IM_COL32(111, 108, 155, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_latent)] =                    { "ui_color_39", int64_t(IM_COL32(176, 83, 112, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_vae)] =                       { "ui_color_40", int64_t(IM_COL32(166, 46, 46, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_clip)] =                      { "ui_color_41", int64_t(IM_COL32(227, 184, 54, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_image)] =                     { "ui_color_42", int64_t(IM_COL32(91, 135, 163, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_mask)] =                      { "ui_color_43", int64_t(IM_COL32(93, 158, 89, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_conditioning)] =              { "ui_color_44", int64_t(IM_COL32(195, 134, 52, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_clip_vision)] =               { "ui_color_45", int64_t(IM_COL32(100, 100, 100, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_control_net)] =               { "ui_color_46", int64_t(IM_COL32(90, 160, 140, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_audio)] =                     { "ui_color_47", int64_t(IM_COL32(255, 160, 0, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_video)] =                     { "ui_color_48", int64_t(IM_COL32(41, 128, 185, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_ipadapter)] =                 { "ui_color_49", int64_t(IM_COL32(215, 189, 226, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_insightface)] =               { "ui_color_50", int64_t(IM_COL32(231, 76, 60, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_control_net_stack)] =         { "ui_color_51", int64_t(IM_COL32(39, 174, 96, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_clip_vision_output)] =        { "ui_color_52", int64_t(IM_COL32(241, 196, 15, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_noise)] =                     { "ui_color_53", int64_t(IM_COL32(149, 165, 166, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_sigmas)] =                    { "ui_color_54", int64_t(IM_COL32(140, 100, 80, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_sampler)] =                   { "ui_color_55", int64_t(IM_COL32(230, 126, 34, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_pipe_line)] =                 { "ui_color_56", int64_t(IM_COL32(255, 215, 0, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_lora_stack)] =                { "ui_color_57", int64_t(IM_COL32(255, 105, 180, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_lora_list)] =                 { "ui_color_58", int64_t(IM_COL32(219, 112, 147, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_segs)] =                      { "ui_color_59", int64_t(IM_COL32(0, 255, 255, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_bbox)] =                      { "ui_color_60", int64_t(IM_COL32(127, 255, 0, 255)) };
    m_ui_colors[cast::to_szt(AppColor::pin_color_bus)] =                       { "ui_color_61", int64_t(IM_COL32(255, 215, 0, 255)) };
    m_ui_colors[cast::to_szt(AppColor::restart_button)] =                      { "ui_color_62", int64_t(IM_COL32(120, 0, 0, 255)) };
    m_ui_colors[cast::to_szt(AppColor::url_text_default)] =                    { "ui_color_63", int64_t(IM_COL32(51, 127, 229, 255)) };
    m_ui_colors[cast::to_szt(AppColor::url_text_hovered)] =                    { "ui_color_64", int64_t(IM_COL32(102, 153, 255, 255)) };


    m_stat_enabled_disks =                                                     { "stat_enabled_disks",       std::string("") };
    m_stat_limits_warn.resize(cast::to_szt(MetricLimit::_count));
    m_stat_limits_crit.resize(cast::to_szt(MetricLimit::_count));
    m_stat_limits_warn[cast::to_szt(MetricLimit::local_ram)] =                 { "stat_local_ram_warn",      int64_t(2147483648) };     // 2gb
    m_stat_limits_crit[cast::to_szt(MetricLimit::local_ram)] =                 { "stat_local_ram_crit",      int64_t(524288000) };      // 500 mb
    m_stat_limits_warn[cast::to_szt(MetricLimit::local_disk)] =                { "stat_disk_warn",           int64_t(10737418240) };    // 10 gb
    m_stat_limits_crit[cast::to_szt(MetricLimit::local_disk)] =                { "stat_disk_crit",           int64_t(2147483648) };     // 2 gb
    m_stat_limits_warn[cast::to_szt(MetricLimit::local_fps)] =                 { "stat_fps_warn",            int64_t(200) };
    m_stat_limits_crit[cast::to_szt(MetricLimit::local_fps)] =                 { "stat_fps_crit",            int64_t(1000) };
    m_stat_limits_warn[cast::to_szt(MetricLimit::server_ram)] =                { "stat_server_ram_warn",     int64_t(3221225472) };     // 3 gb
    m_stat_limits_crit[cast::to_szt(MetricLimit::server_ram)] =                { "stat_server_ram_crit",     int64_t(1073741824) };     // 1 gb
    m_stat_limits_warn[cast::to_szt(MetricLimit::server_vram)] =               { "stat_vram_warn",           int64_t(1073741824) };     // 1 gb
    m_stat_limits_crit[cast::to_szt(MetricLimit::server_vram)] =               { "stat_vram_crit",           int64_t(209715200) };      // 200 mb
    m_stat_limits_warn[cast::to_szt(MetricLimit::server_pytorch)] =            { "stat_pytorch_warn",        int64_t(524288000) };      // 500 mb
    m_stat_limits_crit[cast::to_szt(MetricLimit::server_pytorch)] =            { "stat_pytorch_crit",        int64_t(104857600) };      // 100 mb
    m_stat_limits_warn[cast::to_szt(MetricLimit::server_ping)] =               { "stat_ping_warn",           int64_t(150) };
    m_stat_limits_crit[cast::to_szt(MetricLimit::server_ping)] =               { "stat_ping_crit",           int64_t(400) };

    std::vector<OptionValue*> values;
    values.reserve(100);
    values.push_back(&m_app_vsync);
    values.push_back(&m_ui_open_wnds);
    values.push_back(&m_ui_nodes_tree_view);
    values.push_back(&m_comfyui_host);
    values.push_back(&m_comfyui_port);
    values.push_back(&m_comfyui_conn_time);
    values.push_back(&m_comfyui_read_time);
    values.push_back(&m_comfyui_start_server);
    values.push_back(&m_comfyui_kill_server);
    values.push_back(&m_comfyui_parse_address);
    values.push_back(&m_comfyui_hide_console);
    values.push_back(&m_comfyui_capture_output);
    values.push_back(&m_comfy_main_py);
    values.push_back(&m_comfy_command);
    values.push_back(&m_wf_widget_input_numbers);
    values.push_back(&m_wf_widget_input_bools);
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
    auto db_values = g::pDbManager->load_all_options();
    bool do_update = false;

    for (auto& val : values)
    {
        auto it = std::find_if(db_values.begin(), db_values.end(), [&val](mngr::DbOption& db_opt) { return db_opt.name == val->db_name; });
        if (it != db_values.end())
        {
            auto& db_opt = it->val.value();

            if (std::holds_alternative<int64_t>(val->value))
                val->value = db_opt.ival;
            else if (std::holds_alternative<std::string>(val->value))
                val->value = db_opt.sval;
            else if (std::holds_alternative<std::filesystem::path>(val->value))
                val->value = std::filesystem::u8path(db_opt.sval);
            else if (std::holds_alternative<bool>(val->value))
                val->value = bool(db_opt.ival == 1);
            else
                LogError(ExcepCategory::logic, "Unhandled option type");
        }
        else
        {
            do_update = true;
            db_values.push_back({});
            mngr::DbOption& db_opt = db_values.back();
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
                LogError(ExcepCategory::logic, "Unhandled option type");
        }
    }

    if (do_update)
    {
        g::pDbManager->save_all_options(db_values);
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
        LogExcep(ExcepCategory::logic, "Value " << option.db_name << " is not a string");
    }
    else if (*std::get_if<std::string>(&option.value) != new_str)
    {
        *std::get_if<std::string>(&option.value) = new_str;
        g::pDbManager->save_option(option.db_name, { new_str });
    }
}

void SettingsManager::save_option(OptionValue& option, int64_t new_int)
{
    if (!std::holds_alternative<int64_t>(option.value))
    {
        LogExcep(ExcepCategory::logic, "Value " << option.db_name << " is not a number");
    }
    else if (*std::get_if<int64_t>(&option.value) != new_int)
    {
        *std::get_if<int64_t>(&option.value) = new_int;
        g::pDbManager->save_option(option.db_name, { new_int });
    }
}

void SettingsManager::save_option(OptionValue& option, const std::filesystem::path& new_path)
{
    if (!std::holds_alternative<std::filesystem::path>(option.value))
    {
        LogExcep(ExcepCategory::logic, "Value " << option.db_name << " is not a path");
    }
    else if (*std::get_if<std::filesystem::path>(&option.value) != new_path)
    {
        *std::get_if<std::filesystem::path>(&option.value) = new_path;
        g::pDbManager->save_option(option.db_name, { new_path.u8string() });
    }
}

//////////////////////////////////////////////////////////////////////
////////       UI                        /////////////////////////////
//////////////////////////////////////////////////////////////////////

void SettingsManager::ui_open_wnds_save(int64_t windows)
{
    save_option(m_ui_open_wnds, windows & ui::ViewAllPanels);
}

//////////////////////////////////////////////////////////////////////
////////       Nodes                     /////////////////////////////
//////////////////////////////////////////////////////////////////////

void SettingsManager::nd_node_options_preload()
{
    nd_node_options_release();
    const auto res = g::pDbManager->load_all_node_options(mngr::NodeOption::advanced);
    for (auto& [id, value] : res)
    {
        m_hashed_pin_advanced[id] = value.ival == 1;
    }
}

void SettingsManager::nd_pin_advanced_save(hid pin_id, bool advanced) const
{
    g::pDbManager->save_node_option(pin_id, mngr::NodeOption::advanced, { advanced ? 1 : 0 });
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
