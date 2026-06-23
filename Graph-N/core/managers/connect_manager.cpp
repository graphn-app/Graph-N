
#include "core/managers/connect_manager.h"
#include "core/managers/settings_manager.h"
#include "core/managers/web_socket.h"

namespace mngr
{

ConnectManager::ConnectManager()
{
    LogF;
}

ConnectManager::~ConnectManager()
{
    LogF;
    stop_all_threads();
}

bool ConnectManager::init()
{
    LogF;
    if (!Manager::init())
        return false;

    m_http_thread = std::thread(&ConnectManager::http_worker, this);
    return true;
}

void ConnectManager::stop_all_threads()
{
    LogF_In;

    m_stop_all_threads = true;
    // handle g::app_global_stop_all_threads
    {
        std::unique_lock<std::mutex> lock(m_http_inbox_mutex);  // cleanup queue coz its a app quit
        while (!m_http_inbox.empty())
            m_http_inbox.pop();
    }
    m_inbox_cv.notify_one();

    if (m_http_thread.joinable())
        m_http_thread.join();

    drop_ws_connection();

    LogF_Out;
}

void ConnectManager::reconnect()
{
    drop_ws_connection();

    auto opt = g::pOptsManager;
    m_ws = WebSocket::create_and_connect(opt->comfyui_host_load(),  opt->comfyui_port_load(), opt->comfyui_connect_timeout_load(), opt->comfyui_ping_interval_load());
}

void ConnectManager::drop_ws_connection()
{
    if (m_ws != nullptr)
        m_ws->stop_and_forget();
    m_ws.reset();
}

ConnectState ConnectManager::get_status(int64_t* latency /*= nullptr*/)
{
    if (latency)
        *latency = -1;

    if (m_ws != nullptr)
    {
        const auto state = m_ws->get_state();
        if (ConnectState::Connected == state)
        {
            if (latency)
                *latency = m_ws->get_latency();
        }
        return state;
    }

    return ConnectState::Disconnected;
}

uint32_t ConnectManager::send_http_request(const char* who_is_asking, const char* request, bool post, const std::string&& post_payload /*= ""*/)
{
    auto req = std::make_shared<RequestData>();
    req->request = request;
    req->post = post;
    req->post_payload = std::move(post_payload);
    req->req_id = ++m_last_request_id;

    Log("HTTP-Req#" << req->req_id << ". Registered: " << (post ? "POST" : "GET") << ". Who: " << who_is_asking << ". Url: " << req->request);

    {
        const auto host = g::pOptsManager->comfyui_host_load();
        const auto port = cast::to_i32(g::pOptsManager->comfyui_port_load());
        const auto tm_out = g::pOptsManager->comfyui_connect_timeout_load();

        std::unique_lock<std::mutex> lock(m_http_inbox_mutex);
        m_http_inbox.push(req);
        if (host != m_prev_host || port != m_prev_port || tm_out != m_prev_timeout)
        {
            m_reconnect = true;
            m_host = host;
            m_port = port;
            m_timeout = tm_out;
        }
    }
    m_inbox_cv.notify_one();

    return req->req_id;
}

void ConnectManager::http_worker()
{
    LogFS_In;

    std::unique_ptr<httplib::Client> client;

    while (get_status() != ConnectState::Connected)
    {
        if (m_stop_all_threads)
            return;
        std::this_thread::sleep_for(100ms);
    }

    const auto connect = [this, &client]()
    {
        client = std::make_unique<httplib::Client>(m_host, m_port);
        client->set_connection_timeout(m_timeout);
        client->set_read_timeout(m_timeout);
        client->set_keep_alive(true);
    };

    {
        std::unique_lock<std::mutex> lock(m_http_inbox_mutex);
        connect();
    }

    bool force_reconnect = false;

    while (!m_stop_all_threads)
    {
        std::shared_ptr<RequestData> req;

        {
            std::unique_lock<std::mutex> lock(m_http_inbox_mutex);
            m_inbox_cv.wait(lock, [this]() {
                return !m_http_inbox.empty() || m_stop_all_threads;
            });

            if (m_stop_all_threads && m_http_inbox.empty())
                break;

            if (m_reconnect || force_reconnect)
            {
                force_reconnect = false;
                connect();
            }

            req = m_http_inbox.front();
            m_http_inbox.pop();
        }

        LogS("HTTP-Req#" << req->req_id << ". Started");

        httplib::Result res;
        if (req->post)
            res = client->Post(req->request, req->post_payload, "application/json");
        else
            res = client->Get(req->request);

        if (res)
        {
            req->status = res->status;
            req->data = std::move(res->body);

            LogS("HTTP-Req#" << req->req_id << ". Finished. Result: " << res->status << ". Body: " << req->data.size() << " bytes");

            std::unique_lock<std::mutex> lock(m_http_pickup_mutex);
            m_http_pickup.push(req);
        }
        else
            force_reconnect = true;
    }

    LogFS_Out;
}

bool ConnectManager::pickup_http_response(uint32_t& req_id, int32_t& result, std::string& response)
{
    std::unique_lock<std::mutex> lock(m_http_pickup_mutex);
    if (m_http_pickup.empty())
        return false;

    auto req = m_http_pickup.front();
    req_id = req->req_id;
    result = req->status;
    response = std::move(req->data);
    m_http_pickup.pop();
    return true;
}

bool ConnectManager::pickup_ws_message(bool& is_binary, std::string& message)
{
    if (m_ws != nullptr)
    {
        auto msgs = m_ws->get();
        for (auto& msg : msgs)
        {
            m_ws_pickup.push_back({ msg.binary, std::move(msg.message) });
        }
    }
    if (m_ws_pickup.empty())
        return false;

    auto& msg = m_ws_pickup.front();
    is_binary = msg.first;
    message = std::move(msg.second);
    m_ws_pickup.pop_front();
    return true;
}

} // namespace mngr
