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
	LogWriter& operator << (const std::stringstream& str);
	LogWriter& operator << (std::string_view str);
	LogWriter& operator << (const std::filesystem::path& path);

	LogWriter& operator << (const byte_buff& buff);
	LogWriter& operator << (const tools::time::dt& dt);

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
    template<typename T>
    std::string& append_int(T val)
    {
        char buff[64] = {};
        auto[ptr, ec] = std::to_chars(buff, buff + sizeof(buff), val);
        if (ec == std::errc())
        {
            m_buffer.append(buff, ptr - buff);
        }
        return m_buffer;
    }

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
#define DEBUG_CATCH \
    { \
        switch (::MessageBoxA(nullptr, \
            "Some error happened\nTo exit app - press 'Abort'\nAttach debugger and press 'Retry' to debug app\nTo continue execution press 'Ignore')", \
            APP_CAPTION, MB_ABORTRETRYIGNORE | MB_ICONWARNING | MB_TASKMODAL | MB_TOPMOST)) \
        { \
        case IDABORT: \
            ::exit(1); \
            break; \
        case IDRETRY: \
            __debugbreak(); \
            break; \
        } \
    }

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
            SAVE_EXCEPTION(ExcepCategory::logger, e.what()); \
            glog::Logger::instance().disable_logging(); \
            DEBUG_CATCH; \
        } \
        catch (...) { \
            SAVE_EXCEPTION(ExcepCategory::logger, "Unknown exception"); \
            glog::Logger::instance().disable_logging(); \
            DEBUG_CATCH; \
        } \
	}

#define LOG_SOMETHING_HAPPENED(category, log_prefix, message, do_throw) \
    { \
        glog::LogWriter log_writer_1__; \
        log_writer_1__ << message; \
        const std::string& buffer = log_writer_1__.get_buffer(); \
        SAVE_EXCEPTION(category, buffer); \
	    if (glog::Logger::get_log_enabled()) \
        { \
            try { \
                glog::LogWriter log_writer_2__; \
			    log_writer_2__.log_line_start_foo(__FUNCTION__, __LINE__); \
			    log_writer_2__ << "[" << log_prefix << "] " << buffer << glog::LogLineEnd(); \
                glog::Logger::instance().write(log_writer_2__); \
            } \
            catch (const std::exception& e) { \
                SAVE_EXCEPTION(ExcepCategory::logger, e.what()); \
                glog::Logger::instance().disable_logging(); \
                DEBUG_CATCH; \
            } \
            catch (...) { \
                SAVE_EXCEPTION(ExcepCategory::logger, "Unknown exception"); \
                glog::Logger::instance().disable_logging(); \
                DEBUG_CATCH; \
            } \
	    } \
        DEBUG_CATCH; \
        if (do_throw) \
        { \
            throw std::runtime_error(buffer); \
        } \
    }


#define LogExcep(category, message)                 LOG_SOMETHING_HAPPENED(category, "EXCEPTION", message, false)
#define LogError(category, message)                 LOG_SOMETHING_HAPPENED(category, "ERROR", message, false)
#define LogInternalError(category, message)         LOG_SOMETHING_HAPPENED(category, "INTERNAL_LOGIC_ERROR", message, false)
#define LogUnhandledJson(message)	                Log("[JSON_UNHANDLED] " << message)
#define LogInvalidJson(message)		                Log("[JSON_INVALID] " << message)

#define LogAndThrow(category, message)              LOG_SOMETHING_HAPPENED(category, "EXCEPTION", message, true)
#define LogAndThrowInternalError(category, message) LOG_SOMETHING_HAPPENED(category, "INTERNAL_LOGIC_ERROR", message, true)

#define LogF						Log("")

template <typename Arg>
void format_helper(std::stringstream& ss, Arg&& arg)
{
    ss << std::forward<Arg>(arg);
}

template <typename Arg, typename... Args>
void format_helper(std::stringstream& ss, Arg&& arg, Args&&... args)
{
    ss << std::forward<Arg>(arg) << ", ";
    format_helper(ss, std::forward<Args>(args)...);
}

template <typename... Args>
std::string format_log_msg(Args&&... args)
{
    std::stringstream ss;
    format_helper(ss, std::forward<Args>(args)...);
    return ss.str();
}

#define LogV(...)				    Log(format_log_msg(__VA_ARGS__))

#ifdef _WIN32
#define LogERR						uint32_t(GetLastError())
#else
#define LogERR                      uint32_t(0)
#endif
#define LogHEX(h)					glog::LogHex(h)
#define LogPTR(p)					glog::LogPtr(p)

