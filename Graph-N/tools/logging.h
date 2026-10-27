/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once

#ifndef TOOLS_DETACHED_STATE
class number;
#endif // TOOLS_DETACHED_STATE

namespace glog
{

class Logger final
{
    friend class LogWriter;

public:
    static Logger& instance() noexcept;
    void stop() noexcept;

    static bool get_log_enabled() noexcept { return m_log_file_mark_enabled; }
    static bool get_log_web_response_enabled() noexcept { return m_log_web_dump_file_mark_enabled; }
    static bool get_log_object_info_enabled() noexcept { return m_log_object_info_file_mark_enabled; }
    static bool get_log_dump_invalid_json_enabled() noexcept { return m_log_invalid_json_mark_enabled; }

    static bool get_log_started() noexcept { return m_enabled.load(std::memory_order_relaxed); }
    static void set_log_stopped() noexcept { m_enabled.store(false, std::memory_order_relaxed); }

    static void set_log_enabled(bool enabled);
    static void set_log_web_response_enabled(bool enabled);
    static void set_log_object_info_enabled(bool enabled);
    static void set_log_dump_invalid_json_enabled(bool enabled);

    static void cleanup() noexcept;

    void force_flush() noexcept;
    void write(LogWriter& writer) noexcept;

private:
    Logger();
    ~Logger() noexcept;

    NO_COPY_MOVE(Logger);

    static void check_log_file_mark();
    static void set_log_enabled(bool enable, const char* log_mark);

    void log_start();
    void log_stop() noexcept;

    void working_thread();

private:
    static bool m_log_file_mark_enabled;
    static bool m_log_web_dump_file_mark_enabled;
    static bool m_log_object_info_file_mark_enabled;
    static bool m_log_invalid_json_mark_enabled;

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

class LogLineEnd final { };

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
    LogWriter& operator << (const wchar_t* str);
    LogWriter& operator << (const std::string& str);
    LogWriter& operator << (std::string_view str);
    LogWriter& operator << (const std::filesystem::path& path);

    LogWriter& operator << (const byte_buff& buff);
    LogWriter& operator << (const tools::dt& dt);

#ifndef TOOLS_DETACHED_STATE
    LogWriter& operator << (const ImVec2& vec2);
    LogWriter& operator << (const ImVec4& vec4);
    LogWriter& operator << (const ImRect& rc);
#endif // TOOLS_DETACHED_STATE

    LogWriter& operator << (int32_t i32);
    LogWriter& operator << (uint32_t ui32);
    LogWriter& operator << (int64_t i64);
    LogWriter& operator << (uint64_t ui64);
    LogWriter& operator << (float f);
    LogWriter& operator << (double d);
    LogWriter& operator << (bool b);
#ifdef _WIN32
    LogWriter& operator << (DWORD dw);
#endif // _WIN32

#ifndef TOOLS_DETACHED_STATE
    LogWriter& operator << (const number& n);

    LogWriter& operator << (DlgCode code);
#endif // TOOLS_DETACHED_STATE

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
    LogGuard() noexcept
    {
        Logger::instance();
    }
    ~LogGuard() noexcept
    {
        glog::Logger::cleanup();
        Logger::instance().stop();
    }
};

} // namespace glog

////////////////////////////////////

#define WIDELINE2(x) L ## x
#define WIDELINE(x) WIDELINE2(x)
#define __WIDEFILE__ WIDELINE(__FILE__)

#ifndef TOOLS_DETACHED_STATE
#define SAVE_EXCEPTION(message)         g::save_exception(message, __FILE__, __LINE__)
#else
#define SAVE_EXCEPTION(message)
#endif // TOOLS_DETACHED_STATE

#if defined (_DEBUG) && defined (_WIN32) && !defined (SKIP_DEBUG_CATCH)
#define DEBUG_CATCH \
    do { \
        switch (::MessageBoxA(nullptr, \
            "Some error happened\nTo exit app - press 'Abort'\nTo attach debugger - press 'Retry'\nTo continue execution - press 'Ignore')", \
            APP_CAPTION, MB_ABORTRETRYIGNORE | MB_ICONWARNING | MB_TASKMODAL | MB_TOPMOST)) \
        { \
        case IDABORT: \
            ::exit(1); \
            break; \
        default: \
        case IDRETRY: \
            __debugbreak(); \
            break; \
        case IDIGNORE: \
            break; \
        } \
    } while(false)
