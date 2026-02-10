
#pragma once
#include "core/managers/manager.h"

namespace mngr
{

class SettingsManager final : public Manager
{
public:
    SettingsManager() = default;
    ~SettingsManager() override;

    bool init() override;

    // general
    inline bool general_vsync_load() const { return *std::get_if<bool>(&m_general_vsync.value); }
    inline void general_vsync_save(bool enabled) { save_option(m_general_vsync, bool(enabled)); }

    // ui
    inline ImU32 color_load(AppColor color) const { return cast::to_u32(*std::get_if<int64_t>(&m_ui_colors[cast::to_szt(color)].value)); }

    inline int64_t ui_open_wnds_load() const { return *std::get_if<int64_t>(&m_ui_open_wnds.value); }
    void ui_open_wnds_save(int64_t windows);

    inline bool ui_nodes_pane_tree_view_load() const { return *std::get_if<bool>(&m_ui_nodes_tree_view.value); }
    inline void ui_nodes_pane_tree_view_save(bool tree_view) { save_option(m_ui_nodes_tree_view, bool(tree_view)); }

    // comfyui
    inline const std::string& comfyui_host_load() const { return *std::get_if<std::string>(&m_comfyui_host.value); }
    inline void comfyui_host_save(const std::string& host) { save_option(m_comfyui_host, host); }

    inline int64_t comfyui_port_load() const { return *std::get_if<int64_t>(&m_comfyui_port.value); }
    inline void comfyui_port_save(int64_t port) { save_option(m_comfyui_port, int64_t(port)); }

    inline int64_t comfyui_connect_timeout_load() const { return *std::get_if<int64_t>(&m_comfyui_conn_time.value); }
    inline void comfyui_connect_timeout_save(int64_t timeout) { save_option(m_comfyui_conn_time, int64_t(timeout)); }

    inline int64_t comfyui_read_timeout_load() const { return *std::get_if<int64_t>(&m_comfyui_read_time.value); }
    inline void comfyui_read_timeout_save(int64_t timeout) { save_option(m_comfyui_read_time, int64_t(timeout)); }

    inline bool comfyui_start_server_load() const { return *std::get_if<bool>(&m_comfyui_start_server.value); }
    inline void comfyui_start_server_save(bool start) { save_option(m_comfyui_start_server, bool(start)); }

    inline const std::string& comfyui_python_load() const { return *std::get_if<std::string>(&m_comfyui_python.value); }
    inline void comfyui_python_save(const std::string& python) { save_option(m_comfyui_python, python); }

    inline bool comfyui_kill_server_load() const { return *std::get_if<bool>(&m_comfyui_kill_server.value); }
    inline void comfyui_kill_server_save(bool kill) { save_option(m_comfyui_kill_server, bool(kill)); }

    inline bool comfyui_hide_console_load() const { return *std::get_if<bool>(&m_comfyui_hide_console.value); }
    inline void comfyui_hide_console_save(bool hide) { save_option(m_comfyui_hide_console, bool(hide)); }

    inline bool comfyui_capture_output_load() const { return *std::get_if<bool>(&m_comfyui_capture_output.value); }
    inline void comfyui_capture_output_save(bool capture) { save_option(m_comfyui_capture_output, bool(capture)); }

    inline bool comfyui_autohide_output_load() const { return *std::get_if<bool>(&m_comfyui_auto_hide_output.value); }
    inline void comfyui_autohide_output_save(bool hide) { save_option(m_comfyui_auto_hide_output, bool(hide)); }

    inline bool comfyui_parse_address_load() const { return *std::get_if<bool>(&m_comfyui_parse_address.value); }
    inline void comfyui_parse_address_save(bool parse) { save_option(m_comfyui_parse_address, bool(parse)); }

    inline const std::filesystem::path& comfyui_main_py_load() const { return *std::get_if<std::filesystem::path>(&m_comfyui_main_py.value); }
    inline void comfyui_main_py_save(const std::filesystem::path& main_py) { save_option(m_comfyui_main_py, main_py); }

    inline const std::string& comfyui_command_load() const { return *std::get_if<std::string>(&m_comfyui_command.value); }
    inline void comfyui_command_save(const std::string& cmd) { save_option(m_comfyui_command, cmd); }

    // wf
    inline WidgetInput wf_widget_input_numbers_load() const { return cast::to_enum<WidgetInput>(*std::get_if<int64_t>(&m_wf_widget_input_numbers.value)); }
    inline void wf_widget_input_numbers_save(WidgetInput numbers) { save_option(m_wf_widget_input_numbers, cast::to_i64<>(numbers)); }

    inline WidgetInput wf_widget_input_bools_load() const { return cast::to_enum<WidgetInput>(*std::get_if<int64_t>(&m_wf_widget_input_bools.value)); }
    inline void wf_widget_input_bools_save(WidgetInput bools) { save_option(m_wf_widget_input_bools, cast::to_i64<>(bools)); }

    inline const std::string& wf_active_load() const { return *std::get_if<std::string>(&m_wf_active.value); }
    inline void wf_active_save(const std::string& active) { save_option(m_wf_active, active); }

