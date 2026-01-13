
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
    ~ConnectManager() override;

    bool init() override;
    void free() override;
    void stop_all_threads() override;

    ConnectState check_status();
    std::string& get_nodes_raw() { return m_nodes_raw; }

    void connect();
    void stop_connect();

private:
    struct ConnectData
    {
        std::string host;
        int32_t port = 0;
        int64_t conn_timeout = 0;
        int64_t read_timeout = 0;
        int32_t conn_th_id = 0;
        ConnectState state = ConnectState::disconnected;
        std::atomic_bool stop_thread_flag = false;
        std::string data;
    };

    static std::shared_ptr<ConnectData> connect_thread(std::shared_ptr<ConnectData> data);

private:
    std::shared_ptr<ConnectData> m_conn_data;
    std::atomic<ConnectState> m_state = ConnectState::disconnected;
    std::thread m_thread;
    std::future<std::shared_ptr<ConnectData>> m_future;
    std::string m_nodes_raw;
    int32_t m_conn_th_id = 0;
};

} // namespace mngr
