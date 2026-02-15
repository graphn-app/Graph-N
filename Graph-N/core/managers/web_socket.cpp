
#include "core/managers/web_socket.h"

namespace mngr
{

WebSocket::WebSocket()
    : m_resolver(net::make_strand(m_ioc))
    , m_ws(net::make_strand(m_ioc)) 
{
    LogF;
}

WebSocket::~WebSocket()
{
    // DO NOT LOG, logger object might be destroyed already
}

std::shared_ptr<WebSocket> WebSocket::create_and_connect(const std::string& host, int64_t port, int64_t timeout)
{
    auto obj = std::shared_ptr<WebSocket>(new WebSocket);
    obj->m_timeout = timeout;
    obj->m_host = host;

    obj->m_th = std::thread([self = obj->shared_from_this()]
        {
            [[maybe_unused]] auto work_guard = net::make_work_guard(self->m_ioc);
            self->m_ioc.run();
        }
    );

    LogS("Connection started at " << host << ":" << port);
    obj->m_resolver.async_resolve(host, std::to_string(port), beast::bind_front_handler(&WebSocket::on_resolve, obj->shared_from_this()));

    return obj;
}

void WebSocket::stop_and_forget()
{
    if (m_th.joinable())
    {
        Log("WebSocket thread to stop");
        net::post(m_ioc, [this] { m_ioc.stop(); });
        m_th.detach();
    }
}

void WebSocket::send(std::string&& text)
{
    net::post(m_ws.get_executor(),
        [self = shared_from_this(), text = std::move(text)]() mutable
        {
            self->m_write_queue.push(std::move(text));
            if (self->m_write_queue.size() > 1)
                return;
            self->do_write();
        }
    );
}

void WebSocket::ping()
{
    const auto now = std::chrono::steady_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    net::post(m_ws.get_executor(),
        [self = shared_from_this(), payload =std::to_string(ms)]()
        {
            self->m_ws.async_ping(websocket::ping_data(payload), 
                [self](beast::error_code)
            { /* ignore */ });
        }
    );
}

std::list<Message> WebSocket::get()
{
    std::list<Message> result;
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_inbox.empty())
    {
        std::swap(result, m_inbox);
        m_inbox_size = 0;
    }
    return result;
}

void WebSocket::on_resolve(beast::error_code ec, tcp::resolver::results_type results)
{
    if (ec)
        return fail(ec, "resolve");

    beast::get_lowest_layer(m_ws).expires_after(std::chrono::seconds(m_timeout));
    beast::get_lowest_layer(m_ws).async_connect(results, beast::bind_front_handler(&WebSocket::on_connect, shared_from_this()));
}

void WebSocket::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type /*ep*/)
{
    if (ec)
        return fail(ec, "connect");

    beast::get_lowest_layer(m_ws).expires_never();
    m_ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
    m_ws.set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req)
        {
            req.set(http::field::user_agent, APP_CAPTION "-Client");
        }
    ));

    const auto target = fmt::format(comfyui_ws::root, base::GlobalID::client_id());
    m_ws.async_handshake(m_host, target, beast::bind_front_handler(&WebSocket::on_handshake, shared_from_this()));
}

void WebSocket::on_handshake(beast::error_code ec)
{
    if (ec)
        return fail(ec, "handshake");
    m_connected = true;

    m_ws.control_callback([this](websocket::frame_type kind, beast::string_view payload)
    {
        if (websocket::frame_type::pong == kind)
        {
            try
            {
                const auto sent = std::stoll(std::string(payload));
                const auto now = std::chrono::steady_clock::now();
                const auto curr = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                m_latency = cast::to_i64(curr - sent);
            }
            catch(...)
            {}
        }
    });
    do_read();
}

void WebSocket::do_read()
{
    m_ws.async_read(m_buffer, beast::bind_front_handler(&WebSocket::on_read, shared_from_this()));
}

void WebSocket::on_read(beast::error_code ec, std::size_t /*bytes_transferred*/)
{
    if (ec)
        return fail(ec, "read");

    std::string data = beast::buffers_to_string(m_buffer.data());
    const bool is_bin = m_ws.got_binary();
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_inbox.emplace_back(is_bin, std::move(data));
        m_inbox_size = m_inbox.size();
    }
    m_buffer.consume(m_buffer.size());
    do_read();
}

void WebSocket::do_write()
{
    m_ws.async_write(net::buffer(m_write_queue.front()), beast::bind_front_handler(&WebSocket::on_write, shared_from_this()));
}

void WebSocket::on_write(beast::error_code ec, std::size_t /*bytes_transferred*/)
{
    if (ec)
        return fail(ec, "write");

    m_write_queue.pop();
    if (!m_write_queue.empty())
        do_write();
}

void WebSocket::on_close(beast::error_code ec)
{
    fail(ec, "close");
}

void WebSocket::fail(beast::error_code ec, char const* what)
{
    m_connected = false;
    m_last_error = fmt::format("WS last error: {}, {}", ec.value(), what);
}
} // namespace mngr

