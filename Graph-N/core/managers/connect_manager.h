
#pragma once
#include "core/managers/manager.h"

namespace mngr
{

class WebSocket;

class ConnectManager final : public Manager
{
public:
    ConnectManager();
    ~ConnectManager() override;

    bool init() override;
    void free() override;
    void stop_all_threads() override;

    ConnectState check_status(size_t* inbox = nullptr, int64_t* latency = nullptr);

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
        ConnectState state = ConnectState::Disconnected;
        std::atomic_bool stop_thread_flag = false;
        std::string data;
    };

    static std::shared_ptr<ConnectData> connect_thread(std::shared_ptr<ConnectData> data);

    void make_ws_connection();
    void drop_ws_connection();

private:
    std::shared_ptr<ConnectData> m_conn_data;
    std::atomic<ConnectState> m_state = ConnectState::Disconnected;
    std::thread m_thread;
    std::future<std::shared_ptr<ConnectData>> m_future;
    std::string m_nodes_raw;
    int32_t m_conn_th_id = 0;

    std::shared_ptr<WebSocket> m_ws;
};

} // namespace mngr