#else
#define DEBUG_CATCH
#endif // _DEBUG && _WIN32 && SKIP_DEBUG_CATCH

#define LogFull(ptr, message) \
    do { if (glog::Logger::get_log_started()) \
    { \
        try { \
            glog::LogWriter log_writer__; \
            log_writer__.log_line_start(ptr, __FUNCTION__, __LINE__); \
            log_writer__ << message << glog::LogLineEnd(); \
            glog::Logger::instance().write(log_writer__); \
        } \
        catch (...) { \
            glog::Logger::set_log_stopped(); \
            DEBUG_CATCH; \
        } \
    } }  while(false)

#define LogClear(message) \
    do { if (glog::Logger::get_log_started()) \
    { \
        try { \
            glog::LogWriter log_writer__; \
            log_writer__ << message << glog::LogLineEnd(); \
            glog::Logger::instance().write(log_writer__); \
        } \
        catch (...) { \
            glog::Logger::set_log_stopped(); \
            DEBUG_CATCH; \
        } \
    } }  while(false)

#define Log(message)    LogFull(this, message)
#define LogS(message)   LogFull(nullptr, message)

#define LOG_AND_CATCH(ptr, log_prefix, message, save) \
    do { \
        try { \
            glog::LogWriter log_writer_1__; \
            log_writer_1__ << message; \
            const std::string& buffer__ = log_writer_1__.get_buffer(); \
            if (save) { SAVE_EXCEPTION(buffer__); } \
            if (glog::Logger::get_log_started()) \
            { \
                    glog::LogWriter log_writer_2__; \
                    log_writer_2__.log_line_start(ptr, __FUNCTION__, __LINE__); \
                    log_writer_2__ << "[" << log_prefix << "] " << buffer__ << glog::LogLineEnd(); \
                    glog::Logger::instance().write(log_writer_2__); \
                    glog::Logger::instance().force_flush(); \
            } \
        } \
        catch (...) { \
            glog::Logger::set_log_stopped(); \
            DEBUG_CATCH; \
        } \
        DEBUG_CATCH; \
    }  while(false)

#define LOG_CRITICAL(ptr, log_prefix, message) \
    do { \
        try { \
            glog::LogWriter log_writer_1__; \
            log_writer_1__ << message; \
            const std::string& buffer__ = log_writer_1__.get_buffer(); \
            SAVE_EXCEPTION(buffer__); \
            if (glog::Logger::get_log_started()) \
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
            glog::Logger::set_log_stopped(); \
            DEBUG_CATCH; \
        } \
        ::exit(1); \
    } while(false)

#define LogWarn(message)                         Log("[WARNING] " << message)
#define LogWarnS(message)                        LogS("[WARNING] " << message)

#define LogExcep(message)                        LOG_AND_CATCH(this, "EXCEPTION", message, true)
#define LogExcepS(message)                       LOG_AND_CATCH(nullptr, "EXCEPTION", message, true)

#define LogError(message)                        LOG_AND_CATCH(this, "ASSERT", message, true)
#define LogErrorS(message)                       LOG_AND_CATCH(nullptr, "ASSERT", message, true)

#define LogRError(message, ret)                  { LOG_AND_CATCH(this, "ASSERT", message, true); return ret; }
#define LogRErrorS(message, ret)                 { LOG_AND_CATCH(nullptr, "ASSERT", message, true); return ret; }

#define LogCError(message)                       if (true) { LOG_AND_CATCH(this, "ASSERT", message, true); continue; } else (void)0
#define LogCErrorS(message)                      if (true) { LOG_AND_CATCH(nullptr, "ASSERT", message, true); continue; } else (void)0

#define LogCritical(message)                     LOG_CRITICAL(this, "CRITICAL", message)
#define LogCriticalS(message)                    LOG_CRITICAL(nullptr, "CRITICAL", message)

#define LogCriticalIf(true_cond, message)        do { if (!(true_cond)) [[unlikely]] { LOG_CRITICAL(this, "CRITICAL", message); } } while(false)
#define LogCriticalIfS(true_cond, message)       do { if (!(true_cond)) [[unlikely]] { LOG_CRITICAL(nullptr, "CRITICAL", message); } } while(false)

