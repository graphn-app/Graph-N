
#include "core/managers/settings_manager.h"
#include "core/managers/db_manager.h"
#include "ui/graph-n_flow.h"

namespace mngr
{

//////////////////////////////////////////////////////////////////////
////////       DB  NAMES                 /////////////////////////////
//////////////////////////////////////////////////////////////////////

inline constexpr auto gopt_flow_open_windows            = "flow_open_windows";
inline constexpr auto gopt_flow_clr_prefix              = "flow_color_";
inline constexpr auto gopt_flow_nodes_pane_view         = "flow_nodes_pane_view";

inline constexpr auto gopt_comfyui_host                 = "comfyui_host";
inline constexpr auto gopt_comfyui_port                 = "comfyui_port";
inline constexpr auto gopt_comfyui_conn_time            = "comfyui_conn_time";
inline constexpr auto gopt_comfyui_read_time            = "comfyui_read_time";

//////////////////////////////////////////////////////////////////////
////////       DEFAULTS                  /////////////////////////////
//////////////////////////////////////////////////////////////////////

inline constexpr int64_t gopt_def_flow_clrs[cast::to_u64(VisibleObject::_count)] =
                                                            {
                                                                0,
                                                                IM_COL32(150, 0, 0, 255),			// DisconnectedMenuBar
                                                                IM_COL32(255, 255, 255, 255),		// DisconnectedText
                                                                IM_COL32(0, 0, 150, 255),			// QueueButton
                                                                IM_COL32(0, 0, 0, 255),				// QueueText
                                                                IM_COL32(0, 128, 128, 255),			// Options Title
                                                                IM_COL32(0, 0, 0, 255),				// Options bg
                                                                IM_COL32(24, 24, 27, 255),			// PanelNodes bg
                                                                IM_COL32(220, 220, 220, 50),		// PanelNodesHoverGroup
                                                                IM_COL32(211, 112, 0, 80),			// PanelNodesHoverNode
                                                                IM_COL32(255, 165, 0, 250),			// PanelNodesSelectedNode
                                                                IM_COL32(24, 24, 27, 255),			// PanelNodeInspector bg
                                                                IM_COL32(220, 220, 220, 50),		// PanelNodeInspectorDescrFrame
                                                            };
inline constexpr int64_t gopt_def_flow_open_wnds            = ui::ViewOptsComfyUi | ui::ViewPanelsResetAll;
inline constexpr int64_t gopt_def_flow_nodes_pane_view      = 1;

inline constexpr auto	 gopt_def_comfyui_host              = "127.0.0.1";
inline constexpr int64_t gopt_def_comfyui_port              = 8188;
inline constexpr int64_t gopt_def_comfyui_conn_time         = 5;
inline constexpr int64_t gopt_def_comfyui_read_time         = 10;

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

int64_t SettingsManager::get_opt_int(int64_t& opt, const char* name, const int64_t def, const int64_t undef /*= -1*/)
{
    if (undef == opt)
    {
        const auto val = g::pDbManager->get_option_int(name);
        if (!val.has_value())
        {
            set_opt_int(opt, def, name);
            return get_opt_int(opt, name, def, undef);
        }
        opt = val.value();
    }
    return opt;
}

const std::string& SettingsManager::get_opt_str(std::string& opt, const char* name, const char* def)
{
    if (opt.empty())
    {
        const auto val = g::pDbManager->get_option_str(name);
        if (!val.has_value())
        {
            set_opt_str(opt, def, name);
            return get_opt_str(opt, name, def);
        }
        opt = val.value();
    }
    return opt;
}

void SettingsManager::set_opt_int(int64_t& opt, const int64_t val, const char* name)
{
    opt = val;
    g::pDbManager->set_option(name, opt);
}

void SettingsManager::set_opt_str(std::string& opt, const std::string& val, const char* name)
{
    opt = val;
    g::pDbManager->set_option(name, opt);
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
    windows &= ~(ui::ViewAllMenuBar | ui::ViewAllOptions);
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

size_t SettingsManager::comfyui_get_connect_timeout()
{
    return cast::to_szt(get_opt_int(m_comfyui_conn_time, gopt_comfyui_conn_time, gopt_def_comfyui_conn_time));
}

void SettingsManager::comfyui_set_connect_timeout(size_t timeout)
{
    set_opt_int(m_comfyui_conn_time, timeout, gopt_comfyui_conn_time);
}

size_t SettingsManager::comfyui_get_read_timeout()
{
    return cast::to_szt(get_opt_int(m_comfyui_read_time, gopt_comfyui_read_time, gopt_def_comfyui_read_time));
}

void SettingsManager::comfyui_set_read_timeout(size_t timeout)
{
    set_opt_int(m_comfyui_read_time, timeout, gopt_comfyui_read_time);
}

//////////////////////////////////////////////////////////////////////
////////       .                         /////////////////////////////
//////////////////////////////////////////////////////////////////////

} // namespace mngr
