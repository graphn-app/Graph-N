#include "app_instance.h"

AppInstance::AppInstance(WakeUpCallback on_wake_up)
    : m_port_file("graph-n.port")
    , m_cmd_ping("/06EA77DDA7B844A89FE4478B248DB159")
    , m_cmd_pong("0113EF5A019142758FF784F6AE8AC343")
    , m_cmd_wakeup("/4EFB863641614642BCAECDE14E1597A5")
    , m_on_wake_up(on_wake_up)
{
    m_server = std::make_unique<httplib::Server>();
    auto port_path = fs::get_root_directory_path();
    port_path /= m_port_file;

    int32_t target_port = 0;
    std::ifstream in_file(port_path);
    if (in_file.is_open())
    {
        in_file >> target_port;
        in_file.close();
    }

    if (target_port > 0)
    {
        Log("Target port: " << target_port);
        httplib::Client cli("127.0.0.1", target_port);
        cli.set_connection_timeout(0, 500000); // 0.5 sec
        const auto res = cli.Get(m_cmd_ping);
        if (res && 200 == res->status && res->body == m_cmd_pong)
        {
            Log("Server replied");
            m_port = target_port;
            m_is_primary = false;
            notify_existing_instance();
            return;
        }
    }

    // we're the first instance
    // search for available port
    const int32_t port = m_server->bind_to_any_port("127.0.0.1");
    if (port > 0)
    {
        Log("Port binded: " << port);
        m_port = port;
        m_is_primary = true;

        std::ofstream out_file(port_path);
        out_file << m_port;
        out_file.close();

        start_server();
    }
    else
    {
        Log("Http critical: cannot bind to any port");
    }
}

AppInstance::~AppInstance()
{
    if (m_server)
    {
        try
        {
            m_server->stop();
            if (m_server_thread.joinable())
                m_server_thread.join();
        }
        catch (...) {}
    }
}

void AppInstance::notify_existing_instance()
{
    LogV("127.0.0.1", m_port);
    httplib::Client cli("127.0.0.1", m_port);
    cli.Post(m_cmd_wakeup, "", "text/plain");
}

void AppInstance::start_server()
{
    m_server->Get(m_cmd_ping,
        [pong = m_cmd_pong](const httplib::Request&, httplib::Response& res)
        {
            res.set_content(pong, "text/plain");
        }
    );

    m_server->Post(m_cmd_wakeup,
        [this](const httplib::Request& req, httplib::Response& res)
        {
            if (m_on_wake_up)
            {
                m_on_wake_up(req.body);
            }
            res.set_content("OK", "text/plain");
        }
    );

    m_server_thread = std::thread(
        [this]()
        {
            m_server->listen_after_bind();
        }
    );
}
