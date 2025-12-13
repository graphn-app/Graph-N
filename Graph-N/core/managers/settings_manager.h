
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

	// flow
	uint32_t flow_get_color(VisibleObject obj);
	
	uint32_t flow_get_open_wnds();
	void flow_set_open_wnds(uint32_t windows);

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
	__int64 get_opt_int(__int64& opt, const char* name, const __int64 def, const __int64 undef = -1);
	const std::string& get_opt_str(std::string& opt, const char* name, const char* def);

	void set_opt_int(__int64& opt, const __int64 val, const char* name);
	void set_opt_str(std::string& opt, const std::string& val, const char* name);

private:
	// flow
	__int64 m_flow_open_wnds = -1;
	__int64 m_flow_colors[cast::to_szt(VisibleObject::_count)];

	// comfyui
	std::string m_comfyui_host;
	__int64 m_comfyui_port = -1;
	__int64 m_comfyui_conn_time = -1;
	__int64 m_comfyui_read_time = -1;
};

} // namespace mngr