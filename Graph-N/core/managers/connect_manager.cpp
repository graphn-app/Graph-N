
#include "core/managers/connect_manager.h"
#include "core/managers/settings_manager.h"

namespace mngr
{

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
    LogF;
    m_stop_thread_flag = true;
    if (m_thread.joinable())
        m_thread.join();
}

ConnectState ConnectManager::check_status()
{
    using namespace std::chrono_literals;
    if (ConnectState::connecting == m_state)
    {
        if (m_future.wait_for(1ms) == std::future_status::ready)
        {
            if (m_thread.joinable())
                m_thread.join();

            auto data = m_future.get();
            if (ConnectState::connected == data.first)
            {
                m_state = ConnectState::connected;
                m_stop_thread_flag = false;
                Log("Connected");
                m_nodes_raw = std::move(data.second);
            }
            else
            {
                Log("Connection stopped with status " << cast::to_u64(data.first));
            }
        }
    }

    return m_state;
}

void ConnectManager::connect()
{
    LogF;

    stop_all_threads();

    m_stop_thread_flag = false;
    m_state = ConnectState::connecting;
    std::packaged_task<std::pair<ConnectState, std::string>()> task(std::bind(&ConnectManager::connect_thread, this));
    m_future = task.get_future();
    m_thread = std::thread(std::move(task));
}

void ConnectManager::stop_connect()
{
    LogF;
    stop_all_threads();
}

std::pair<ConnectState, std::string> ConnectManager::connect_thread()
{
    LogF;

    using namespace std::chrono_literals;
    while (!m_stop_thread_flag)
    {
        try
        {
            const auto host = g::pOptsManager->comfyui_get_host();
            const auto port = g::pOptsManager->comfyui_get_port();

            httplib::Client cli(host, port);
            cli.set_keep_alive(true);
            cli.set_connection_timeout(g::pOptsManager->comfyui_get_connect_timeout());
            cli.set_read_timeout(g::pOptsManager->comfyui_get_read_timeout());

            auto res = cli.Get("/");

            if (m_stop_thread_flag)
            {
                Log("Connection thread aborted");
                return std::make_pair(ConnectState::disconnected, "");
            }

            if (!res)
            {
                std::this_thread::sleep_for(1ms);
                continue;
            }

            Log("Start reading nodes data");

            res = cli.Get(comfyui_api::object_info);

            if (m_stop_thread_flag)
            {
                Log("Connection thread aborted");
                return std::make_pair(ConnectState::disconnected, "");
            }

            if (res && 200 == res->status)
            {
                Log("Read " << res->body.size() << " bytes");
                return std::make_pair(ConnectState::connected, std::move(res->body));
            }

            Log("Reading failed. Error code " << httplib::to_string(res.error()));

            std::this_thread::sleep_for(1ms);
        }
        catch (const std::exception& e)
        {
            LogExcep(ExcepCategory::comfyui, "Connection failed with err " << e.what());
        }
    }

    if (m_stop_thread_flag)
    {
        Log("Connection thread aborted");
        return std::make_pair(ConnectState::disconnected, "");
    }

    LogAndThrow(ExcepCategory::logic, "Unexpected exit flow");
}

} // namespace mngr