#define LogAssert(true_cond, message)            do { if (!(true_cond)) [[unlikely]] { LOG_AND_CATCH(this, "ASSERT", message, true); } } while(false)
#define LogAssertS(true_cond, message)           do { if (!(true_cond)) [[unlikely]] { LOG_AND_CATCH(nullptr, "ASSERT", message, true); } } while(false)

#define LogRAssert(true_cond, message, ret)      do { if (!(true_cond)) [[unlikely]] { LOG_AND_CATCH(this, "ASSERT", message, true); return ret; } } while(false)
#define LogRAssertS(true_cond, message, ret)     do { if (!(true_cond)) [[unlikely]] { LOG_AND_CATCH(nullptr, "ASSERT", message, true); return ret; } } while(false)

#define LogCAssert(true_cond, message)           if (!(true_cond)) [[unlikely]] { LOG_AND_CATCH(this, "ASSERT", message, true); continue; } else (void)0
#define LogCAssertS(true_cond, message)          if (!(true_cond)) [[unlikely]] { LOG_AND_CATCH(nullptr, "ASSERT", message, true); continue; } else (void)0

#ifndef TOOLS_DETACHED_STATE
#define LogSqlDbErr(message)                     LOG_AND_CATCH(this, "SQL_ERROR", message, false)

#define LogUnhandledJson(message)                while(glog::Logger::get_log_dump_invalid_json_enabled()) { Log("[JSON_UNHANDLED] " << message); break; }
#define LogUnhandledJsonS(message)               while(glog::Logger::get_log_dump_invalid_json_enabled()) { LogS("[JSON_UNHANDLED] " << message); break; }
#define LogInvalidJson(message)                  while(glog::Logger::get_log_dump_invalid_json_enabled()) { Log("[JSON_INVALID] " << message); break; }
#define LogInvalidJsonS(message)                 while(glog::Logger::get_log_dump_invalid_json_enabled()) { LogS("[JSON_INVALID] " << message); break; }
#endif // TOOLS_DETACHED_STATE

#define LogIn                                    Log(" > in")
#define LogInS                                   LogS(" > in")

#define LogOut                                   Log(" < out")
#define LogOutS                                  LogS(" < out")


//// Helper to convert types that fmt doesn't handle by default (like enums)
//template <typename T>
//void append_arg_to_buf(fmt::memory_buffer& out, const T& arg)
//{
//    // If it's an enum, cast to underlying integer type (mimics stringstream behavior)
//    if constexpr (std::is_enum_v<T>)
//    {
//        fmt::format_to(std::back_inserter(out), FMT_STRING("{}"), static_cast<std::underlying_type_t<T>>(arg));
//    }
//
//    // If it's a filesystem path, convert to generic string
//    else if constexpr (std::is_same_v<T, std::filesystem::path>)
//    {
//        fmt::format_to(std::back_inserter(out), FMT_STRING("{}"), arg.string());
//    }
//    // Standard types (int, string, float, etc.)
//    else
//    {
//        fmt::format_to(std::back_inserter(out), FMT_STRING("{}"), arg);
//    }
//}
//
//template <typename... Args>
//std::string format_log_msg(Args&&... args)
//{
//    // Use memory_buffer for performance (avoids heap allocations for small strings)
//    fmt::memory_buffer out;
//    std::string_view sep = "";
//
//    // C++17 fold expression to iterate over arguments
//    // Appends separator only after the first element
//    const auto append = [&](auto&& arg)
//    {
//        // separator
//        if (!sep.empty())
//            fmt::format_to(std::back_inserter(out), FMT_STRING("{}"), sep);
//
//        // use the helper to print the argument
//        append_arg_to_buf(out, arg);
//
//        sep = ", ";
//    };
//    (append(std::forward<Args>(args)), ...);
//
//    return fmt::to_string(out);
//}
//#define LogV(...)                    Log(format_log_msg(__VA_ARGS__))
//#define LogVS(...)                   LogS(format_log_msg(__VA_ARGS__))

#ifdef _WIN32
#define LogApiERR                    uint32_t(::GetLastError())
#elif defined __APPLE__
#error Not implemented
#elif defined __linux__
#error Not implemented
#endif

#define LogHEX(h)                    glog::LogHex(h)
#define LogHRES(h)                   glog::LogHRes(uint32_t(h))
#define LogPTR(p)                    glog::LogPtr(p)
