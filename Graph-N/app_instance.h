#pragma once

namespace httplib { class Server; }

class AppInstance final
{
public:
    using WakeUpCallback = std::function<void(const std::string& args)>;

    AppInstance(WakeUpCallback on_wake_up);
    ~AppInstance() noexcept;

    NO_COPY_MOVE(AppInstance);

    // returns true, if this is first instance
    // returns false, if main instance already running (we should close)
    bool is_primary() const noexcept { return m_is_primary; }

private:
    void notify_existing_instance();
    void start_server();

private:
    const char* m_port_file;
    const char* m_cmd_ping;
    const char* m_cmd_pong;
    const char* m_cmd_wakeup;

    int32_t m_port = 0;
    WakeUpCallback m_on_wake_up = nullptr;
    std::unique_ptr<httplib::Server> m_server;
    std::thread m_server_thread;
    bool m_is_primary = false;
};


