
#include "core/managers/settings_manager.h"
#include "core/managers/db_manager.h"
#include "ui/graph-n_flow.h"

namespace mngr
{

//////////////////////////////////////////////////////////////////////
////////       DB  NAMES                 /////////////////////////////
//////////////////////////////////////////////////////////////////////

inline constexpr auto gopt_app_vsync                    = "app_vsync_enabled";

inline constexpr auto gopt_flow_open_windows            = "flow_open_windows";
inline constexpr auto gopt_flow_clr_prefix              = "flow_color_";
inline constexpr auto gopt_flow_nodes_pane_view         = "flow_nodes_pane_view";

inline constexpr auto gopt_comfyui_host                 = "comfyui_host";
inline constexpr auto gopt_comfyui_port                 = "comfyui_port";
inline constexpr auto gopt_comfyui_conn_time            = "comfyui_conn_time";
inline constexpr auto gopt_comfyui_read_time            = "comfyui_read_time";
inline constexpr auto gopt_comfyui_start_server         = "comfyui_start_server";
inline constexpr auto gopt_comfyui_parse_address        = "comfyui_parse_address";
inline constexpr auto gopt_comfyui_kill_server          = "comfyui_kill_server";
inline constexpr auto gopt_comfyui_hide_console         = "comfyui_hide_console";
inline constexpr auto gopt_comfyui_capture_output       = "comfyui_capture_output";
inline constexpr auto gopt_comfyui_main_py              = "comfyui_main_py";
inline constexpr auto gopt_comfyui_command              = "comfyui_command";

inline constexpr auto gopt_wf_widget_input_number       = "wf_widget_input_numbers";
inline constexpr auto gopt_wf_widget_input_bools        = "wf_widget_input_bools";

inline constexpr auto gopt_stat_lram_warn               = "stat_local_ram_warn";
inline constexpr auto gopt_stat_lram_crit               = "stat_local_ram_crit";
inline constexpr auto gopt_stat_sram_warn               = "stat_server_ram_warn";
inline constexpr auto gopt_stat_sram_crit               = "stat_server_ram_crit";
inline constexpr auto gopt_stat_vram_warn               = "stat_vram_warn";
inline constexpr auto gopt_stat_vram_crit               = "stat_vram_crit";
inline constexpr auto gopt_stat_pytorch_warn            = "stat_pytorch_warn";
inline constexpr auto gopt_stat_pytorch_crit            = "stat_pytorch_crit";
inline constexpr auto gopt_stat_disk_warn               = "stat_disk_warn";
inline constexpr auto gopt_stat_disk_crit               = "stat_disk_crit";
inline constexpr auto gopt_stat_disks_enabled           = "stat_disks_enabled";
inline constexpr auto gopt_stat_ping_warn               = "stat_ping_warn";
inline constexpr auto gopt_stat_ping_crit               = "stat_ping_crit";
inline constexpr auto gopt_stat_fps_warn                = "stat_fps_warn";
inline constexpr auto gopt_stat_fps_crit                = "stat_fps_crit";

//////////////////////////////////////////////////////////////////////
////////       DEFAULTS                  /////////////////////////////
//////////////////////////////////////////////////////////////////////

inline constexpr int64_t gopt_def_app_vsync                 = 1;

inline constexpr int64_t gopt_def_flow_clrs[cast::to_u64(VisibleObject::_count)] =
                                                            {
                                                                0,
                                                                IM_COL32(150, 0, 0, 255),			// DisconnectedMenuBar
                                                                IM_COL32(255, 255, 255, 255),		// DisconnectedText
                                                                IM_COL32(0, 0, 150, 255),			// QueueButton
                                                                IM_COL32(0, 0, 0, 255),				// QueueText
                                                                IM_COL32(0, 128, 128, 255),			// Options Title
                                                                IM_COL32(0, 0, 0, 255),				// Options bg
                                                                IM_COL32(24, 24, 27, 255),			// PanelNodes bg -> all panels bg, except specified
                                                                IM_COL32(220, 220, 220, 50),		// PanelNodesHoverGroup
                                                                IM_COL32(211, 112, 0, 80),			// PanelNodesHoverNode
                                                                IM_COL32(255, 165, 0, 250),			// PanelNodesSelectedNode
                                                                0,			                        // PanelNodeInspector
                                                                IM_COL32(220, 220, 220, 50),		// PanelNodeInspectorDescrFrame
                                                                0,		                            // PanelStats
                                                                IM_COL32(45, 50, 60, 255),	    	// WfNodeHeaderBg
                                                                IM_COL32(255, 176, 50, 255),		// WfNodeHeaderText
                                                                IM_COL32(30, 30, 30, 200),  		// WfNodeBodyBg
                                                                IM_COL32(56, 56, 56, 255),  		// WfNodeWidgetPin
                                                                IM_COL32(100, 100, 100, 255),		// WfNodePinText
                                                                IM_COL32(100, 100, 100, 255),		// WfNodeBorder
                                                            };
inline constexpr int64_t gopt_def_flow_open_wnds            = ui::ViewOptsComfyUi | ui::ViewPanelsResetAll;
inline constexpr int64_t gopt_def_flow_nodes_pane_view      = 1;

inline constexpr auto	 gopt_def_comfyui_host              = "127.0.0.1";
inline constexpr int64_t gopt_def_comfyui_port              = 8188;
inline constexpr int64_t gopt_def_comfyui_conn_time         = 1;
inline constexpr int64_t gopt_def_comfyui_read_time         = 5;
inline constexpr int64_t gopt_def_comfyui_start_server      = 0;
inline constexpr int64_t gopt_def_comfyui_parse_address     = 0;
inline constexpr int64_t gopt_def_comfyui_hide_console      = 0;
inline constexpr int64_t gopt_def_comfyui_capture_output    = 0;
inline constexpr int64_t gopt_def_comfyui_kill_server       = 0;
 
inline constexpr int64_t gopt_def_wf_widget_input_number    = cast::to_i64(WidgetInput::slider);
inline constexpr int64_t gopt_def_wf_widget_input_bools     = cast::to_i64(WidgetInput::select);

inline constexpr int64_t gopt_def_stat_lram_warn            = 2147483648;   // 2 gb
inline constexpr int64_t gopt_def_stat_lram_crit            = 524288000;    // 500 mb
inline constexpr int64_t gopt_def_stat_sram_warn            = 3221225472;   // 1 gb
inline constexpr int64_t gopt_def_stat_sram_crit            = 1073741824;   // 3 gb
inline constexpr int64_t gopt_def_stat_vram_warn            = 1073741824;   // 1 gb
inline constexpr int64_t gopt_def_stat_vram_crit            = 209715200;    // 200 mb
inline constexpr int64_t gopt_def_stat_pytorch_warn         = 524288000;    // 500 mb
inline constexpr int64_t gopt_def_stat_pytorch_crit         = 104857600;    // 200 mb
inline constexpr int64_t gopt_def_stat_disk_warn            = 10737418240;  // 10 gb
inline constexpr int64_t gopt_def_stat_disk_crit            = 2147483648;   // 2 gb
inline constexpr int64_t gopt_def_stat_ping_warn            = 150;
inline constexpr int64_t gopt_def_stat_ping_crit            = 400;
inline constexpr int64_t gopt_def_stat_fps_warn             = 30;
inline constexpr int64_t gopt_def_stat_fps_crit             = 15;

//////////////////////////////////////////////////////////////////////
////////       COMMON                    /////////////////////////////
//////////////////////////////////////////////////////////////////////

bool SettingsManager::init()
{
    for (auto& clr : m_flow_colors)
    {
        clr = -1;
    }

    return true;
}

int64_t SettingsManager::get_opt_int(int64_t& out_option, const char* db_name, const int64_t def_value, const int64_t undef_example /*= -1*/)
{
    if (undef_example == out_option)
    {
        const auto val = g::pDbManager->get_option(db_name);
        if (!val.has_value())
        {
            set_opt_int(out_option, def_value, db_name);
            return get_opt_int(out_option, db_name, def_value, undef_example);
        }
        out_option = val.value().ival;
    }
    return out_option;
}

std::string& SettingsManager::get_opt_str(std::string& out_option, const char* db_name, const char* def_value)
{
    if (out_option.empty())
    {
        const auto val = g::pDbManager->get_option(db_name);
        if (!val.has_value())
        {
            set_opt_str(out_option, def_value, db_name);
            return get_opt_str(out_option, db_name, def_value);
        }
        out_option = val.value().sval;
    }
    return out_option;
}

void SettingsManager::set_opt_int(int64_t& opt_param, const int64_t opt_value, const char* db_name)
{
    opt_param = opt_value;
    g::pDbManager->set_option(db_name, { opt_param });
}

void SettingsManager::set_opt_str(std::string& opt_param, const std::string& opt_value, const char* db_name)
{
    opt_param = opt_value;
    g::pDbManager->set_option(db_name, { opt_param });
}

template<typename ENUM>
ENUM get_opt_int_e(int64_t value, ENUM def)
{
    if (value < cast::to_i64(ENUM::_first) || value > cast::to_i64(ENUM::_last))
        return def;
    return cast::to_enum<ENUM>(value);
}

//////////////////////////////////////////////////////////////////////
////////       APP                       /////////////////////////////
//////////////////////////////////////////////////////////////////////

bool SettingsManager::app_get_vsync_enabled()
{
    int64_t vsync = -1;
    return (get_opt_int(vsync, gopt_app_vsync, gopt_def_app_vsync) != 0);
}

void SettingsManager::app_set_vsync_enabled(bool enabled)
{
    int64_t vsync = enabled ? 1 : 0;
    set_opt_int(vsync, vsync, gopt_app_vsync);
}

//////////////////////////////////////////////////////////////////////
////////       FLOW                      /////////////////////////////
//////////////////////////////////////////////////////////////////////

uint32_t SettingsManager::flow_get_color(VisibleObject obj)
{
    const std::string name = fmt::format(FMT_COMPILE("{}{}"), gopt_flow_clr_prefix, cast::to_u64(obj));
    int64_t& clr = m_flow_colors[cast::to_u64(obj)];
    int64_t def = gopt_def_flow_clrs[cast::to_u64(obj)];
    return cast::to_u32(get_opt_int(clr, name.c_str(), def));
}

uint64_t SettingsManager::flow_get_open_wnds()
{
    return get_opt_int(m_flow_open_wnds, gopt_flow_open_windows, gopt_def_flow_open_wnds) | ui::ViewQueueCtrl;
#ifdef _DEBUG
    //| ui::ViewPanelsResetAll
#endif
    ;
}

void SettingsManager::flow_set_open_wnds(uint64_t windows)
{
    windows &= ~(ui::ViewAllMenuBar | ui::ViewAllOptions | ui::ViewAllDebug);
    set_opt_int(m_flow_open_wnds, windows, gopt_flow_open_windows);
}

bool SettingsManager::flow_get_nodes_pane_tree_view()
{
    return get_opt_int(m_flow_nodes_pane_view, gopt_flow_nodes_pane_view, gopt_def_flow_nodes_pane_view) == 1;
}

void SettingsManager::flow_set_nodes_pane_tree_view(bool tree_view)
{
    set_opt_int(m_flow_nodes_pane_view, tree_view, gopt_flow_nodes_pane_view);
}

//////////////////////////////////////////////////////////////////////
////////       COMFYUI                   /////////////////////////////
//////////////////////////////////////////////////////////////////////

const std::string& SettingsManager::comfyui_get_host()
{
    return get_opt_str(m_comfyui_host, gopt_comfyui_host, gopt_def_comfyui_host);
}

void SettingsManager::comfyui_set_host(const std::string& host)
{
    set_opt_str(m_comfyui_host, host, gopt_comfyui_host);
}

uint16_t SettingsManager::comfyui_get_port()
{
    return cast::to_u16(get_opt_int(m_comfyui_port, gopt_comfyui_port, gopt_def_comfyui_port));
}

void SettingsManager::comfyui_set_port(uint16_t port)
{
    set_opt_int(m_comfyui_port, port, gopt_comfyui_port);
}

uint64_t SettingsManager::comfyui_get_connect_timeout()
{
    return cast::to_szt(get_opt_int(m_comfyui_conn_time, gopt_comfyui_conn_time, gopt_def_comfyui_conn_time));
}

void SettingsManager::comfyui_set_connect_timeout(uint64_t timeout)
{
    set_opt_int(m_comfyui_conn_time, timeout, gopt_comfyui_conn_time);
}

uint64_t SettingsManager::comfyui_get_read_timeout()
{
    return cast::to_szt(get_opt_int(m_comfyui_read_time, gopt_comfyui_read_time, gopt_def_comfyui_read_time));
}

void SettingsManager::comfyui_set_read_timeout(uint64_t timeout)
{
    set_opt_int(m_comfyui_read_time, timeout, gopt_comfyui_read_time);
}

bool SettingsManager::comfyui_get_start_server()
{
    return get_opt_int(m_comfyui_start_server, gopt_comfyui_start_server, gopt_def_comfyui_start_server) == 1;
}

void SettingsManager::comfyui_set_start_server(bool start)
{
    set_opt_int(m_comfyui_start_server, start ? 1 : 0, gopt_comfyui_start_server);
}

bool SettingsManager::comfyui_get_kill_server()
{
    return get_opt_int(m_comfyui_kill_server, gopt_comfyui_kill_server, gopt_def_comfyui_kill_server) == 1;
}

void SettingsManager::comfyui_set_kill_server(bool kill)
{
    set_opt_int(m_comfyui_kill_server, kill ? 1 : 0, gopt_comfyui_kill_server);
}

bool SettingsManager::comfyui_get_hide_console()
{
    return get_opt_int(m_comfyui_hide_console, gopt_comfyui_hide_console, gopt_def_comfyui_hide_console) == 1;
}

void SettingsManager::comfyui_set_hide_console(bool hide)
{
    set_opt_int(m_comfyui_hide_console, hide ? 1 : 0, gopt_comfyui_hide_console);
}

bool SettingsManager::comfyui_get_capture_output()
{
    return get_opt_int(m_comfyui_capture_output, gopt_comfyui_capture_output, gopt_def_comfyui_capture_output) == 1;
}

void SettingsManager::comfyui_set_capture_output(bool capture)
{
    set_opt_int(m_comfyui_capture_output, capture ? 1 : 0, gopt_comfyui_capture_output);
}

bool SettingsManager::comfyui_get_parse_address()
{
    return get_opt_int(m_comfyui_parse_address, gopt_comfyui_parse_address, gopt_def_comfyui_parse_address) == 1;
}

void SettingsManager::comfyui_set_parse_address(bool parse)
{
    set_opt_int(m_comfyui_parse_address, parse ? 1 : 0, gopt_comfyui_parse_address);
}

const std::filesystem::path& SettingsManager::comfyui_get_main_py()
{
    std::string main = m_comfy_main_py.u8string();
    get_opt_str(main, gopt_comfyui_main_py, "");
    m_comfy_main_py = std::filesystem::u8path(main);
    return m_comfy_main_py;
}

void SettingsManager::comfyui_set_main_py(const std::filesystem::path& main)
{
    std::string opt_saved = m_comfy_main_py.u8string();
    std::string opt_to_save = main.u8string();
    set_opt_str(opt_saved, opt_to_save, gopt_comfyui_main_py);
    m_comfy_main_py = main;
}

const std::string& SettingsManager::comfyui_get_command()
{
    return get_opt_str(m_comfy_command, gopt_comfyui_command, "");
}

void SettingsManager::comfyui_set_command(const std::string& cmd)
{
    set_opt_str(m_comfy_command, cmd, gopt_comfyui_command);
}

WidgetInput SettingsManager::wf_get_widget_input_numbers()
{
    return get_opt_int_e(get_opt_int(m_wf_widget_input_numbers, gopt_wf_widget_input_number, gopt_def_wf_widget_input_number), WidgetInput::slider);
}

void SettingsManager::wf_set_widget_input_numbers(WidgetInput numbers)
{
    set_opt_int(m_wf_widget_input_numbers, cast::to_i64(numbers), gopt_wf_widget_input_number);
}

WidgetInput SettingsManager::wf_get_widget_input_bools()
{
    return get_opt_int_e(get_opt_int(m_wf_widget_input_bools, gopt_wf_widget_input_bools, gopt_def_wf_widget_input_bools), WidgetInput::slider);
}

void SettingsManager::wf_set_widget_input_bools(WidgetInput bools)
{
    set_opt_int(m_wf_widget_input_bools, cast::to_i64(bools), gopt_wf_widget_input_bools);
}

//////////////////////////////////////////////////////////////////////
////////       Nodes                     /////////////////////////////
//////////////////////////////////////////////////////////////////////

void SettingsManager::nd_preload_node_options()
{
    nd_release_node_options();
    const auto res = g::pDbManager->get_all_node_options(mngr::NodeOption::advanced);
    for (auto& [id, value] : res)
    {
        m_hashed_pin_advanced[id] = value.ival == 1;
    }
}

void SettingsManager::nd_release_node_options()
{
    m_hashed_pin_advanced.clear();
}

std::optional<bool> SettingsManager::nd_get_pin_advanced(hid pin_id)
{
    auto it = m_hashed_pin_advanced.find(pin_id);
    if (it == m_hashed_pin_advanced.end())
        return std::nullopt;
    return it->second;
}

void SettingsManager::nd_set_pin_advanced(hid pin_id, bool advanced)
{
    g::pDbManager->set_node_option(pin_id, mngr::NodeOption::advanced, { advanced ? 1 : 0 });
}

//////////////////////////////////////////////////////////////////////
////////       Stats                     /////////////////////////////
//////////////////////////////////////////////////////////////////////

void SettingsManager::stat_get_lram_threshold(int64_t& warn, int64_t& crit)
{
    warn = crit = -1;
    get_opt_int(warn, gopt_stat_lram_warn, gopt_def_stat_lram_warn);
    get_opt_int(crit, gopt_stat_lram_crit, gopt_def_stat_lram_crit);
}

void SettingsManager::stat_set_lram_threshold(int64_t warn, int64_t crit)
{
    set_opt_int(warn, warn, gopt_stat_lram_warn);
    set_opt_int(crit, crit, gopt_stat_lram_crit);
}

void SettingsManager::stat_get_sram_threshold(int64_t& warn, int64_t& crit)
{
    warn = crit = -1;
    get_opt_int(warn, gopt_stat_sram_warn, gopt_def_stat_sram_warn);
    get_opt_int(crit, gopt_stat_sram_crit, gopt_def_stat_sram_crit);
}

void SettingsManager::stat_set_sram_threshold(int64_t warn, int64_t crit)
{
    set_opt_int(warn, warn, gopt_stat_sram_warn);
    set_opt_int(crit, crit, gopt_stat_sram_crit);
}

void SettingsManager::stat_get_vram_threshold(int64_t& warn, int64_t& crit)
{
    warn = crit = -1;
    get_opt_int(warn, gopt_stat_vram_warn, gopt_def_stat_vram_warn);
    get_opt_int(crit, gopt_stat_vram_crit, gopt_def_stat_vram_crit);
}

void SettingsManager::stat_set_vram_threshold(int64_t warn, int64_t crit)
{
    set_opt_int(warn, warn, gopt_stat_vram_warn);
    set_opt_int(crit, crit, gopt_stat_vram_crit);
}

void SettingsManager::stat_get_pytorch_threshold(int64_t& warn, int64_t& crit)
{
    warn = crit = -1;
    get_opt_int(warn, gopt_stat_pytorch_warn, gopt_def_stat_pytorch_warn);
    get_opt_int(crit, gopt_stat_pytorch_crit, gopt_def_stat_pytorch_crit);
}

void SettingsManager::stat_set_pytorch_threshold(int64_t warn, int64_t crit)
{
    set_opt_int(warn, warn, gopt_stat_pytorch_warn);
    set_opt_int(crit, crit, gopt_stat_pytorch_crit);
}

void SettingsManager::stat_get_disk_threshold(int64_t& warn, int64_t& crit)
{
    warn = crit = -1;
    get_opt_int(warn, gopt_stat_disk_warn, gopt_def_stat_disk_warn);
    get_opt_int(crit, gopt_stat_disk_crit, gopt_def_stat_disk_crit);
}

void SettingsManager::stat_set_disk_threshold(int64_t warn, int64_t crit)
{
    set_opt_int(warn, warn, gopt_stat_disk_warn);
    set_opt_int(crit, crit, gopt_stat_disk_crit);
}

std::vector<std::string> SettingsManager::stat_get_disks_enabled()
{
    std::string disks_str;
    get_opt_str(disks_str, gopt_stat_disks_enabled, "");
    return tools::str::split(disks_str, '|');
}

void SettingsManager::stat_set_disks_enabled(const std::vector<std::string>& disks)
{
    auto str = tools::str::merge(disks, '|');
    set_opt_str(str, str, gopt_stat_disks_enabled);
}

void SettingsManager::stat_get_ping_threshold(int64_t& warn, int64_t& crit)
{
    warn = crit = -1;
    get_opt_int(warn, gopt_stat_ping_warn, gopt_def_stat_ping_warn);
    get_opt_int(crit, gopt_stat_ping_crit, gopt_def_stat_ping_crit);
}

void SettingsManager::stat_set_ping_threshold(int64_t warn, int64_t crit)
{
    set_opt_int(warn, warn, gopt_stat_ping_warn);
    set_opt_int(crit, crit, gopt_stat_ping_crit);
}

void SettingsManager::stat_get_fps_threshold(int64_t& warn, int64_t& crit)
{
    warn = crit = -1;
    get_opt_int(warn, gopt_stat_fps_warn, gopt_def_stat_fps_warn);
    get_opt_int(crit, gopt_stat_fps_crit, gopt_def_stat_fps_crit);
}

void SettingsManager::stat_set_fps_threshold(int64_t warn, int64_t crit)
{
    set_opt_int(warn, warn, gopt_stat_fps_warn);
    set_opt_int(crit, crit, gopt_stat_fps_crit);
}

} // namespace mngr
