#include "core/managers/connect_manager.h"
#include "core/managers/settings_manager.h"
#include "core/managers/web_socket.h"
#include "core/base/comfyui_web_paths.h"

namespace mngr
{

ConnectManager::ConnectManager() noexcept = default;
ConnectManager::~ConnectManager() noexcept = default;

bool ConnectManager::init()
{
    if (!Manager::init())
        return false;

    m_http_thread = std::thread(&ConnectManager::http_worker, this);
    return true;
}

void ConnectManager::stop_and_cleanup()
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
    drop_all_messages();

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

uint32_t ConnectManager::send_http_get_request(HttpRequestor requestor, const char* url, bool retry_on_timeout)
{
    auto req = std::make_shared<RequestData>();
    req->type = RequestData::Type::GetByUrl;
    req->url = url;
    req->requestor = requestor;
    req->is_object_info = req->url.find(comfyui_http::object_info) != std::string::npos;
    req->is_binary_resp = req->url.find(comfyui_http::view_input) != std::string::npos || req->url.find(comfyui_http::view) != std::string::npos;
    req->retry_on_timeout = retry_on_timeout;
    return register_http_request(req);
}

uint32_t ConnectManager::send_http_get_request(HttpRequestor requestor, const char* url, httplib::Params&& params, httplib::Headers&& headers, bool retry_on_timeout)
{
    auto req = std::make_shared<RequestData>();
    req->type = RequestData::Type::GetByUrlAndParams;
    req->url = url;
    req->params = std::move(params);
    req->headers = std::move(headers);
    req->requestor = requestor;
    req->retry_on_timeout = retry_on_timeout;
    return register_http_request(req);
}

uint32_t ConnectManager::send_http_post_request(HttpRequestor requestor, const char* url, std::string&& payload, bool retry_on_timeout)
{
    auto req = std::make_shared<RequestData>();
    req->type = RequestData::Type::PostRawPayload;
    req->url = url;
    req->post_payload = std::move(payload);
    req->requestor = requestor;
    req->retry_on_timeout = retry_on_timeout;
    return register_http_request(req);
}

uint32_t ConnectManager::send_http_post_request(HttpRequestor requestor, const char* url, httplib::UploadFormDataItems&& payload_items, bool retry_on_timeout)
{
    auto req = std::make_shared<RequestData>();
    req->type = RequestData::Type::PostItems;
    req->url = url;
    req->post_payload_items = std::move(payload_items);
    req->requestor = requestor;
    req->retry_on_timeout = retry_on_timeout;
    return register_http_request(req);
}

uint32_t ConnectManager::register_http_request(std::shared_ptr<RequestData> req)
{
    req->req_id = m_last_request_id.fetch_add(1, std::memory_order_relaxed) + 1;

    req->log_this_request = true;
    const char* who_is_asking = nullptr;
    switch (req->requestor)
    {
    case HttpRequestor::Queue_Output:
        who_is_asking = "Queue/Output";
        break;
    case HttpRequestor::Queue_History:
        who_is_asking = "Queue/History";
        break;
    case HttpRequestor::Queue_Interrupt:
        who_is_asking = "Queue/Interrupt";
        break;
    case HttpRequestor::Queue_Task:
        who_is_asking = "Queue/Task";
        break;
    case HttpRequestor::Sync_Init:
        who_is_asking = "Sync/Init";
        break;
    case HttpRequestor::Sync_Action:
        who_is_asking = "Sync/Action";
        break;
    case HttpRequestor::Models_All:
        who_is_asking = "Models/All";
        break;
    case HttpRequestor::Models_Model:
        who_is_asking = "Models/Model";
        req->log_this_request = false;
        break;
    case HttpRequestor::Flow_Import_projects:
        who_is_asking = "Flow/Import_projects";
        break;
    case HttpRequestor::Flow_Reparse_nodes:
        who_is_asking = "Flow/Reparse_nodes";
        break;
    case HttpRequestor::QPanel_Output:
        who_is_asking = "QPanel/Output";
        break;
    case HttpRequestor::QPanel_History:
        who_is_asking = "QPanel/History";
        break;
    case HttpRequestor::SystemStats_Stats:
        who_is_asking = "SystemStats/Stats";
        req->log_this_request = false;
        break;
    case HttpRequestor::Stats_Prompt:
        who_is_asking = "Stats/Prompt";
        req->log_this_request = false;
        break;
    case HttpRequestor::CustomWdgt_Image:
        who_is_asking = "CustomWdgt/Image";
        break;
    case HttpRequestor::CustomWdgt_Video:
        who_is_asking = "CustomWdgt/Video";
        break;
    case HttpRequestor::CustomWdgt_Audio:
        who_is_asking = "CustomWdgt/Audio";
        break;
    case HttpRequestor::UploadMedia:
        who_is_asking = "UploadMedia";
        break;
    case HttpRequestor::Restart:
        who_is_asking = "Reboot";
        break;
    default:
        LogError("Unknown requestor");
        who_is_asking = "Unknown_requestor";
    }

    if (glog::Logger::get_log_web_response_enabled())   // user requested all web activity
        req->log_this_request = true;

    const char* type = (RequestData::Type::GetByUrl == req->type || RequestData::Type::GetByUrlAndParams == req->type ? "GET" : "POST");
    if (req->log_this_request)
        Log("HTTP-Req#" << req->req_id << ". Registered: " << type << ". Who: " << who_is_asking << ". Retry: " << req->retry_on_timeout << ". Url: " << req->url);

    {
        const auto host = g::pOptsManager->comfyui_host_load_safe();
        const auto port = cast::to_i32(g::pOptsManager->comfyui_port_load_safe());
        const auto tm_out = g::pOptsManager->comfyui_connect_timeout_load_safe();

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
        if (g::app_global_stop_all_threads.stop_requested())
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

    while (!g::app_global_stop_all_threads.stop_requested())
    {
        std::shared_ptr<RequestData> req;

        {
            std::unique_lock<std::mutex> lock(m_http_inbox_mutex);
            m_inbox_cv.wait(lock, [this]() noexcept {
                return !m_http_inbox.empty() || g::app_global_stop_all_threads.stop_requested();
            });

            if (g::app_global_stop_all_threads.stop_requested() && m_http_inbox.empty())
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

        if (req->log_this_request)
            Log("HTTP-Req#" << req->req_id << ". Started");

        httplib::Result res;
        switch (req->type)
        {
        case RequestData::Type::GetByUrl:
            res = client->Get(req->url);
            break;
        case RequestData::Type::GetByUrlAndParams:
            res = client->Get(req->url, req->params, req->headers);
            break;
        case RequestData::Type::PostRawPayload:
            res = client->Post(req->url, req->post_payload, "application/json");
            break;
        case RequestData::Type::PostItems:
            res = client->Post(req->url, req->post_payload_items);
            break;
        default:
            {
                std::unique_lock<std::mutex> lock(m_http_pickup_mutex);
                req->status = HTTP_RESP_FAIL;
                m_http_pickup.push(req);
                m_http_pickup_empty.store(false, std::memory_order_release);
                LogCError("HTTP-Req#" << req->req_id << ". Unknown request type");
            }
        }

        if (res)
        {
            req->status = res->status;
            req->data = std::move(res->body);

            if (req->log_this_request)
                Log("HTTP-Req#" << req->req_id << ". Finished. Result: " << res->status << ". Body: " << req->data.size() << " bytes");
            if (req->is_object_info && glog::Logger::get_log_object_info_enabled())
            {
                const auto obj_info_path = fs::get_log_directory_path() / "object_info.json";
                tools::path::make_file(obj_info_path, true, req->data);
                Log("/object_info dumped into " << obj_info_path);
            }
            else if (!req->is_object_info && !req->is_binary_resp && glog::Logger::get_log_web_response_enabled() && req->log_this_request)
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
            m_http_pickup_empty.store(false, std::memory_order_release);
        }
    }

    LogOut;
}

bool ConnectManager::pickup_http_response(uint32_t& req_id, int32_t& result, std::string& response)
{
    if (m_http_pickup_empty.load(std::memory_order_acquire)) [[likely]]
        return false;

    std::unique_lock<std::mutex> lock(m_http_pickup_mutex);
    if (m_http_pickup.empty()) [[unlikely]] // double check
        return false;

    auto req = m_http_pickup.front();

    if (HTTP_RESP_TIMEOUT == req->status) [[unlikely]]
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
                if (m_http_pickup.empty()) m_http_pickup_empty.store(true, std::memory_order_relaxed);
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
            if (m_http_pickup.empty()) m_http_pickup_empty.store(true, std::memory_order_relaxed);
            return false;
        }
    }

    req_id = req->req_id;
    result = req->status;
    response = std::move(req->data);

    m_http_pickup.pop();
    if (m_http_pickup.empty()) m_http_pickup_empty.store(true, std::memory_order_relaxed);
    return true;
}

bool ConnectManager::pickup_ws_message(bool& is_binary, std::string& message)
{
    if (m_ws != nullptr) [[likely]]
    {
        auto msgs = m_ws->get();
        for (auto& msg : msgs)
        {
            m_ws_pickup.push_back({ msg.binary, std::move(msg.message) });
        }
    }
    if (m_ws_pickup.empty()) [[likely]]
        return false;

    auto& msg = m_ws_pickup.front();
    is_binary = msg.first;
    message = std::move(msg.second);
    m_ws_pickup.pop_front();
    return true;
}

void ConnectManager::drop_all_messages()
{
    {
        std::unique_lock<std::mutex> lock(m_http_pickup_mutex);
        std::queue<std::shared_ptr<RequestData>> temp;
        m_http_pickup.swap(temp);
        m_http_pickup_empty.store(true, std::memory_order_release);
    }
    {
        std::unique_lock<std::mutex> lock(m_http_inbox_mutex);
        std::queue<std::shared_ptr<RequestData>> temp;
        m_http_inbox.swap(temp);
    }

    m_ws_pickup.clear();
}

} // namespace mngr
