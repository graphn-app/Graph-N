#pragma once

class number;

namespace glog
{

class Logger final
{
    friend class LogWriter;

public:
    static Logger& instance() noexcept;
    void stop() noexcept;

    static bool get_log_enabled() noexcept { return m_enabled.load(std::memory_order_relaxed); }
    static bool get_log_web_response_enabled() { return check_web_dump_file_mark(); }
    static bool get_log_object_info_enabled() { return check_object_info_file_mark(); }

    static void set_log_enabled(bool enable);
    static void disable_logging() noexcept { m_enabled.store(false, std::memory_order_relaxed); }
    static void cleanup() noexcept;

    void force_flush() noexcept;
    void write(LogWriter& writer) noexcept;

private:
    Logger() noexcept;
    ~Logger() noexcept;
    NO_COPY_MOVE(Logger);

    static bool check_log_file_mark();
    static bool check_web_dump_file_mark();
    static bool check_object_info_file_mark();

    void log_start() noexcept;
    void log_stop() noexcept;

    void working_thread();

private:
    static const char* m_log_file_mark;
    static const char* m_log_web_dump_file_mark;
    static const char* m_log_object_info_file_mark;
    static std::atomic_bool m_enabled;

    std::ofstream m_logfs;
    std::string m_log_path;

    std::atomic_bool m_do_flush = false;
    std::atomic_bool m_do_close = false;

    const size_t m_flush_timeout = 4000;
    const size_t m_overflow_limit = 20;

    std::mutex m_lock;
    std::queue<std::string> m_buffer;
    std::atomic_size_t m_buffer_size = 0;

    std::condition_variable m_cv;
    std::thread m_working_thread;
};

////////////////////////////////////////////////////
///////////////        LogWriter

class LogLineEnd { };

struct LogHex final
{
    long long h = 0;
    LogHex(long long h_) noexcept :
        h(h_)
    { }
};

struct LogHRes final
{
    uint32_t h = 0;
    LogHRes(uint32_t h_) noexcept :
        h(h_)
    { }
};

struct LogPtr final
{
    const void* ptr = nullptr;
    LogPtr(const void* ptr_) noexcept :
        ptr(ptr_)
    { }
};

class LogWriter final
{ 
public:
    LogWriter();
    ~LogWriter() noexcept = default;

    NO_COPY_MOVE(LogWriter);

    std::string extract_buffer() noexcept { return std::move(m_buffer); }
    const std::string& get_buffer() const noexcept { return m_buffer; }

    void log_line_start(const void* this_, const char* foo, int32_t line);

    LogWriter& operator << (const char* str);
    LogWriter& operator << (const std::string& str);
    LogWriter& operator << (std::string_view str);
    LogWriter& operator << (const std::filesystem::path& path);

    LogWriter& operator << (const byte_buff& buff);
    LogWriter& operator << (const tools::dt& dt);

    LogWriter& operator << (const ImVec2& vec2);
    LogWriter& operator << (const ImVec4& vec4);
    LogWriter& operator << (const ImRect& rc);

    LogWriter& operator << (int32_t i32);
    LogWriter& operator << (uint32_t ui32);
    LogWriter& operator << (int64_t i64);
    LogWriter& operator << (uint64_t ui64);
    LogWriter& operator << (float f);
    LogWriter& operator << (double d);
    LogWriter& operator << (const number& n);
    LogWriter& operator << (bool b);

    LogWriter& operator << (DlgCode code);

    LogWriter& operator << (const type_info& type);

    LogWriter& operator << (const LogLineEnd&);
    LogWriter& operator << (const LogHex& hex_item);
    LogWriter& operator << (const LogHRes& hres_item);
    LogWriter& operator << (const LogPtr& ptr);

private:
    std::string m_buffer;
};

struct LogGuard final
{
    LogGuard()
    {
        Logger::instance();
    }
    ~LogGuard()
    {
        Logger::instance().stop();
    }
};

} // namespace glog

////////////////////////////////////

#define WIDELINE2(x) L ## x
#define WIDELINE(x) WIDELINE2(x)
#define __WIDEFILE__ WIDELINE(__FILE__)

