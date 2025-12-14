
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
	int64_t get_opt_int(int64_t& opt, const char* name, const int64_t def, const int64_t undef = -1);
	const std::string& get_opt_str(std::string& opt, const char* name, const char* def);

	void set_opt_int(int64_t& opt, const int64_t val, const char* name);
	void set_opt_str(std::string& opt, const std::string& val, const char* name);

private:
	// flow
	int64_t m_flow_open_wnds = -1;
	int64_t m_flow_colors[cast::to_szt(VisibleObject::_count)];

	// comfyui
	std::string m_comfyui_host;
	int64_t m_comfyui_port = -1;
	int64_t m_comfyui_conn_time = -1;
	int64_t m_comfyui_read_time = -1;
};

} // namespace mngr