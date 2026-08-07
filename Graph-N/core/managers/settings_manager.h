
#pragma once
#include "core/base/structs.h"
#include "core/managers/manager.h"

namespace mngr
{

class SettingsManager final : public Manager
{
public:
    SettingsManager() noexcept = default;
    ~SettingsManager() noexcept override;

    bool init() override;

    // app
    bool app_autoupdate_load() const noexcept { return *std::get_if<bool>(&m_app_autoupdate.value); }
    void app_autoupdate_save(bool enabled) { save_option(m_app_autoupdate, bool(enabled)); }

    bool app_vsync_load() const noexcept { return *std::get_if<bool>(&m_app_vsync.value); }
    void app_vsync_save(bool enabled) { save_option(m_app_vsync, bool(enabled)); }

    bool app_fps_limit_load() const noexcept { return *std::get_if<bool>(&m_app_fps_limit.value); }
    void app_fps_limit_save(bool enabled) { save_option(m_app_fps_limit, bool(enabled)); }

    int64_t app_fps_limit_val_load() const noexcept { return *std::get_if<int64_t>(&m_app_fps_limit_val.value); }
    void app_fps_limit_val_save(int64_t limit) { save_option(m_app_fps_limit_val, int64_t(limit)); }

    int64_t app_scale_load() const noexcept { return *std::get_if<int64_t>(&m_app_scale.value); }
    void app_scale_save(int64_t scale) { save_option(m_app_scale, int64_t(scale)); }

    // ui
    ImU32 color_load(AppColor color) const noexcept 
    { 
        const size_t index = cast::to_szt(color);
        assert(index < m_ui_colors.size());
        if (index >= m_ui_colors.size())
            return IM_COL32(40, 200, 40, 255);
        return cast::to_u32(*std::get_if<int64_t>(&m_ui_colors[index].value)); 
    }

    int64_t ui_open_wnds_load() const noexcept { return *std::get_if<int64_t>(&m_ui_open_wnds.value); }
    void ui_open_wnds_save(int64_t windows);

    bool ui_nodes_pane_tree_view_load() const noexcept { return *std::get_if<bool>(&m_ui_nodes_tree_view.value); }
    void ui_nodes_pane_tree_view_save(bool tree_view) { save_option(m_ui_nodes_tree_view, bool(tree_view)); }

    int64_t ui_theme_load() const noexcept { return *std::get_if<int64_t>(&m_ui_theme.value); }
    void ui_theme_save(int64_t theme) { save_option(m_ui_theme, int64_t(theme)); }

    ActivityBarMode ui_activity_bar_mode_load() const noexcept { return cast::to_enum<ActivityBarMode>(*std::get_if<int64_t>(&m_ui_activity_bar_mode.value)); }
    void ui_activity_bar_mode_save(ActivityBarMode mode) { save_option(m_ui_activity_bar_mode, int64_t(cast::to_i64(mode))); }

    bool ui_activity_bar_big_icons_load() const noexcept { return *std::get_if<bool>(&m_ui_activity_bar_big_icons.value); }
    void ui_activity_bar_big_icons_save(bool big) { save_option(m_ui_activity_bar_big_icons, bool(big)); }

    bool ui_activity_bar_show_restart_load() const noexcept { return *std::get_if<bool>(&m_ui_activity_bar_show_restart.value); }
    void ui_activity_bar_show_restart_save(bool show) { save_option(m_ui_activity_bar_show_restart, bool(show)); }
    
    std::vector<ui::KeyBinding> ui_hotkeys_load() noexcept;
    void ui_hotkeys_save(const std::vector<ui::KeyBinding>& hotkeys) { save_option(m_ui_hotkeys, format_hotkeys_to_json(hotkeys)); }

    // comfyui
    const std::string& comfyui_host_load() const noexcept { return *std::get_if<std::string>(&m_comfyui_host.value); }
    std::string comfyui_host_load_safe();
    void comfyui_host_save(const std::string& host) { save_option(m_comfyui_host, host); }

    int64_t comfyui_port_load() const noexcept { return *std::get_if<int64_t>(&m_comfyui_port.value); }
    int64_t comfyui_port_load_safe();
    void comfyui_port_save(int64_t port) { save_option(m_comfyui_port, int64_t(port)); }

    int64_t comfyui_connect_timeout_load() const noexcept { return *std::get_if<int64_t>(&m_comfyui_conn_time.value); }
    int64_t comfyui_connect_timeout_load_safe();
    void comfyui_connect_timeout_save(int64_t timeout) { save_option(m_comfyui_conn_time, int64_t(timeout)); }

