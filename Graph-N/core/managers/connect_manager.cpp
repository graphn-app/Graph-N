
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
    return true;
}

void ConnectManager::stop_all_threads()
{
    LogF_In;

    {
        std::unique_lock<std::mutex> lock(m_req_mutex);
        while (!m_requests.empty())
            m_requests.pop();
    }
    {
        std::unique_lock<std::mutex> lock(m_cli_mutex);
        if (m_http_client)
            m_http_client->stop();
    }
    if (m_req_thread.joinable())
        m_req_thread.join();

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

ConnectState ConnectManager::get_status(size_t* inbox /*= nullptr*/, int64_t* latency /*= nullptr*/)
{
    if (latency)
        *latency = -1;
    if (inbox)
        *inbox = 0;

    if (m_req_future.valid())
    {
        using namespace std::chrono_literals;
        if (m_req_future.wait_for(0ms) == std::future_status::ready)
        {
            if (m_req_thread.joinable())
                m_req_thread.join();

            auto req_data = m_req_future.get();
            Log("Request '" << req_data->request << "' is done. Ok=" << req_data->success << ", data=" << req_data->data.size());
            req_data->callback(req_data->success, std::move(req_data->data));

            std::unique_lock<std::mutex> lock(m_req_mutex);
            if (!m_requests.empty() && ConnectState::Connected == m_ws->get_state())
            {
                req_data = m_requests.front();
                m_requests.pop();
                send_http_request_internal(req_data);
            }
        }
    }

    if (m_ws)
    {
        const auto state = m_ws->get_state();
        if (ConnectState::Connected == state)
        {
            if (latency)
                *latency = m_ws->get_latency();
            if (inbox)
                *inbox = m_ws->peek();

            if (m_prev_state != state && !m_req_future.valid())
            {
                std::unique_lock<std::mutex> lock(m_req_mutex);
                if (!m_requests.empty())
                {
                    auto req_data = m_requests.front();
                    m_requests.pop();
                    send_http_request_internal(req_data);
                }
            }
        }
        m_prev_state = state;
        return state;
    }

    m_prev_state = ConnectState::Disconnected;
    return ConnectState::Disconnected;
}

void ConnectManager::send_http_request(const char* request, bool post, RequestCallback callback)
{
    auto req = std::make_shared<RequestData>();
    req->request = request;
    req->post = post;
    req->callback = callback;

    const auto state = m_ws ? m_ws->get_state() : ConnectState::Disconnected;
    std::unique_lock<std::mutex> lock(m_req_mutex);
    if (state != ConnectState::Connected || m_req_future.valid())
    {
        m_requests.push(req);
        return;
    }

    send_http_request_internal(req);
}

void ConnectManager::send_http_request_internal(std::shared_ptr<RequestData> req)
{
    req->reconnect = false;
    req->host = g::pOptsManager->comfyui_host_load();
    req->port = cast::to_i32(g::pOptsManager->comfyui_port_load());
    if (req->host != m_prev_host || req->port != m_prev_port)
    {
        req->reconnect = true;
        m_prev_host = req->host;
        m_prev_port = req->port;
    }
    req->conn_timeout = g::pOptsManager->comfyui_connect_timeout_load();

    std::packaged_task<std::shared_ptr<RequestData>()> task(std::bind(&ConnectManager::request_thread, this, req));
    m_req_future = task.get_future();
    m_req_thread = std::thread(std::move(task));
}

std::shared_ptr<ConnectManager::RequestData> ConnectManager::request_thread(std::shared_ptr<RequestData> request)
{
    LogS("Requesting '" << request->request << "' to " << request->host << ":" << request->port);

    std::unique_lock<std::mutex> lock(m_cli_mutex);
    if (request->reconnect || !m_http_client)
    {
        m_http_client = std::make_unique<httplib::Client>(request->host, request->port);
        m_http_client->set_connection_timeout(request->conn_timeout);
        m_http_client->set_keep_alive(true);
    }
    auto* cli = m_http_client.get();
    lock.unlock();

    httplib::Result res;
    if (request->post)
        res = cli->Post(request->request);
    else
        res = cli->Get(request->request);

    request->success = false;
    if (res && 200 == res->status)
    {
        request->success = true;
        request->data = std::move(res->body);        
    }

    return request;
}

} // namespace mngr
