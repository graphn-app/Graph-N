
#pragma once

#pragma warning(push, 0)
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#pragma warning(pop)

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
using work_guard_type = net::executor_work_guard<net::io_context::executor_type>;

namespace mngr
{

struct Message final
{
    bool binary;
    std::string message;

    Message(bool b, std::string&& m)
    {
        binary = b;
        message = std::move(m);
    }
};

class WebSocket final : public std::enable_shared_from_this<WebSocket>
{
    WebSocket();
    NO_COPY_MOVE(WebSocket);

public:
    ~WebSocket();
    static std::shared_ptr<WebSocket> create_and_connect(const std::string& host, int64_t port, int64_t timeout, int64_t ping_interval);
    void stop_and_forget();

    inline ConnectState get_state() const { return m_state.load(std::memory_order_relaxed); }

    void send(std::string&& text);

    std::list<Message> get();
    inline size_t peek() const { return m_inbox_size; }

    inline int64_t get_latency() const { return m_latency.load(std::memory_order_relaxed); }

    inline const std::string& get_last_error() const { return m_last_error; }

private:
    void ping();

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);
    void on_handshake(beast::error_code ec);
    void on_close(beast::error_code ec);
    void fail(beast::error_code ec, char const* what);

    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);

    void do_write();
    void on_write(beast::error_code ec, std::size_t bytes_transferred);

private:
    std::atomic<ConnectState> m_state = ConnectState::Disconnected;
    std::atomic_int64_t m_latency = 0;

    std::thread m_th_working;

    net::io_context m_ioc;
    tcp::resolver m_resolver;
    websocket::stream<beast::tcp_stream> m_ws;

    std::mutex m_mutex;
    beast::flat_buffer m_buffer;

    std::queue<std::string> m_write_queue;
    std::list<Message> m_inbox;
    std::atomic_size_t m_inbox_size = 0;

    std::string m_host;
    int64_t m_timeout = 0;
    std::string m_last_error;

    std::thread m_th_ping;
    int64_t m_ping_interval;

    std::atomic_bool m_stop_flag = false;
};

} // namespace mngr
