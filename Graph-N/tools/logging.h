#pragma once

namespace glog
{

class LogWriter;

class Logger final
{
public:
	static Logger* instance(bool first = false);
	static void destroy();

	Logger();
	~Logger();

	void enable_multithreading();
	void disable_multithreading();

	void write(const LogWriter& writer);

	static bool get_log_enabled();
	static void set_log_enabled(bool enable);

private:

	static std::unique_ptr<Logger> Logger::init_log_instance();
	static void destroy_log_instance(std::unique_ptr<Logger>& ptr);

	void init_logger();
	void write_log_tail();
	void close_log();
	bool setup_log_file();
	void write_log_caption();

private:
	static std::unique_ptr<Logger> m_log;
	static const char* m_log_file_mark;

	bool m_enabled = true;
	const std::string m_app_name;
	std::ofstream m_logfs;
	std::string m_log_path;

	std::string m_proc_id;

	bool m_multithreading_enabled = false;
	std::atomic_int m_multithreading_counter = 0;

	std::mutex m_lock;
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
	LogWriter() { }
	~LogWriter() { }

	const std::stringstream& get_buffer() const { return m_buffer; }

	void log_line_start_file(const char * file, int line);
	void log_line_start_foo(const char * file, int line);

	LogWriter& operator << (const char * str);
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
	std::stringstream m_buffer;
};

struct LogGuard final
{
	LogGuard()
	{
		Logger::instance(true /*first*/);
	}
	~LogGuard()
	{
		Logger::destroy();
	}
};

struct LogLockGuard final
{
	LogLockGuard()
	{
		auto logger = glog::Logger::instance();
		if (logger)
			logger->enable_multithreading();
	}
	~LogLockGuard()
	{
		auto logger = glog::Logger::instance();
		if (logger)
			logger->disable_multithreading();
	}
};

} // namespace glog

////////////////////////////////////

#define WIDELINE2(x) L ## x
#define WIDELINE(x) WIDELINE2(x)
#define __WIDEFILE__ WIDELINE(__FILE__)

#define Log(message)														\
	{																		\
		glog::LogWriter* log_writer__ = nullptr;							\
		try {																\
			auto logger__ = glog::Logger::instance();						\
			if (logger__) {													\
				log_writer__ = new glog::LogWriter;							\
				log_writer__->log_line_start_foo(__FUNCTION__, __LINE__);	\
				(*log_writer__) << message << glog::LogLineEnd();			\
				logger__->write(*log_writer__);								\
			}																\
		} catch (...) { }													\
		delete log_writer__;												\
	}

#define LogWarn(message)			Log("[WARN] " << message)
#define LogInternalWarn(message)	Log("[INTERNAL_LOGIC_ERROR] " << message)
#define LogUnhandledJson(message)	Log("[JSON_UNHANDLED] " << message)
#define LogInvalidJson(message)		Log("[JSON_INVALID] " << message)

#define LogF						Log("")
#define Log2(p1, p2)				Log(p1 << ", " << p2)
#define Log3(p1, p2, p3)			Log(p1 << ", " << p2 << ", " << p3)
#define Log4(p1, p2, p3, p4)		Log(p1 << ", " << p2 << ", " << p3 << ", " << p4)
#define Log5(p1, p2, p3, p4, p5)	Log(p1 << ", " << p2 << ", " << p3 << ", " << p4 << ", " << p5)

#ifdef _WIN32
#define LgERR						((uint32_t)GetLastError())
#endif
#define LgHEX(h)					glog::LogHex(h)
#define LgPTR(p)					glog::LogPtr(p)

#define LogExcep(message)			Log("[EXCEP] " << message)
#define LogAndThrow(message)											\
	{																	\
		LogExcep(message);												\
		const std::string location__ = std::string(__FILE__) + std::string("\r\n") + std::string(__FUNCTION__) + std::string(": ") + std::to_string(__LINE__);	\
		LogExcep(location__);											\
		auto ex_log_writer__ = new glog::LogWriter;						\
		(*ex_log_writer__) << message;									\
		const auto ex_msg__ = ex_log_writer__->get_buffer().str();		\
		delete ex_log_writer__;											\
		throw std::runtime_error(ex_msg__);								\
	}

#define LogAndThrowInternalErr(message)									\
	{																	\
		LogInternalWarn(message);												\
		const std::string location__ = std::string(__FILE__) + std::string("\r\n") + std::string(__FUNCTION__) + std::string(": ") + std::to_string(__LINE__);	\
		LogInternalWarn(location__);											\
		auto ex_log_writer__ = new glog::LogWriter;						\
		(*ex_log_writer__) << message;									\
		const auto ex_msg__ = ex_log_writer__->get_buffer().str();		\
		delete ex_log_writer__;											\
		throw std::runtime_error(ex_msg__);								\
	}

#define LogExcepRe(message)			Log("[EXCEP-RE] " << message)
#define LogAndReThrow(message)											\
	{																	\
		LogExcepRe(message);											\
		const std::string location__ = std::string(__FILE__) + std::string("\r\n") + std::string(__FUNCTION__) + std::string(": ") + std::to_string(__LINE__);	\
		LogExcepRe(location__);											\
		throw;															\
	}
