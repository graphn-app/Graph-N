
#pragma once
#include "core/managers/manager.h"

namespace mngr
{

enum class ConnectState
{
	disconnected,
	connecting,
	connected,
};

class ConnectManager final : public Manager
{
public:
	ConnectManager() = default;
	~ConnectManager() override = default;

	bool init() override;
	void free() override;
	void stop_all_threads() override;

	ConnectState check_status();
	std::string& get_nodes_raw() { return m_nodes_raw; }

	void connect();
	void stop_connect();	

private:
	std::pair<ConnectState, std::string> connect_thread();

private:
	std::atomic_bool m_stop_thread_flag = false;
	std::atomic<ConnectState> m_state = ConnectState::disconnected;

	std::thread m_thread;
	std::future<std::pair<ConnectState, std::string>> m_future;
	std::string m_nodes_raw;
};

} // namespace mngr