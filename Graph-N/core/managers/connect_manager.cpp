
#include "core/managers/connect_manager.h"
#include "core/managers/settings_manager.h"

namespace mngr
{

ConnectManager::~ConnectManager()
{
    LogF;
}

bool ConnectManager::init()
{
    LogF;

    return true;
}

void ConnectManager::free()
{
    LogF;
}

void ConnectManager::stop_all_threads()
{
    LogF_In;

    if (m_conn_data)
    {
        Log("Terminating thread " << m_conn_data->conn_th_id);
        m_conn_data->stop_thread_flag = true;
    }
    m_conn_data.reset();

    if (m_thread.joinable())
        m_thread.detach();

    LogF_Out;
}

ConnectState ConnectManager::check_status()
{
    using namespace std::chrono_literals;
    if (ConnectState::connecting == m_state)
    {
        if (m_future.wait_for(0ms) == std::future_status::ready)
        {
            if (m_thread.joinable())
                m_thread.join();

            auto data = m_future.get();
            if (data->conn_th_id != m_conn_th_id)
            {
                LogError(ExcepCategory::logic, "Thread id not equal. Finished " << data->conn_th_id << ", expected: " << m_conn_th_id);
            }
            if (ConnectState::connected == data->state)
            {
                m_state = ConnectState::connected;
                Log("Connected thread " << data->conn_th_id);
                m_nodes_raw = std::move(data->data);
            }
            else
            {
                Log("Connection thread " << data->conn_th_id << " finished with status " << cast::to_u64(data->state));
            }
        }
    }

    return m_state;
}

void ConnectManager::connect()
{
    LogF;

    stop_all_threads();

    m_conn_data = std::make_shared<ConnectData>();
    m_conn_data->host = g::pOptsManager->comfyui_host_load();
    m_conn_data->port = cast::to_i32(g::pOptsManager->comfyui_port_load());
    m_conn_data->conn_timeout = g::pOptsManager->comfyui_connect_timeout_load();
    m_conn_data->read_timeout = g::pOptsManager->comfyui_read_timeout_load();
    m_conn_data->conn_th_id = ++m_conn_th_id;
    Log("Starting connection thread " << m_conn_data->conn_th_id);

    m_state = ConnectState::connecting;
    std::packaged_task<std::shared_ptr<ConnectData>()> task(std::bind(&ConnectManager::connect_thread, m_conn_data));
    m_future = task.get_future();
    m_thread = std::thread(std::move(task));
}

void ConnectManager::stop_connect()
{
    LogF;
    stop_all_threads();
    if (ConnectState::connecting == m_state)
        m_state = ConnectState::disconnected;
}

std::shared_ptr<ConnectManager::ConnectData> ConnectManager::connect_thread(std::shared_ptr<ConnectData> data)
{
    LogF;

    Log("Thread " << data->conn_th_id << ". Connecting to " APP_COMFYUI ". " << data->host << ":" << data->port << ", ct=" << data->conn_timeout << ", rt=" << data->read_timeout);
    using namespace std::chrono_literals;
    while (!data->stop_thread_flag)
    {
        try
        {
            httplib::Client cli(data->host, data->port);
            cli.set_keep_alive(true);
            cli.set_connection_timeout(data->conn_timeout);
            cli.set_read_timeout(data->read_timeout);

            const auto res = cli.Get(comfyui_api::object_info);
            if (data->stop_thread_flag)
            {
                Log("Thread " << data->conn_th_id << ". Connection thread aborted");
                data->state = ConnectState::disconnected;
                return data;
            }

            if (res && 200 == res->status)
            {
                Log("Thread " << data->conn_th_id << "Start reading nodes: " << res->body.size() << " bytes");
                data->state = ConnectState::connected;
                data->data = std::move(res->body);
                return data;
            }

            Log("Thread " << data->conn_th_id << ". Connect failed. Err: " << httplib::to_string(res.error()));

            for (int i = 0; i < 10; ++i)
            { 
                if (data->stop_thread_flag)
                    break;
                std::this_thread::sleep_for(100ms);
            }
        }
        catch (const std::exception& e)
        {
            LogExcep(ExcepCategory::comfyui, "Thread " << data->conn_th_id << ". Connection failed with err " << e.what());
        }
    }

    if (data->stop_thread_flag)
    {
        Log("Thread " << data->conn_th_id << ". Connection thread aborted");
        data->state = ConnectState::disconnected;
        return data;
    }

    LogAndThrow(ExcepCategory::logic, "Thread " << data->conn_th_id << ". Unexpected exit flow");
}

} // namespace mngr
