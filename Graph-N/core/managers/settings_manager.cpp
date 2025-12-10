
#include "core/managers/settings_manager.h"
#include "core/managers/db_manager.h"

namespace mngr
{

inline constexpr auto g_comfyui_host			= "comfyui_host";
inline constexpr auto g_comfyui_port			= "comfyui_port";

// defaults
inline constexpr auto g_def_comfyui_host		= "localhost";
inline constexpr uint16_t g_def_comfyui_port	= 8188;

uint32_t SettingsManager::get_window_title_color(DlgObject /*dlg*/)
{
	return 0;
}

const std::string& SettingsManager::comfyui_get_address()
{
	if (m_comfyui_address.empty())
	{
		const auto val = g::pDbManager->get_option_str(g_comfyui_host);
		if (!val.has_value())
		{
			comfyui_set_address(g_def_comfyui_host);
			return comfyui_get_address();
		}
		m_comfyui_address = val.value();
	}
	return m_comfyui_address;
}

void SettingsManager::comfyui_set_address(const std::string& address)
{
	m_comfyui_address = address;
	g::pDbManager->set_option(g_comfyui_host, m_comfyui_address);
}

uint16_t SettingsManager::comfyui_get_port()
{
	if (0 == m_comfyui_port)
	{
		const auto val = g::pDbManager->get_option_int(g_comfyui_port);
		if (!val.has_value())
		{
			comfyui_set_port(g_def_comfyui_port);
			return comfyui_get_port();
		}
		m_comfyui_port = cast::to_16(val.value());
	}
	return m_comfyui_port;
}

void SettingsManager::comfyui_set_port(uint16_t port)
{
	m_comfyui_port = port;
	g::pDbManager->set_option(g_comfyui_port, m_comfyui_port);
}

} // namespace mngr