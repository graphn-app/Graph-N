#pragma once

namespace glog
{

class Logger final
{
    friend class LogWriter;

public:
	static Logger& instance();
    void stop();

	static bool get_log_enabled();
	static void set_log_enabled(bool enable);
    static void disable_logging() { m_enabled = false; }

    void force_flush();
    void write(LogWriter& writer);

private:
    Logger();
    ~Logger();
    NO_COPY_MOVE(Logger);

    static void cleanup();
    static bool check_log_file_mark();

	void log_start();
	void log_stop();

    void working_thread();

private:
	static const char* m_log_file_mark;
    static std::atomic_bool m_enabled;

	std::ofstream m_logfs;
	std::string m_log_path;
	std::string m_proc_id;

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
///////////////		LogWriter

class LogLineEnd { };

struct LogHex final
{
	long long i = 0;
	LogHex(long long i_) :
		i(i_)
	{ }
};

struct LogPtr final
{
	void* ptr = nullptr;
	LogPtr(void* ptr_) :
		ptr(ptr_)
	{ }
};

class LogWriter final
{ 
public:
    LogWriter();
    ~LogWriter() = default;

    NO_COPY_MOVE(LogWriter);

	std::string extract_buffer() { return std::move(m_buffer); }
    const std::string& get_buffer() const { return std::move(m_buffer); }

	void log_line_start_file(const char* file, int line);
	void log_line_start_foo(const char* foo, int line);

	LogWriter& operator << (const char* str);
	LogWriter& operator << (const std::string& str);
	LogWriter& operator << (std::string_view str);
	LogWriter& operator << (const std::filesystem::path& path);

	LogWriter& operator << (const byte_buff& buff);
	LogWriter& operator << (const tools::dt& dt);

	LogWriter& operator << (int32_t i32);
	LogWriter& operator << (uint32_t ui32);
	LogWriter& operator << (int64_t i64);
	LogWriter& operator << (uint64_t ui64);
	LogWriter& operator << (float f);
	LogWriter& operator << (bool b);

	LogWriter& operator << (DlgCode code);

	LogWriter& operator << (const type_info& type);

	LogWriter& operator << (const LogLineEnd&);
	LogWriter& operator << (const LogHex& hex_item);
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

#ifdef _DEBUG
// #define SAVE_EXCEPTION(category, message)     { (void)category; void(message); }
#define SAVE_EXCEPTION(category, message)     g::save_exception(category, message, __FILE__, __LINE__)

#else
#define SAVE_EXCEPTION(category, message)     g::save_exception(category, message, __FILE__, __LINE__)
#define DEBUG_CATCH
#endif


#define Log(message) \
	if (glog::Logger::get_log_enabled()) \
    { \
        try { \
            glog::LogWriter log_writer__; \
			log_writer__.log_line_start_foo(__FUNCTION__, __LINE__); \
			log_writer__ << message << glog::LogLineEnd(); \
            glog::Logger::instance().write(log_writer__); \
        } \
        catch (const std::exception& e) { \
            SAVE_EXCEPTION(ExcepCategory::Logger, e.what()); \
            glog::Logger::instance().disable_logging(); \
            DEBUG_CATCH; \
        } \
        catch (...) { \
            SAVE_EXCEPTION(ExcepCategory::Logger, "Unknown exception"); \
            glog::Logger::instance().disable_logging(); \
            DEBUG_CATCH; \
        } \
	}

#define LOG_SOMETHING_HAPPENED(category, log_prefix, message, do_throw) \
    { \
        glog::LogWriter log_writer_1__; \
        log_writer_1__ << message; \
        const std::string& buffer__ = log_writer_1__.get_buffer(); \
        SAVE_EXCEPTION(category, buffer__); \
	    if (glog::Logger::get_log_enabled()) \
        { \
            try { \
                glog::LogWriter log_writer_2__; \
			    log_writer_2__.log_line_start_foo(__FUNCTION__, __LINE__); \
			    log_writer_2__ << "[" << log_prefix << "] " << buffer__ << glog::LogLineEnd(); \
                glog::Logger::instance().write(log_writer_2__); \
            } \
            catch (const std::exception& e) { \
                SAVE_EXCEPTION(ExcepCategory::Logger, e.what()); \
                glog::Logger::instance().disable_logging(); \
                DEBUG_CATCH; \
            } \
            catch (...) { \
                SAVE_EXCEPTION(ExcepCategory::Logger, "Unknown exception"); \
                glog::Logger::instance().disable_logging(); \
                DEBUG_CATCH; \
            } \
	    } \
        DEBUG_CATCH; \
        if (do_throw) \
        { \
            throw std::runtime_error(buffer__); \
        } \
    }


#define LogExcep(category, message)                 LOG_SOMETHING_HAPPENED(category, "EXCEPTION", message, false)
#define LogError(category, message)                 LOG_SOMETHING_HAPPENED(category, "ERROR", message, false)
#define LogInternalError(category, message)         LOG_SOMETHING_HAPPENED(category, "INTERNAL_LOGIC_ERROR", message, false)
#define LogUnhandledJson(message)	                Log("[JSON_UNHANDLED] " << message)
#define LogInvalidJson(message)		                Log("[JSON_INVALID] " << message)

#define LogAndThrow(category, message)              LOG_SOMETHING_HAPPENED(category, "EXCEPTION", message, true)
#define LogAndThrowInternalError(category, message) LOG_SOMETHING_HAPPENED(category, "INTERNAL_LOGIC_ERROR", message, true)

#define LogFS						Log("")
#define LogFS_In					Log(" > in")
#define LogFS_Out    				Log(" < out")

#define LogF						Log(glog::LogPtr(this))
#define LogF_In						Log(glog::LogPtr(this) << " > in")
#define LogF_Out    				Log(glog::LogPtr(this) << " < out")


// Helper to convert types that fmt doesn't handle by default (like enums)
template <typename T>
void append_arg_to_buf(fmt::memory_buffer& out, const T& arg)
{
    // If it's an enum, cast to underlying integer type (mimics stringstream behavior)
    if constexpr (std::is_enum_v<T>)
    {
        fmt::format_to(std::back_inserter(out), FMT_COMPILE("{}"), static_cast<std::underlying_type_t<T>>(arg));
    }

    // If it's a filesystem path, convert to generic string
    else if constexpr (std::is_same_v<T, std::filesystem::path>)
    {
        fmt::format_to(std::back_inserter(out), FMT_COMPILE("{}"), arg.string());
    }
    // Standard types (int, string, float, etc.)
    else
    {
        fmt::format_to(std::back_inserter(out), FMT_COMPILE("{}"), arg);
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
            fmt::format_to(std::back_inserter(out), FMT_COMPILE("{}"), sep);

        // use the helper to print the argument
        append_arg_to_buf(out, arg);

        sep = ", ";
    };
    (append(std::forward<Args>(args)), ...);

    return fmt::to_string(out);
}
#define LogV(...)				    Log(format_log_msg(__VA_ARGS__))

#ifdef _WIN32
#define LogERR						uint32_t(GetLastError())
#elif defined __APPLE__
#error Not implemented
#elif defined __linux__
#error Not implemented
#endif
#define LogHEX(h)					glog::LogHex(h)
#define LogPTR(p)					glog::LogPtr(p)

