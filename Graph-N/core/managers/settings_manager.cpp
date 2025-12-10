
#include "core/managers/settings_manager.h"
#include "core/managers/db_manager.h"

namespace mngr
{

inline constexpr auto gopt_comfyui_host					= "comfyui_host";
inline constexpr auto gopt_comfyui_port					= "comfyui_port";
inline constexpr auto gopt_comfyui_conn_time			= "comfyui_conn_time";
inline constexpr auto gopt_comfyui_read_time			= "comfyui_read_time";

// defaults
inline constexpr auto gopt_def_comfyui_host				= "localhost";
inline constexpr uint16_t gopt_def_comfyui_port			= 8188;
inline constexpr size_t gopt_def_comfyui_conn_time		= 5;
inline constexpr size_t gopt_def_comfyui_read_time		= 10;

uint32_t SettingsManager::get_window_title_color(DlgObject /*dlg*/)
{
	return 0;
}

const std::string& SettingsManager::comfyui_get_host()
{
	if (m_comfyui_host.empty())
	{
		const auto val = g::pDbManager->get_option_str(gopt_comfyui_host);
		if (!val.has_value())
		{
			comfyui_set_host(gopt_def_comfyui_host);
			return comfyui_get_host();
		}
		m_comfyui_host = val.value();
	}
	return m_comfyui_host;
}

void SettingsManager::comfyui_set_host(const std::string& host)
{
	m_comfyui_host = host;
	g::pDbManager->set_option(gopt_comfyui_host, host);
}

uint16_t SettingsManager::comfyui_get_port()
{
	if (0 == m_comfyui_port)
	{
		const auto val = g::pDbManager->get_option_int(gopt_comfyui_port);
		if (!val.has_value())
		{
			comfyui_set_port(gopt_def_comfyui_port);
			return comfyui_get_port();
		}
		m_comfyui_port = cast::to_u16(val.value());
	}
	return m_comfyui_port;
}

void SettingsManager::comfyui_set_port(uint16_t port)
{
	m_comfyui_port = port;
	g::pDbManager->set_option(gopt_comfyui_port, m_comfyui_port);
}

size_t SettingsManager::comfyui_get_connect_timeout()
{
	if (0 == m_comfyui_conn_time)
	{
		const auto val = g::pDbManager->get_option_int(gopt_comfyui_conn_time);
		if (!val.has_value())
		{
			comfyui_set_connect_timeout(gopt_def_comfyui_conn_time);
			return comfyui_get_connect_timeout();
		}
		m_comfyui_conn_time = cast::to_szt(val.value());
	}
	return m_comfyui_conn_time;
}

void SettingsManager::comfyui_set_connect_timeout(size_t timeout)
{
	m_comfyui_conn_time = timeout;
	g::pDbManager->set_option(gopt_comfyui_conn_time, m_comfyui_conn_time);
}

size_t SettingsManager::comfyui_get_read_timeout()
{
	if (0 == m_comfyui_read_time)
	{
		const auto val = g::pDbManager->get_option_int(gopt_comfyui_read_time);
		if (!val.has_value())
		{
			comfyui_set_read_timeout(gopt_def_comfyui_read_time);
			return comfyui_get_read_timeout();
		}
		m_comfyui_read_time = cast::to_szt(val.value());
	}
	return m_comfyui_read_time;
}

void SettingsManager::comfyui_set_read_timeout(size_t timeout)
{
	m_comfyui_read_time = timeout;
	g::pDbManager->set_option(gopt_comfyui_read_time, m_comfyui_read_time);
}

} // namespace mngr