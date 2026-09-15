#pragma once
#include "core/managers/manager.h"

namespace mngr
{

class WebSocket;

class ConnectManager final : public Manager
{
    friend class ui::GraphN_Flow;

public:
    ConnectManager() noexcept;
    ~ConnectManager() noexcept override;

    bool init() override;
    void stop_all_threads() override;

    ConnectState get_status(int64_t* latency = nullptr) noexcept;

private:
    void reconnect();

    uint32_t send_http_get_request(HttpRequestor requestor, const char* url, bool retry_on_timeout);
    uint32_t send_http_get_request(HttpRequestor requestor, const char* url, httplib::Params&& params, httplib::Headers&& headers, bool retry_on_timeout);
    uint32_t send_http_post_request(HttpRequestor requestor, const char* url, std::string&& payload, bool retry_on_timeout);
    uint32_t send_http_post_request(HttpRequestor requestor, const char* url, httplib::UploadFormDataItems&& payload_items, bool retry_on_timeout);

    bool pickup_http_response(uint32_t& req_id, int32_t& result, std::string& response);
    bool pickup_ws_message(bool& is_binary, std::string& message);

    struct RequestData final
    {
        enum class Type { None, GetByUrl, GetByUrlAndParams, PostRawPayload, PostItems };
        Type type = Type::None;

        uint32_t req_id = 0;
        HttpRequestor requestor;
        bool log_this_request = true;

        std::string url;

        httplib::Params params;
        httplib::Headers headers;
        std::string post_payload;
        httplib::UploadFormDataItems post_payload_items;

        bool is_object_info = false;
        bool is_binary_resp = false;

        int32_t status = 0;
        std::string data;

        bool retry_on_timeout = false;
        uint32_t retry_counter = 0;
    };

    uint32_t register_http_request(std::shared_ptr<RequestData> req);
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
    std::atomic_bool m_http_pickup_empty;

    std::shared_ptr<WebSocket> m_ws;
    std::list<std::pair<bool, std::string>> m_ws_pickup;
};

} // namespace mngr