    bool comfyui_start_server_load() const noexcept { return *std::get_if<bool>(&m_comfyui_start_server.value); }
    void comfyui_start_server_save(bool start) { save_option(m_comfyui_start_server, bool(start)); }

    const std::filesystem::path& comfyui_python_load() const noexcept { return *std::get_if<std::filesystem::path>(&m_comfyui_python.value); }
    void comfyui_python_save(const std::filesystem::path& python) { save_option(m_comfyui_python, python); }

    const std::string& comfyui_python_args_load() const noexcept { return *std::get_if<std::string>(&m_comfyui_python_args.value); }
    void comfyui_python_args_save(const std::string& python) { save_option(m_comfyui_python_args, python); }

    bool comfyui_kill_server_load() const noexcept { return *std::get_if<bool>(&m_comfyui_kill_server.value); }
    void comfyui_kill_server_save(bool kill) { save_option(m_comfyui_kill_server, bool(kill)); }

    bool comfyui_hide_console_load() const noexcept { return *std::get_if<bool>(&m_comfyui_hide_console.value); }
    void comfyui_hide_console_save(bool hide) { save_option(m_comfyui_hide_console, bool(hide)); }

    bool comfyui_capture_output_load() const noexcept { return *std::get_if<bool>(&m_comfyui_capture_output.value); }
    void comfyui_capture_output_save(bool capture) { save_option(m_comfyui_capture_output, bool(capture)); }

    bool comfyui_autohide_output_load() const noexcept { return *std::get_if<bool>(&m_comfyui_auto_hide_output.value); }
    void comfyui_autohide_output_save(bool hide) { save_option(m_comfyui_auto_hide_output, bool(hide)); }

    bool comfyui_parse_address_load() const noexcept { return *std::get_if<bool>(&m_comfyui_parse_address.value); }
    void comfyui_parse_address_save(bool parse) { save_option(m_comfyui_parse_address, bool(parse)); }

    const std::filesystem::path& comfyui_main_py_load() const noexcept { return *std::get_if<std::filesystem::path>(&m_comfyui_main_py.value); }
    void comfyui_main_py_save(const std::filesystem::path& main_py) { save_option(m_comfyui_main_py, main_py); }

    const std::string& comfyui_comfyui_args_load() const noexcept { return *std::get_if<std::string>(&m_comfyui_comfyui_args.value); }
    void comfyui_comfyui_args_save(const std::string& cmd) { save_option(m_comfyui_comfyui_args, cmd); }

    const std::filesystem::path& comfyui_workdir_load() const noexcept { return *std::get_if<std::filesystem::path>(&m_comfyui_workdir.value); }
    void comfyui_workdir_save(const std::filesystem::path& path) { save_option(m_comfyui_workdir, path); }

    const std::filesystem::path& comfyui_output_load() const noexcept { return *std::get_if<std::filesystem::path>(&m_comfyui_output.value); }
    void comfyui_output_save(const std::filesystem::path& path) { save_option(m_comfyui_output, path); }

    int64_t comfyui_ping_interval_load() const noexcept { return *std::get_if<int64_t>(&m_comfyui_ping_interval.value); }
    void comfyui_ping_interval_save(int64_t interval) { save_option(m_comfyui_ping_interval, int64_t(interval)); }

    // wf
    WidgetInput wf_widget_input_numbers_load() const noexcept { return cast::to_enum<WidgetInput>(*std::get_if<int64_t>(&m_wf_widget_input_numbers.value)); }
    void wf_widget_input_numbers_save(WidgetInput numbers) { save_option(m_wf_widget_input_numbers, cast::to_i64<>(numbers)); }

    WidgetInput wf_widget_input_bools_load() const noexcept { return cast::to_enum<WidgetInput>(*std::get_if<int64_t>(&m_wf_widget_input_bools.value)); }
    void wf_widget_input_bools_save(WidgetInput bools) { save_option(m_wf_widget_input_bools, cast::to_i64<>(bools)); }

    std::list<std::pair<std::string, bool>> wf_active_load() const noexcept;
    void wf_active_save(const std::list<std::pair<std::string, bool>>& active);

    bool wf_show_grid_load() const noexcept { return *std::get_if<bool>(&m_wf_show_grid.value); }
    void wf_show_grid_save(bool show) { save_option(m_wf_show_grid, bool(show)); }

