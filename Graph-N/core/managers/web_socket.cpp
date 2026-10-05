#include "core/managers/web_socket.h"
#include "core/base/comfyui_web_paths.h"

namespace mngr
{

WebSocket::WebSocket()
    : m_resolver(net::make_strand(m_ioc))
    , m_ws(net::make_strand(m_ioc)) 
{
}

WebSocket::~WebSocket() noexcept = default;

std::shared_ptr<WebSocket> WebSocket::create_and_connect(const std::string& host, int64_t port, int64_t timeout, int64_t ping_interval)
{
    // constructor is private, so we can't use make_shared directly
    auto obj = std::shared_ptr<WebSocket>(new WebSocket);
    obj->m_timeout = timeout;
    obj->m_host = host;
    obj->m_state.store(ConnectState::Connecting, std::memory_order_relaxed);

    obj->m_th_working = std::thread([self = obj->shared_from_this()]
        {
            [[maybe_unused]] auto work_guard = net::make_work_guard(self->m_ioc);
            self->m_ioc.run();
        }
    );

    obj->m_th_ping = std::thread([self = obj->shared_from_this(), ping_interval]
        {
            constexpr const uint32_t sleep_time = 100;
            for (uint32_t elapsed = 0; !self->m_stop_flag.stop_requested(); elapsed += sleep_time)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
                if (elapsed >= ping_interval * 1000)
                {
                    self->ping();
                    elapsed = 0;
                }
            }
        }
    );

    LogS("Connection started at " << host << ":" << port);
    obj->m_resolver.async_resolve(host, std::to_string(port), beast::bind_front_handler(&WebSocket::on_resolve, obj->shared_from_this()));

    return obj;
}

void WebSocket::stop_and_forget()
{
    if (m_th_ping.joinable())
    {
        m_stop_flag.request_stop();
        // ideally these should be joined, detach can crash if globals are destroyed during exit
        m_th_ping.detach();
    }
    if (m_th_working.joinable())
    {
        Log("WebSocket thread to stop");
        net::post(m_ioc, [this] { m_ioc.stop(); });
        // same here, detaching working thread during teardown is risky
        m_th_working.detach();
    }
}

void WebSocket::send(std::string&& text)
{
    net::post(m_ws.get_executor(),
        [self = shared_from_this(), text = std::move(text)]() mutable
        {
            self->m_write_queue.push(std::move(text));
            // handles overlapping async writes safely
            if (self->m_write_queue.size() > 1)
                return;
            self->do_write();
        }
    );
}

void WebSocket::ping()
{
    const auto now = std::chrono::steady_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();

    net::post(m_ws.get_executor(),
        [self = shared_from_this(), payload = std::to_string(ms)]()
        {
            // capturing payload inside the completion handler keeps it alive for beast to read
            self->m_ws.async_ping(websocket::ping_data(payload), 
                [self, payload](beast::error_code)
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
        m_inbox_size.store(0, std::memory_order_relaxed);
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
    {
        if (beast::error::timeout == ec)
            return fail(ec, "connect timeout (server is dead or blocked)");
        return fail(ec, "connect");
    }

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
    m_state.store(ConnectState::Connected, std::memory_order_relaxed);

    m_ws.control_callback([this](websocket::frame_type kind, beast::string_view payload)
    {
        if (websocket::frame_type::pong == kind)
        {
            try
            {
                const auto sent = std::stoll(std::string(payload));
                const auto curr = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
                m_latency.store(cast::to_i64(curr - sent), std::memory_order_relaxed);
            }
            catch(...)
            {}// ignore parsing exceptions from corrupted pong payloads
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
        bool keep_msg = true;
        if (is_bin)
        {
            Log("New binary message size: " << data.size() << " bytes");
        }
        else
        {
            keep_msg = is_useful(data);
            Log("New text message (" << (keep_msg ? "keep" : "ignore") << ") size: " << data.size() << " bytes");
            if (glog::Logger::get_log_web_response_enabled())
                LogClear(data);
        }
        if (keep_msg)
        {
            m_inbox.emplace_back(is_bin, std::move(data));
            m_inbox_size.store(m_inbox.size(), std::memory_order_relaxed);
        }
    }
    m_buffer.consume(m_buffer.size());
    do_read();
}

bool WebSocket::is_useful(const std::string& message) const
{
    try
    {
        auto j = u_json::parse(message);
        if (!j.contains("type") || !j["type"].is_string()) [[unlikely]]
            return false;

        const std::string& type = j["type"].get_ref<std::string&>();
        if (type == "execution_start" ||
            type == "executing" ||
            type == "progress" ||
            type == "execution_success" ||
            type == "execution_error" ||
            type == "execution_interrupted" ||
            type == "executed" ||
            type == "status")
        {
            return true;
        }
    }
    catch (const std::exception& ex)
    {
        LogExcep("Parse ws message: " << ex.what());
    }
    return false;
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
    m_state.store(ConnectState::Disconnected, std::memory_order_relaxed);
    m_last_error = fmt::format(FMT_STRING("WS last error: {}, {}"), ec.value(), what);
    Log(m_last_error);
}
} // namespace mngr
