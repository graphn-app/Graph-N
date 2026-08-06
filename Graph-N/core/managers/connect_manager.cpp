#include "core/managers/connect_manager.h"
#include "core/managers/settings_manager.h"
#include "core/managers/web_socket.h"
#include "core/base/comfyui_web_paths.h"

namespace mngr
{

ConnectManager::ConnectManager() noexcept = default;

ConnectManager::~ConnectManager() noexcept
{
    try {
        stop_all_threads();
    } catch (...) {}
}

bool ConnectManager::init()
{
    if (!Manager::init())
        return false;

    m_http_thread = std::thread(&ConnectManager::http_worker, this);
    return true;
}

void ConnectManager::stop_all_threads()
{
    LogIn;

    {
        std::unique_lock<std::mutex> lock(m_http_inbox_mutex);  // cleanup queue coz its a app quit
        while (!m_http_inbox.empty())
            m_http_inbox.pop();
    }
    m_inbox_cv.notify_one();

    if (m_http_thread.joinable())
        m_http_thread.join();

    drop_ws_connection();

    LogOut;
}

void ConnectManager::reconnect()
{
    drop_ws_connection();

    const auto* opt = g::pOptsManager;
    m_ws = WebSocket::create_and_connect(opt->comfyui_host_load(),  opt->comfyui_port_load(), opt->comfyui_connect_timeout_load(), opt->comfyui_ping_interval_load());
}

void ConnectManager::drop_ws_connection()
{
    if (m_ws != nullptr)
        m_ws->stop_and_forget();
    m_ws.reset();
}

ConnectState ConnectManager::get_status(int64_t* latency /*= nullptr*/) noexcept
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

uint32_t ConnectManager::send_http_request(
    const char* who_is_asking, const char* request, bool post,
    std::string&& post_payload, httplib::UploadFormDataItems&& post_payload_items, bool retry_on_timeout)
{
    auto req = std::make_shared<RequestData>();
    req->request = request;
    req->who_is_asking = who_is_asking;
    req->is_object_info = req->request.find(comfyui_http::object_info) != std::string::npos;
    req->is_binary_resp = req->request.find(comfyui_http::view_input) != std::string::npos || req->request.find(comfyui_http::view) != std::string::npos;
    req->post = post;
    req->post_payload = std::move(post_payload);
    req->post_payload_items = std::move(post_payload_items);
    req->req_id = m_last_request_id.fetch_add(1, std::memory_order_relaxed) + 1;
    req->retry_on_timeout = retry_on_timeout;

    Log("HTTP-Req#" << req->req_id << ". Registered: " << (post ? "POST" : "GET") << ". Who: " << who_is_asking << ". Retry: " << retry_on_timeout << ". Url: " << req->request);

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
    LogIn;

    std::unique_ptr<httplib::Client> client;

    while (get_status() != ConnectState::Connected)
    {
        if (g::app_global_stop_all_threads.load(std::memory_order_relaxed))
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

    while (!g::app_global_stop_all_threads.load(std::memory_order_relaxed))
    {
        std::shared_ptr<RequestData> req;

        {
            std::unique_lock<std::mutex> lock(m_http_inbox_mutex);
            m_inbox_cv.wait(lock, [this]() noexcept {
                return !m_http_inbox.empty() || g::app_global_stop_all_threads.load(std::memory_order_relaxed);
            });

            if (g::app_global_stop_all_threads.load(std::memory_order_relaxed) && m_http_inbox.empty())
                break;

            if (m_reconnect || force_reconnect)
            {
                force_reconnect = false;
                m_reconnect = false;
                connect();
            }

            req = m_http_inbox.front();
            m_http_inbox.pop();
        }

        Log("HTTP-Req#" << req->req_id << ". Started");

        httplib::Result res;
        if (req->post)
        {
            if (!req->post_payload_items.empty())
                res = client->Post(req->request, req->post_payload_items);
            else
                res = client->Post(req->request, req->post_payload, "application/json");
        }
        else
            res = client->Get(req->request);

        if (res)
        {
            req->status = res->status;
            req->data = std::move(res->body);

            Log("HTTP-Req#" << req->req_id << ". Finished. Result: " << res->status << ". Body: " << req->data.size() << " bytes");
            if (req->is_object_info && glog::Logger::get_log_object_info_enabled())
            {
                const auto obj_info_path = fs::get_log_directory_path() / "object_info.json";
                tools::path::make_file(obj_info_path, true, req->data);
                Log("/object_info dumped into " << obj_info_path);
            }
            else if (!req->is_object_info && !req->is_binary_resp && glog::Logger::get_log_web_response_enabled())
                LogClear(req->data);
        }
        else
        {
            force_reconnect = true;

            if (res.error() == httplib::Error::Connection || res.error() == httplib::Error::Read)
                req->status = HTTP_RESP_TIMEOUT;
            else
                req->status = HTTP_RESP_FAIL;

            Log("HTTP-Req#" << req->req_id << ". Failed. Error: " << httplib::to_string(res.error()));
        }

        {
            std::unique_lock<std::mutex> lock(m_http_pickup_mutex);
            m_http_pickup.push(req);
        }
    }

    LogOut;
}

bool ConnectManager::pickup_http_response(uint32_t& req_id, int32_t& result, std::string& response)
{
    std::unique_lock<std::mutex> lock(m_http_pickup_mutex);
    if (m_http_pickup.empty())
        return false;

    auto req = m_http_pickup.front();

    if (HTTP_RESP_TIMEOUT == req->status)
    {
        Log("HTTP-Req#" << req->req_id << ". Timeout. Retry request: " << req->retry_on_timeout);
        if (req->retry_on_timeout)
        {
            if (++req->retry_counter > HTTP_RETRY_MAX_COUNT)
            {
                Log("HTTP-Req#" << req->req_id << ". Retry max count reached: " << req->retry_counter);
                req_id = req->req_id;
                result = req->status;
                response = std::move(req->data);
                m_http_pickup.pop();
                return true;
            }

            req->status = 0; 
            req->data.clear();

            {
                std::unique_lock<std::mutex> inbox_lock(m_http_inbox_mutex);

                m_reconnect = true;
                m_host = g::pOptsManager->comfyui_host_load();
                m_port = cast::to_i32(g::pOptsManager->comfyui_port_load());
                m_timeout = g::pOptsManager->comfyui_connect_timeout_load();
                m_http_inbox.push(req);
            }
            m_inbox_cv.notify_one();

            m_http_pickup.pop();
            return false;
        }
    }

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