    bool wf_snap_nodes_load() const noexcept { return *std::get_if<bool>(&m_wf_snap_nodes.value); }
    void wf_snap_nodes_save(bool snap) { save_option(m_wf_snap_nodes, bool(snap)); }

    int64_t wf_link_thickness_load() const noexcept { return *std::get_if<int64_t>(&m_wf_link_thickness.value); }
    void wf_link_thickness_save(int64_t timeout) { save_option(m_wf_link_thickness, int64_t(timeout)); }

    bool wf_imported_load(const std::string& project) const noexcept;
    void wf_imported_save(const std::string& project);

    // queue
    std::set<std::string> queue_favorited_load() const noexcept;
    std::set<std::string> queue_favorited_save(const std::string& prompt_id, bool add);

    int64_t queue_history_num_load() const noexcept { return *std::get_if<int64_t>(&m_queue_history_num.value); }
    void queue_history_num_save(int64_t num) { save_option(m_queue_history_num, int64_t(num)); }

    // logs
    int64_t log_db_backup_num_load() const noexcept { return *std::get_if<int64_t>(&m_log_db_backup_num.value); }
    void log_db_backup_num_save(int64_t num) { save_option(m_log_db_backup_num, int64_t(num)); }

    int64_t log_app_log_num_load() const noexcept { return *std::get_if<int64_t>(&m_log_app_log_num.value); }
    void log_app_log_num_save(int64_t num) { save_option(m_log_app_log_num, int64_t(num)); }

    int64_t log_comfyui_log_num_load() const noexcept { return *std::get_if<int64_t>(&m_log_comfyui_log_num.value); }
    void log_comfyui_log_num_save(int64_t num) { save_option(m_log_comfyui_log_num, int64_t(num)); }

    int64_t log_comfyui_log_in_ram_load() const noexcept { return *std::get_if<int64_t>(&m_log_comfyui_log_in_ram.value); }
    void log_comfyui_log_in_ram_save(int64_t num) { save_option(m_log_comfyui_log_in_ram, int64_t(num)); }

    // items
    void pin_advanced_preload();
    void pin_advanced_release() noexcept { m_item_preload_pin_adv.clear(); }
    std::optional<bool> pin_advanced_load(hid pin_hash) const { auto it = m_item_preload_pin_adv.find(pin_hash); return (it == m_item_preload_pin_adv.end() ? std::nullopt : std::optional<bool>(it->second)); }
    void pin_advanced_save(hid pin_hash, bool is_adv) const { item_save(pin_hash, DbItemOptionType::PinAdvanced, is_adv); }

    void model_starred_preload();
    void model_starred_release() noexcept { m_item_preload_model_starred.clear(); }
    std::optional<bool> model_starred_load(hid model_id) const { auto it = m_item_preload_model_starred.find(model_id); return (it == m_item_preload_model_starred.end() ? std::nullopt : std::optional<bool>(it->second)); }
    void model_starred_save(hid model_hash, bool is_starred) const { item_save(model_hash, DbItemOptionType::ModelStarred, is_starred); }

    std::string wf_labels_load(hid hash) const { auto val = item_load(hash, DbItemOptionType::WfLabels); return (val.has_value() ? std::move(val.value().sval) : ""); }
    void wf_labels_save(hid hash, std::string&& labels) const { item_save(hash, DbItemOptionType::WfLabels, std::move(labels)); }

    std::string nodes_labels_load(hid hash) const { auto val = item_load(hash, DbItemOptionType::NodeLabels); return (val.has_value() ? std::move(val.value().sval) : ""); }
    void nodes_labels_save(hid hash, std::string&& labels) const { item_save(hash, DbItemOptionType::NodeLabels, std::move(labels)); }

    // stats
    int64_t stat_threshold_warn_load(MetricLimit type) const noexcept
    {
        const size_t index = cast::to_szt(type);
        assert(index < m_stat_limits_warn.size());
        if (index >= m_stat_limits_warn.size())
            return 0;
        return *std::get_if<int64_t>(&m_stat_limits_warn[index].value);
    }
    int64_t stat_threshold_crit_load(MetricLimit type) const noexcept
    {
        const size_t index = cast::to_szt(type);
        assert(index < m_stat_limits_warn.size());
        if (index >= m_stat_limits_warn.size())
            return 0;
        return *std::get_if<int64_t>(&m_stat_limits_crit[index].value);
    }
    void stat_threshold_warn_save(MetricLimit type, int64_t threshold) { save_option(m_stat_limits_warn[cast::to_szt(type)], threshold); }
    void stat_threshold_crit_save(MetricLimit type, int64_t threshold) { save_option(m_stat_limits_crit[cast::to_szt(type)], threshold); }

