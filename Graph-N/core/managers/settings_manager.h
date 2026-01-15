
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

    // app
    inline bool app_vsync_load() const { return *std::get_if<bool>(&m_app_vsync.value); }
    inline void app_vsync_save(bool enabled) { save_option(m_app_vsync, bool(enabled)); }

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

    inline bool comfyui_kill_server_load() const { return *std::get_if<bool>(&m_comfyui_kill_server.value); }
    inline void comfyui_kill_server_save(bool kill) { save_option(m_comfyui_kill_server, bool(kill)); }

    inline bool comfyui_hide_console_load() const { return *std::get_if<bool>(&m_comfyui_hide_console.value); }
    inline void comfyui_hide_console_save(bool hide) { save_option(m_comfyui_hide_console, bool(hide)); }

    inline bool comfyui_capture_output_load() const { return *std::get_if<bool>(&m_comfyui_capture_output.value); }
    inline void comfyui_capture_output_save(bool capture) { save_option(m_comfyui_capture_output, bool(capture)); }

    inline bool comfyui_get_parse_address_load() const { return *std::get_if<bool>(&m_comfyui_parse_address.value); }
    inline void comfyui_parse_address_save(bool parse) { save_option(m_comfyui_parse_address, bool(parse)); }

    inline const std::filesystem::path& comfyui_main_py_load() const { return *std::get_if<std::filesystem::path>(&m_comfy_main_py.value); }
    inline void comfyui_main_py_save(const std::filesystem::path& main) { save_option(m_comfy_main_py, main); }

    inline const std::string& comfyui_command_load() const { return *std::get_if<std::string>(&m_comfy_command.value); }
    inline void comfyui_command_save(const std::string& cmd) { save_option(m_comfy_command, cmd); }

    // wf
    inline WidgetInput wf_widget_input_numbers_load() const { return cast::to_enum<WidgetInput>(*std::get_if<int64_t>(&m_wf_widget_input_numbers.value)); }
    inline void wf_widget_input_numbers_save(WidgetInput numbers) { save_option(m_wf_widget_input_numbers, cast::to_i64<>(numbers)); }

    inline WidgetInput wf_widget_input_bools_load() const { return cast::to_enum<WidgetInput>(*std::get_if<int64_t>(&m_wf_widget_input_bools.value)); }
    inline void wf_widget_input_bools_save(WidgetInput bools) { save_option(m_wf_widget_input_bools, cast::to_i64<>(bools)); }

    inline const std::string& wf_active_load() const { return *std::get_if<std::string>(&m_wf_active.value); }
    inline void wf_active_save(const std::string& active) { save_option(m_wf_active, active); }


    // nodes
    void nd_node_options_preload();
    inline void nd_node_options_release() { m_hashed_pin_advanced.clear(); }
    inline std::optional<bool> nd_pin_advanced_load(hid pin_id) const
    {
        const auto it = m_hashed_pin_advanced.find(pin_id);
        if (it == m_hashed_pin_advanced.end())
            return std::nullopt;
        return it->second;
    }
    void nd_pin_advanced_save(hid pin_id, bool advanced) const;

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

private:
    // app
    OptionValue m_app_vsync; // int

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
    OptionValue m_comfyui_kill_server; // int
    OptionValue m_comfyui_hide_console; // int
    OptionValue m_comfyui_capture_output; // int
    OptionValue m_comfyui_parse_address; // int
    OptionValue m_comfy_main_py; // path
    OptionValue m_comfy_command; // str

    // wf
    OptionValue m_wf_widget_input_numbers; // int
    OptionValue m_wf_widget_input_bools; // int
    OptionValue m_wf_active; // str

    // nodes
    std::unordered_map<hid, bool> m_hashed_pin_advanced;

    // stats
    std::vector<OptionValue> m_stat_limits_warn;
    std::vector<OptionValue> m_stat_limits_crit;
    OptionValue m_stat_enabled_disks;
    std::vector<std::pair<std::string, bool>> m_stat_enabled_disks_parsed;
    OptionValue m_stat_as_progress;
};

} // namespace mngr