    inline bool wf_show_grid_load() const { return *std::get_if<bool>(&m_wf_show_grid.value); }
    inline void wf_show_grid_save(bool show) { save_option(m_wf_show_grid, bool(show)); }

    inline bool wf_snap_nodes_load() const { return *std::get_if<bool>(&m_wf_snap_nodes.value); }
    inline void wf_snap_nodes_save(bool snap) { save_option(m_wf_snap_nodes, bool(snap)); }

    // items
    void pin_advanced_preload();
    void pin_advanced_release() { m_item_preload_pin_adv.clear(); }
    inline std::optional<bool> pin_advanced_load(hid pin_id) const { auto it = m_item_preload_pin_adv.find(pin_id); return (it == m_item_preload_pin_adv.end() ? std::nullopt : std::optional<bool>(it->second)); }
    void pin_advanced_save(hid pin_id, bool is_adv) const { item_save(pin_id, DbItemOptionType::PinAdvanced, is_adv); }

    void model_starred_preload();
    void model_starred_release() { m_item_preload_model_starred.clear(); }
    inline std::optional<bool> model_starred_load(hid model_id) const { auto it = m_item_preload_model_starred.find(model_id); return (it == m_item_preload_model_starred.end() ? std::nullopt : std::optional<bool>(it->second)); }
    void model_starred_save(hid model_id, bool is_starred) const { item_save(model_id, DbItemOptionType::ModelStarred, is_starred); }

    inline std::string wf_labels_load(hid id) const { auto val = item_load(id, DbItemOptionType::WfLabels); return (val.has_value() ? std::move(val.value().sval) : ""); }
    void wf_labels_save(hid id, std::string&& labels) const { item_save(id, DbItemOptionType::ModelStarred, std::move(labels)); }

    std::string nodes_labels_load(hid id) const { auto val = item_load(id, DbItemOptionType::NodeLabels); return (val.has_value() ? std::move(val.value().sval) : ""); }
    void nodes_labels_save(hid id, std::string&& labels) const { item_save(id, DbItemOptionType::NodeLabels, std::move(labels)); }

    // stats
    inline int64_t stat_threshold_warn_load(MetricLimit type) const { return *std::get_if<int64_t>(&m_stat_limits_warn[cast::to_szt(type)].value); }
    inline int64_t stat_threshold_crit_load(MetricLimit type) const { return *std::get_if<int64_t>(&m_stat_limits_crit[cast::to_szt(type)].value); }
    void stat_threshold_warn_save(MetricLimit type, int64_t threshold) { save_option(m_stat_limits_warn[cast::to_szt(type)], threshold); }
    void stat_threshold_crit_save(MetricLimit type, int64_t threshold) { save_option(m_stat_limits_crit[cast::to_szt(type)], threshold); }

    inline std::vector<std::pair<std::string, bool>>& stat_disks_enabled_load() { return m_stat_enabled_disks_parsed; }
    void stat_disks_enabled_save();

    inline bool stat_as_progress_load() const { return *std::get_if<bool>(&m_stat_as_progress.value); }
    inline void stat_as_progress_save(bool enabled) { save_option(m_stat_as_progress, bool(enabled)); }
    
private:

    struct OptionValue final
    {
        const char* db_name = nullptr;
        std::variant<std::string, int64_t, std::filesystem::path, bool> value;
    };

    void init_options(std::vector<OptionValue*>& values);
    void init_disks();

    void save_option(OptionValue& option, const std::string& new_str);
    void save_option(OptionValue& option, int64_t new_int);
    void save_option(OptionValue& option, bool new_bool);
    void save_option(OptionValue& option, const std::filesystem::path& new_path);

    std::optional<DbValue> item_load(hid id, DbItemOptionType type) const;
    void item_save(hid id, DbItemOptionType type, int64_t value) const;
    void item_save(hid id, DbItemOptionType type, std::string&& value) const;

private:
    std::string m_empty;

    // general
    OptionValue m_general_vsync; // int

    // ui
    OptionValue m_ui_open_wnds; // int
    OptionValue m_ui_nodes_tree_view; // int
    std::vector<OptionValue> m_ui_colors; // int

    // comfyui
    OptionValue m_comfyui_host; // str
    OptionValue m_comfyui_port; // int
    OptionValue m_comfyui_conn_time; // int
    OptionValue m_comfyui_read_time; // int
    OptionValue m_comfyui_start_server; // int
    OptionValue m_comfyui_python; // str
    OptionValue m_comfyui_kill_server; // int
    OptionValue m_comfyui_hide_console; // int
    OptionValue m_comfyui_capture_output; // int
    OptionValue m_comfyui_auto_hide_output; // int
    OptionValue m_comfyui_parse_address; // int
    OptionValue m_comfyui_main_py; // path
    OptionValue m_comfyui_command; // str

    // wf
    OptionValue m_wf_widget_input_numbers; // int
    OptionValue m_wf_widget_input_bools; // int
    OptionValue m_wf_active; // str
    OptionValue m_wf_show_grid; // int
    OptionValue m_wf_snap_nodes; // int

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