#define SAVE_EXCEPTION(message)         g::save_exception(message, __FILE__, __LINE__)

#define LogFull(ptr, message) \
    { if (glog::Logger::get_log_enabled()) \
    { \
        try { \
            glog::LogWriter log_writer__; \
            log_writer__.log_line_start(ptr, __FUNCTION__, __LINE__); \
            log_writer__ << message << glog::LogLineEnd(); \
            glog::Logger::instance().write(log_writer__); \
        } \
        catch (...) { \
            glog::Logger::instance().disable_logging(); \
            DEBUG_CATCH; \
        } \
    } }

#define LogClear(message) \
    { if (glog::Logger::get_log_enabled()) \
    { \
        try { \
            glog::LogWriter log_writer__; \
            log_writer__ << message << glog::LogLineEnd(); \
            glog::Logger::instance().write(log_writer__); \
        } \
        catch (...) { \
            glog::Logger::instance().disable_logging(); \
            DEBUG_CATCH; \
        } \
    } }

#define Log(message)    LogFull(this, message)
#define LogS(message)   LogFull(nullptr, message)

#define LOG_AND_CATCH(ptr, log_prefix, message, save) \
    { \
        try { \
            glog::LogWriter log_writer_1__; \
            log_writer_1__ << message; \
            const std::string& buffer__ = log_writer_1__.get_buffer(); \
            if (save) { SAVE_EXCEPTION(buffer__); } \
            if (glog::Logger::get_log_enabled()) \
            { \
                    glog::LogWriter log_writer_2__; \
                    log_writer_2__.log_line_start(ptr, __FUNCTION__, __LINE__); \
                    log_writer_2__ << "[" << log_prefix << "] " << buffer__ << glog::LogLineEnd(); \
                    glog::Logger::instance().write(log_writer_2__); \
                    glog::Logger::instance().force_flush(); \
            } \
        } \
        catch (...) { \
            glog::Logger::instance().disable_logging(); \
            DEBUG_CATCH; \
        } \
        DEBUG_CATCH; \
    }

#define LOG_CRITICAL(ptr, log_prefix, message) \
    { \
        try { \
            glog::LogWriter log_writer_1__; \
            log_writer_1__ << message; \
            const std::string& buffer__ = log_writer_1__.get_buffer(); \
            SAVE_EXCEPTION(buffer__); \
            if (glog::Logger::get_log_enabled()) \
            { \
                    glog::LogWriter log_writer_2__; \
                    log_writer_2__.log_line_start(ptr, __FUNCTION__, __LINE__); \
                    log_writer_2__ << "[" << log_prefix << "] " << buffer__ << glog::LogLineEnd(); \
                    glog::Logger::instance().write(log_writer_2__); \
                    glog::Logger::instance().force_flush(); \
            } \
            const std::string msg__ = "Critical error:\n" + buffer__; \
            ERROR_MESSAGE(msg__.c_str()); \
        } \
        catch (...) { \
            glog::Logger::instance().disable_logging(); \
            DEBUG_CATCH; \
        } \
        ::exit(1); \
    }

#define LogWarn(message)                         Log("[WARNING] " << message)
#define LogWarnS(message)                        LogS("[WARNING] " << message)

#define LogExcep(message)                        LOG_AND_CATCH(this, "EXCEPTION", message, true)
#define LogExcepS(message)                       LOG_AND_CATCH(nullptr, "EXCEPTION", message, true)

#define LogError(message)                        LOG_AND_CATCH(this, "ASSERT", message, true)
#define LogErrorS(message)                       LOG_AND_CATCH(nullptr, "ASSERT", message, true)

#define LogRError(message, ret)                  { LOG_AND_CATCH(this, "ASSERT", message, true); return ret; }
#define LogRErrorS(message, ret)                 { LOG_AND_CATCH(nullptr, "ASSERT", message, true); return ret; }

#define LogCError(message)                       { LOG_AND_CATCH(this, "ASSERT", message, true); continue; }
#define LogCErrorS(message)                      { LOG_AND_CATCH(nullptr, "ASSERT", message, true); continue; }

