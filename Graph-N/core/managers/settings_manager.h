
#pragma once
#include "core/managers/manager.h"

namespace mngr
{

class SettingsManager final : public Manager
{
public:
	SettingsManager() = default;
	~SettingsManager() override = default;

	uint32_t get_window_title_color(DlgObject dlg);

	// comfyui
	const std::string& comfyui_get_host();
	void comfyui_set_host(const std::string& host);

	uint16_t comfyui_get_port();
	void comfyui_set_port(uint16_t port);

	size_t comfyui_get_connect_timeout();
	void comfyui_set_connect_timeout(size_t timeout);

	size_t comfyui_get_read_timeout();
	void comfyui_set_read_timeout(size_t timeout);

private:
	// comfyui
	std::string m_comfyui_host;
	uint16_t m_comfyui_port;
	size_t m_comfyui_conn_time = 5;
	size_t m_comfyui_read_time = 5;
};

} // namespace mngr