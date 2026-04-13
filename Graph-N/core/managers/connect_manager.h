
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
    void stop_all_threads() override;

    void reconnect();

    using RequestCallback = std::function<void(bool, std::string&&)>;
    void send_http_request(const char* request, bool post, RequestCallback callback, const std::string&& post_payload = "");
    void check_request_status();

    ConnectState get_status(size_t* inbox = nullptr, int64_t* latency = nullptr);

private:
    struct RequestData
    {
        bool reconnect = false;
        std::string request;
        bool post = true;
        std::string host;
        int32_t port = 0;
        int64_t conn_timeout = 0;
        RequestCallback callback = nullptr;
        std::string post_payload;

        bool success = false;
        std::string data;
    };

    void send_http_request_internal(std::shared_ptr<RequestData> req);
    std::shared_ptr<RequestData> request_thread(std::shared_ptr<RequestData> request);

    void drop_ws_connection();

private:
    std::mutex m_cli_mutex;
    std::unique_ptr<httplib::Client> m_http_client;

    std::string m_prev_host;
    int32_t m_prev_port = 0;

    std::mutex m_req_mutex;
    std::queue<std::shared_ptr<RequestData>> m_requests;
    std::thread m_req_thread;
    std::future<std::shared_ptr<RequestData>> m_req_future;

    std::shared_ptr<WebSocket> m_ws;
    ConnectState m_prev_state = ConnectState::Disconnected;
};

} // namespace mngr
