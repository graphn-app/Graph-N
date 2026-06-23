
#pragma once
#include "core/managers/manager.h"

namespace mngr
{

class WebSocket;

class ConnectManager final : public Manager
{
    friend class ui::GraphN_Flow;

public:
    ConnectManager();
    ~ConnectManager() override;

    bool init() override;
    void stop_all_threads() override;

    ConnectState get_status(int64_t* latency = nullptr);

private:
    void reconnect();

    uint32_t send_http_request(const char* who_is_asking, const char* request, bool post, const std::string&& post_payload = "");

    bool pickup_http_response(uint32_t& req_id, int32_t& result, std::string& response);
    bool pickup_ws_message(bool& is_binary, std::string& message);

    struct RequestData
    {
        uint32_t req_id = 0;

        std::string request;
        bool post = true;
        std::string post_payload;

        int32_t status = 0;
        std::string data;
    };

    void http_worker();

    void drop_ws_connection();

private:
    std::atomic_uint32_t m_last_request_id = 0;

    bool m_reconnect = false;
    std::string m_host;
    int32_t m_port = 0;
    int64_t m_timeout = 0;
    std::string m_prev_host;
    int32_t m_prev_port = 0;
    int64_t m_prev_timeout = 0;

    std::mutex m_http_inbox_mutex;
    std::condition_variable m_inbox_cv;
    std::queue<std::shared_ptr<RequestData>> m_http_inbox;
    std::thread m_http_thread;

    std::mutex m_http_pickup_mutex;
    std::queue<std::shared_ptr<RequestData>> m_http_pickup;

    std::shared_ptr<WebSocket> m_ws;
    std::list<std::pair<bool, std::string>> m_ws_pickup;
};

} // namespace mngr