#define LogCritical(message)                     LOG_CRITICAL(this, "CRITICAL", message)
#define LogCriticalS(message)                    LOG_CRITICAL(nullptr, "CRITICAL", message)

#define LogCriticalIf(true_cond, message)        if (!(true_cond)) { LOG_CRITICAL(this, "CRITICAL", message); }
#define LogCriticalIfS(true_cond, message)       if (!(true_cond)) { LOG_CRITICAL(nullptr, "CRITICAL", message); }

#define LogAssert(true_cond, message)            if (!(true_cond)) { LOG_AND_CATCH(this, "ASSERT", message, true); }
#define LogAssertS(true_cond, message)           if (!(true_cond)) { LOG_AND_CATCH(nullptr, "ASSERT", message, true); }

#define LogRAssert(true_cond, message, ret)      if (!(true_cond)) { LOG_AND_CATCH(this, "ASSERT", message, true); return ret; }
#define LogRAssertS(true_cond, message, ret)     if (!(true_cond)) { LOG_AND_CATCH(nullptr, "ASSERT", message, true); return ret; }

#define LogCAssert(true_cond, message)           if (!(true_cond)) { LOG_AND_CATCH(this, "ASSERT", message, true); continue; }
#define LogCAssertS(true_cond, message)          if (!(true_cond)) { LOG_AND_CATCH(nullptr, "ASSERT", message, true); continue; }

#define LogSqlDbErr(message)                     LOG_AND_CATCH(this, "SQL_ERROR", message, false)

#define LogUnhandledJson(message)                Log("[JSON_UNHANDLED] " << message)
#define LogUnhandledJsonS(message)               LogS("[JSON_UNHANDLED] " << message)
#define LogInvalidJson(message)                  Log("[JSON_INVALID] " << message)
#define LogInvalidJsonS(message)                 LogS("[JSON_INVALID] " << message)

#define LogIn                                    Log(" > in")
#define LogInS                                   LogS(" > in")

#define LogOut                                   Log(" < out")
#define LogOutS                                  LogS(" < out")


// Helper to convert types that fmt doesn't handle by default (like enums)
template <typename T>
void append_arg_to_buf(fmt::memory_buffer& out, const T& arg)
{
    // If it's an enum, cast to underlying integer type (mimics stringstream behavior)
    if constexpr (std::is_enum_v<T>)
    {
        fmt::format_to(std::back_inserter(out), FMT_STRING("{}"), static_cast<std::underlying_type_t<T>>(arg));
    }

    // If it's a filesystem path, convert to generic string
    else if constexpr (std::is_same_v<T, std::filesystem::path>)
    {
        fmt::format_to(std::back_inserter(out), FMT_STRING("{}"), arg.string());
    }
    // Standard types (int, string, float, etc.)
    else
    {
        fmt::format_to(std::back_inserter(out), FMT_STRING("{}"), arg);
    }
}

template <typename... Args>
std::string format_log_msg(Args&&... args)
{
    // Use memory_buffer for performance (avoids heap allocations for small strings)
    fmt::memory_buffer out;
    std::string_view sep = "";

    // C++17 fold expression to iterate over arguments
    // Appends separator only after the first element
    const auto append = [&](auto&& arg)
    {
        // separator
        if (!sep.empty())
            fmt::format_to(std::back_inserter(out), FMT_STRING("{}"), sep);

        // use the helper to print the argument
        append_arg_to_buf(out, arg);

        sep = ", ";
    };
    (append(std::forward<Args>(args)), ...);

    return fmt::to_string(out);
}
#define LogV(...)                    Log(format_log_msg(__VA_ARGS__))
#define LogVS(...)                   LogS(format_log_msg(__VA_ARGS__))

#ifdef _WIN32
#define LogApiERR                    uint32_t(GetLastError())
#elif defined __APPLE__
#error Not implemented
#elif defined __linux__
#error Not implemented
#endif
#define LogHEX(h)                    glog::LogHex(h)
#define LogHRES(h)                   glog::LogHRes(uint32_t(h))
#define LogPTR(p)                    glog::LogPtr(p)

