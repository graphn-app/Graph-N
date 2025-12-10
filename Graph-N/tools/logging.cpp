
#include "core/base/version.h"

namespace glog
{

std::unique_ptr<Logger> Logger::m_log = nullptr;
const char* Logger::m_log_file_mark = "__log_enabled__";

/////////////////////////////////////////////////////////////////

bool Logger::get_log_enabled()
{
	auto path = fs::get_log_directory_path();
	path /= m_log_file_mark;
	return tools::path::exists(path);
}

void Logger::set_log_enabled(bool enable)
{
	if (get_log_enabled() == enable)
		return;

	auto path = fs::get_log_directory_path();
	path /= m_log_file_mark;

	if (!enable)
	{
		destroy();
		tools::path::remove(path);
	}
	else
	{
		tools::path::make_file_path(path, true /*overwrite*/);
		instance(true /*first*/);
	}
}

Logger* Logger::instance(bool first /*= false*/)
{
	if (!m_log)
	{
		if (first)
			m_log = init_log_instance();
		else
			return nullptr;
	}
	return m_log.get();
}

std::unique_ptr<Logger> Logger::init_log_instance()
{
	if (!get_log_enabled())
		return nullptr;
	auto log = std::make_unique<Logger>();
	log->init_logger();
	return log;
}

void Logger::destroy()
{
	destroy_log_instance(m_log);
}

void Logger::destroy_log_instance(std::unique_ptr<Logger>& ptr)
{
	if (ptr)
	{
		ptr->write_log_tail();
		ptr->m_enabled = false;
	}
	ptr.reset();
}

/////////////////////////////////////////////////////////////////

Logger::Logger()
	: m_app_name(APP_CAPTION)
{
	m_proc_id = "[" + std::to_string(GetCurrentProcessId()) + "] ";
}

Logger::~Logger()
{
	close_log();
}

void Logger::close_log()
{
	m_enabled = false;
	m_logfs.close();
}

void Logger::init_logger()
{
	m_enabled = false;
	if (!setup_log_file())
	{
		return;
	}
	write_log_caption();
	m_enabled = true;
}

void Logger::enable_multithreading()
{
	++m_multithreading_counter;
	m_multithreading_enabled = true;
	Log("Multithreading enabled");
}

void Logger::disable_multithreading()
{
	if (--m_multithreading_counter <= 0)
	{
		m_multithreading_counter = 0;
		m_multithreading_enabled = false;
		Log("Multithreading disabled");
	}
}

bool Logger::setup_log_file()
{
	auto path = fs::get_log_directory_path();
	path /= tt::dt::from_current_date().as_filename() + ".log";
	m_log_path = path.string();

	// _tzset();

	if (!tools::path::exists(m_log_path))
	{
		std::ofstream unilog;
		unilog.open(m_log_path.c_str(), std::ios::out | std::ios::binary);

		unilog << (BYTE)0xEF << (BYTE)0xBB << (BYTE)0xBF;
		unilog.flush();
		unilog.close();
	}

	m_logfs.open(m_log_path.c_str(), std::ios::out | std::ios::app);
	if (m_logfs.is_open() && !m_logfs.bad() && tools::path::exists(m_log_path))
	{
		m_logfs << std::endl << std::endl << std::endl;
		return true;
	}
	return false;
}

void Logger::write_log_caption()
{
	// TODO: VRAM, GetCommandLineA - win32
//	MEMORYSTATUS mem = {};
//	GlobalMemoryStatus(&mem);

	m_logfs
		<< m_proc_id << "Log started: " << tt::dt::from_current_date().as_string(true /*add_day*/, true /*add_time*/) << std::endl
		<< m_proc_id << "App version: " << ver::ver_as_string() << std::endl
		<< m_proc_id << "Command line: " << " " << GetCommandLineA() << std::endl
	;
}

void Logger::write_log_tail()
{
	if (m_logfs.is_open() && !m_logfs.bad())
	{
		m_logfs << m_proc_id << "Log ended: " << tt::dt::from_current_date().as_string(true /*add_day*/, true /*add_time*/) << std::endl << std::endl;
	}
}

void Logger::write(const LogWriter& writer)
{
	if (!m_enabled)
		return;

	std::string buffer = writer.get_buffer().str();

	if (m_multithreading_enabled)
	{
		std::lock_guard<std::mutex> l(m_lock);
		if (m_logfs.is_open() && !m_logfs.bad())
		{
			m_logfs << m_proc_id << buffer;
			m_logfs.flush();
		}
	}
	else
	{
		if (m_logfs.is_open() && !m_logfs.bad())
		{
			m_logfs << m_proc_id << buffer;
			m_logfs.flush();
		}
	}

#ifdef _WIN32
	OutputDebugStringA((m_app_name + "__: " + buffer).c_str());
#endif
}

////////////////////////////////////////////////////
///////////////		LogWriter

void LogWriter::log_line_start_file(const char * file, int line)
{
	if (nullptr == file)
	{
		m_buffer << tt::dt::from_current_date().as_string(false /*add_day*/, true /*add_time*/) << " ln:" << line << "\t>> ";
	}
	else
	{
		std::string sfile;
		// TODO
		auto cut = strstr(file, "\\Graph-N\\");
		if (cut != nullptr)
			sfile = ++cut;

		m_buffer << tt::dt::from_current_date().as_string(false/*add_day*/, true /*add_time*/) << " " << sfile << ":" << line << "\t>> ";
	}
}

void LogWriter::log_line_start_foo(const char * foo, int line)
{
	if (nullptr == foo)
	{
		m_buffer << tt::dt::from_current_date().as_string(false/*add_day*/, true /*add_time*/) << " ln:" << line << "\t>> ";
	}
	else
	{
		m_buffer << tt::dt::from_current_date().as_string(false/*add_day*/, true /*add_time*/) << " " << foo << ":" << line << "\t>> ";
	}
}

LogWriter& LogWriter::operator << (const char * str)
{
	m_buffer << str;
	return *this;
}

LogWriter& LogWriter::operator << (const std::string& str)
{
	m_buffer << str;
	return *this;
}

LogWriter& LogWriter::operator << (const std::stringstream& str)
{
	m_buffer << str.str();
	return *this;
}

LogWriter& LogWriter::operator << (std::string_view str)
{
	m_buffer << str;
	return *this;
}

LogWriter& LogWriter::operator << (const std::filesystem::path& path)
{
	m_buffer << path.string();
	return *this;
}

LogWriter& LogWriter::operator << (const byte_buff& buff)
{
	m_buffer << "buffer(" << buff.size() << ")";
	return *this;
}

LogWriter& LogWriter::operator << (const tools::time::dt& dt)
{
	m_buffer << "time(" << dt.as_string(true/*add_day*/, true /*add_time*/, true /*month_as_name*/) << ")";
	return *this;
}

LogWriter& LogWriter::operator << (int32_t i32)
{
	m_buffer << i32;
	return *this;
}

LogWriter& LogWriter::operator << (uint32_t ui32)
{
	m_buffer << ui32;
	return *this;
}

LogWriter& LogWriter::operator << (int64_t i64)
{
	m_buffer << i64;
	return *this;
}

LogWriter& LogWriter::operator << (uint64_t ui64)
{
	m_buffer << ui64;
	return *this;
}

LogWriter& LogWriter::operator << (float f)
{
	m_buffer << f;
	return *this;
}

LogWriter& LogWriter::operator << (bool b)
{
	m_buffer << (b ? "yes" : "no");
	return *this;
}

template <typename CONT>
void log_cont(const CONT& cont, std::stringstream& buffer)
{
	buffer << "{ ";
	for (auto id : cont)
	{
		buffer << id << ", ";
	}
	buffer << "}";
}

template <typename MAP>
void log_map(const MAP& cont, std::stringstream& buffer)
{
	buffer << "{ ";
	for (auto& pair : cont)
	{
		buffer << "{ " << pair.first << ", " << pair.second << " }, ";
	}
	buffer << "}";
}

LogWriter& LogWriter::operator << (DlgCode code)
{
	m_buffer << cast::to_u16(code);
	switch (code)
	{
	case DlgCode::progress:
		m_buffer << "=progress";
		break;
	case DlgCode::ok:
		m_buffer << "=ok";
		break;
	case DlgCode::yes:
		m_buffer << "=yes";
		break;
	case DlgCode::no:
		m_buffer << "=no";
		break;
	case DlgCode::cancel:
		m_buffer << "=cancel";
		break;
	default:
		LogWarn("=unknown DlgCode " << cast::to_u32(code));
	}
	return *this;
}

LogWriter& LogWriter::operator << (DlgObject dlg)
{
	m_buffer << cast::to_u16(dlg);
	switch (dlg)
	{
	case DlgObject::GraphN_Flow:
		m_buffer << "=GraphN_Flow";
		break;
	default:
		LogWarn("=unknown DlgObject " << cast::to_u32(dlg));
	}
	return *this;
}

LogWriter& LogWriter::operator << (const type_info& type)
{
	m_buffer << type.name() << "/" << type.raw_name();
	return *this;
}

LogWriter& LogWriter::operator << (const LogLineEnd&)
{
	m_buffer << std::endl;
	return *this;
}

LogWriter& LogWriter::operator << (const LogHex& hex_item)
{
	m_buffer << std::hex << "0x" << hex_item.i << std::dec;
	return *this;
}

LogWriter& LogWriter::operator << (const LogPtr& ptr)
{
	m_buffer << std::hex << "0x" << ptr.ptr << std::dec;
	return *this;
}

} // namespace glog