    std::vector<std::pair<std::string, bool>>& stat_disks_enabled_load() noexcept { return m_stat_enabled_disks_parsed; }
    void stat_disks_enabled_save();

    bool stat_as_progress_load() const noexcept { return *std::get_if<bool>(&m_stat_as_progress.value); }
    void stat_as_progress_save(bool enabled) { save_option(m_stat_as_progress, bool(enabled)); }
    
private:
    struct OptionValue final
    {
        const char* db_name = nullptr;
        std::variant<std::string, int64_t, std::filesystem::path, bool> value;
        bool guard_required = false;
    };

    void init_options(std::vector<OptionValue*>& values);
    void init_disks();

    std::string format_hotkeys_to_json(const std::vector<ui::KeyBinding>& hotkeys) const;

    void save_option(OptionValue& option, const std::string& new_str);
    void save_option(OptionValue& option, int64_t new_int);
    void save_option(OptionValue& option, bool new_bool);
    void save_option(OptionValue& option, const std::filesystem::path& new_path);

    std::optional<DbValue> item_load(hid hash, DbItemOptionType type) const;
    void item_save(hid hash, DbItemOptionType type, int64_t value) const;
    void item_save(hid hash, DbItemOptionType type, std::string&& value) const;

private:
    std::string m_empty;
    std::mutex m_guard;

    // app
    OptionValue m_app_autoupdate; // int
    OptionValue m_app_vsync; // int
    OptionValue m_app_fps_limit; // int
    OptionValue m_app_fps_limit_val; // int
    OptionValue m_app_scale; // int

    // ui
    OptionValue m_ui_open_wnds; // int
    OptionValue m_ui_nodes_tree_view; // int
    std::vector<OptionValue> m_ui_colors; // int
    OptionValue m_ui_theme; // int. 0 - sys default, 1 - dark, 2 - light
    OptionValue m_ui_activity_bar_mode; // int
    OptionValue m_ui_activity_bar_big_icons; // int
    OptionValue m_ui_activity_bar_show_restart; // int
    OptionValue m_ui_hotkeys; // str

    // comfyui
    OptionValue m_comfyui_host; // str
    OptionValue m_comfyui_port; // int
    OptionValue m_comfyui_conn_time; // int
    OptionValue m_comfyui_start_server; // int
    OptionValue m_comfyui_python; // path
    OptionValue m_comfyui_python_args; // str
    OptionValue m_comfyui_kill_server; // int
    OptionValue m_comfyui_hide_console; // int
    OptionValue m_comfyui_capture_output; // int
    OptionValue m_comfyui_auto_hide_output; // int
    OptionValue m_comfyui_parse_address; // int
    OptionValue m_comfyui_main_py; // path
    OptionValue m_comfyui_comfyui_args; // str
    OptionValue m_comfyui_workdir; // path
    OptionValue m_comfyui_output; // path
    OptionValue m_comfyui_ping_interval; // int

    // wf
    OptionValue m_wf_widget_input_numbers; // int
    OptionValue m_wf_widget_input_bools; // int
    OptionValue m_wf_active; // str
    OptionValue m_wf_show_grid; // int
    OptionValue m_wf_snap_nodes; // int
    OptionValue m_wf_link_thickness; // int
    OptionValue m_wf_imported; // str

    // queue
    OptionValue m_queue_favs; // str
    OptionValue m_queue_history_num; // int

    // logs
    OptionValue m_log_db_backup_num; // int
    OptionValue m_log_app_log_num; // int
    OptionValue m_log_comfyui_log_num; // int
    OptionValue m_log_comfyui_log_in_ram; // int

    // items
    std::map<hid, bool> m_item_preload_pin_adv;
    std::map<hid, bool> m_item_preload_model_starred;

    // stats
    std::vector<OptionValue> m_stat_limits_warn;
    std::vector<OptionValue> m_stat_limits_crit;
    OptionValue m_stat_enabled_disks;
    std::vector<std::pair<std::string, bool>> m_stat_enabled_disks_parsed;
    OptionValue m_stat_as_progress;
};

} // namespace mngr
