
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
	const std::string& comfyui_get_address();
	void comfyui_set_address(const std::string& address);

	uint16_t comfyui_get_port();
	void comfyui_set_port(uint16_t port);

private:
	// comfyui
	std::string m_comfyui_address;
	uint16_t m_comfyui_port;
};

} // namespace mngr