
#pragma once
#include "core/managers/manager.h"

namespace mngr
{

class SettingsManager final : public Manager
{
public:
    SettingsManager() = default;
    ~SettingsManager() override = default;

    bool init() override;

    // app
    bool app_get_vsync_enabled();
    void app_set_vsync_enabled(bool enabled);

    // flow
    uint32_t flow_get_color(VisibleObject obj);

    uint64_t flow_get_open_wnds();
    void flow_set_open_wnds(uint64_t windows);

    bool flow_get_nodes_pane_tree_view();
    void flow_set_nodes_pane_tree_view(bool tree_view);

    // comfyui
    const std::string& comfyui_get_host();
    void comfyui_set_host(const std::string& host);

    uint16_t comfyui_get_port();
    void comfyui_set_port(uint16_t port);

    uint64_t comfyui_get_connect_timeout();
    void comfyui_set_connect_timeout(uint64_t timeout);

    uint64_t comfyui_get_read_timeout();
    void comfyui_set_read_timeout(uint64_t timeout);

    bool comfyui_get_start_server();
    void comfyui_set_start_server(bool start);

    bool comfyui_get_kill_server();
    void comfyui_set_kill_server(bool kill);

    bool comfyui_get_hide_console();
    void comfyui_set_hide_console(bool hide);

    bool comfyui_get_capture_output();
    void comfyui_set_capture_output(bool capture);

    bool comfyui_get_parse_address();
    void comfyui_set_parse_address(bool parse);

    const std::filesystem::path& comfyui_get_main_py();
    void comfyui_set_main_py(const std::filesystem::path& main);

    const std::string& comfyui_get_command();
    void comfyui_set_command(const std::string& cmd);

    // wf
    WidgetInput wf_get_widget_input_numbers();
    void wf_set_widget_input_numbers(WidgetInput numbers);

    WidgetInput wf_get_widget_input_bools();
    void wf_set_widget_input_bools(WidgetInput bools);

    // nodes
    void nd_preload_node_options();
    void nd_release_node_options();

    std::optional<bool> nd_get_pin_advanced(hid pin_id);
    void nd_set_pin_advanced(hid pin_id, bool advanced);

    // stats
    void stat_get_lram_threshold(int64_t& warn, int64_t& crit);
    void stat_set_lram_threshold(int64_t warn, int64_t crit);

    void stat_get_sram_threshold(int64_t& warn, int64_t& crit);
    void stat_set_sram_threshold(int64_t warn, int64_t crit);

    void stat_get_vram_threshold(int64_t& warn, int64_t& crit);
    void stat_set_vram_threshold(int64_t warn, int64_t crit);

    void stat_get_pytorch_threshold(int64_t& warn, int64_t& crit);
    void stat_set_pytorch_threshold(int64_t warn, int64_t crit);

    void stat_get_disk_threshold(int64_t& warn, int64_t& crit);
    void stat_set_disk_threshold(int64_t warn, int64_t crit);

    std::vector<std::pair<std::string, bool>> stat_get_disks_enabled();
    void stat_set_disks_enabled(const std::vector<std::pair<std::string, bool>>& disks);

    void stat_get_ping_threshold(int64_t& warn, int64_t& crit);
    void stat_set_ping_threshold(int64_t warn, int64_t crit);

    void stat_get_fps_threshold(int64_t& warn, int64_t& crit);
    void stat_set_fps_threshold(int64_t warn, int64_t crit);

private:
    int64_t get_opt_int(int64_t& out_option, const char* db_name, const int64_t def_value, const int64_t undef_example = -1);
    std::string& get_opt_str(std::string& out_option, const char* db_name, const char* def_value);

    void set_opt_int(int64_t& opt_param, const int64_t opt_value, const char* db_name);
    void set_opt_str(std::string& opt_param, const std::string& opt_value, const char* db_name);

private:
    // flow
    int64_t m_flow_open_wnds = -1;
    int64_t m_flow_colors[cast::to_u64(VisibleObject::_count)];
    int64_t m_flow_nodes_pane_view = -1;

    // comfyui
    std::string m_comfyui_host;
    int64_t m_comfyui_port = -1;
    int64_t m_comfyui_conn_time = -1;
    int64_t m_comfyui_read_time = -1;
    int64_t m_comfyui_start_server = -1;
    int64_t m_comfyui_kill_server = -1;
    int64_t m_comfyui_parse_address = -1;
    int64_t m_comfyui_hide_console = -1;
    int64_t m_comfyui_capture_output = -1;
    std::filesystem::path m_comfy_main_py;
    std::string m_comfy_command;

    // wf
    int64_t m_wf_widget_input_numbers = -1;
    int64_t m_wf_widget_input_bools = -1;

    // nodes
    std::unordered_map<hid, bool> m_hashed_pin_advanced;
};

} // namespace mngr
