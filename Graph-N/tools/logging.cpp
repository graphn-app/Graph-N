
#include "core/base/version.h"

namespace glog
{

const char* Logger::m_log_file_mark = "__log_enabled__";
std::atomic_bool Logger::m_enabled = false;

/////////////////////////////////////////////////////////////////

bool Logger::check_log_file_mark()
{
    auto path = fs::get_log_directory_path();
    path /= m_log_file_mark;
    return tools::path::exists(path);
}

bool Logger::get_log_enabled()
{
    return m_enabled;
}

void Logger::set_log_enabled(bool enable)
{
    if (get_log_enabled() == enable)
        return;

    auto path = fs::get_log_directory_path();
    path /= m_log_file_mark;

    if (enable)
    {
        tools::path::make_file_path(path, true /*overwrite*/);
        instance().log_start();
    }
    else
    {
        instance().log_stop();
        tools::path::remove(path);
    }
}

Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

void Logger::stop()
{
    log_stop();
}

Logger::Logger()
{
    log_start();
}

Logger::~Logger()
{
    log_stop();
}

void Logger::force_flush()
{
    m_do_flush = true;
    m_cv.notify_one();
}

void Logger::log_start()
{
    if (get_log_enabled())
        return;

    m_enabled = false;
    if (!check_log_file_mark())
        return;

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
#ifdef _WIN32
        m_proc_id = "[" + std::to_string(GetCurrentProcessId()) + "] ";
#elif defined __APPLE__
        m_proc_id = "[" + ??? + "] ";
#elif defined __linux__
        m_proc_id = "[" + ??? + "] ";
#endif

        m_enabled = true;        
        m_working_thread = std::thread(&Logger::working_thread, this);

        LogWriter writer;
        writer
            << m_proc_id << "Log started: " << tt::dt::from_current_date().as_string(true /*add_day*/, true /*add_time*/) << "\n"
            << m_proc_id << "App version: " << ver::ver_as_string() << "\n"
#ifdef _WIN32
            << m_proc_id << "Command line: " << " " << GetCommandLineA() << "\n"
#elif defined __APPLE__
            << m_proc_id << "Command line: " << " " << ??? << "\n"
#elif defined __linux__
            << m_proc_id << "Command line: " << " " << ??? << "\n"
#endif
            << "\n"
        ;

        write(writer);
        force_flush();
    }
}

void Logger::log_stop()
{
    if (!get_log_enabled())
        return;

    LogWriter writer;
    writer << "Log ended: " << tt::dt::from_current_date().as_string(true /*add_day*/, true /*add_time*/) << "\n\n";
    write(writer);

    m_do_close = true;
    m_cv.notify_one();

    if (m_working_thread.joinable())
        m_working_thread.join();

    m_enabled = false;
    if (m_logfs.is_open() && !m_logfs.bad())
        m_logfs.close();
}

void Logger::write(LogWriter& writer)
{
    if (!get_log_enabled())
        return;

    auto buffer = writer.extract_buffer();

#if defined _WIN32 && defined _DEBUG
    OutputDebugStringA((APP_CAPTION "__: " + buffer).c_str());
#endif

    std::lock_guard<std::mutex> lock(m_lock);
    m_buffer.emplace(std::move(buffer));
    if (++m_buffer_size >= m_overflow_limit)
        m_cv.notify_one();
}

void Logger::working_thread()
{
    if (!get_log_enabled())
        return;

    std::queue<std::string> temp_buffer;

    while (get_log_enabled() && !m_do_close)
    {
        try
        {
            {
                std::unique_lock<std::mutex> lock(m_lock);
                m_cv.wait_for(lock, std::chrono::milliseconds(m_flush_timeout), [this] {
                    return m_do_flush || m_do_close || m_buffer_size >= m_overflow_limit;
                });

                m_do_flush = false;
                if (0 == m_buffer_size && !m_do_close)    // spurious wakeup or flush on empty buffer
                    continue;
            
                temp_buffer.swap(m_buffer);
                m_buffer_size = 0;
            }

            if (!temp_buffer.empty() && m_logfs.is_open())
            {
                while (!temp_buffer.empty())
                {
                    m_logfs << m_proc_id << temp_buffer.front();
                    temp_buffer.pop();
                }

                m_logfs.flush();
            }

        }
        catch (const std::exception& e)
        {
            disable_logging();
            SAVE_EXCEPTION(ExcepCategory::logger, e.what());
            DEBUG_CATCH;
        }
    }
}

////////////////////////////////////////////////////
///////////////		LogWriter

LogWriter::LogWriter()
{
    m_buffer.reserve(512);
}

void LogWriter::log_line_start_file(const char * file, int line)
{
    if (nullptr == file)
    {
        m_buffer.
            append(tt::dt::from_current_date().as_string(false /*add_day*/, true /*add_time*/)).
            append(" ln:");
        append_int(line).
            append("\t>> ");
    }
    else
    {
        std::string sfile;
        static const auto app_dir = fs::get_root_directory_path().string();
        if (strlen(file) > app_dir.size() + 1)  // cut directory/
            file += app_dir.size() + 1;

        m_buffer.
            append(tt::dt::from_current_date().as_string(false /*add_day*/, true /*add_time*/)).
            append(" ").
            append(file).
            append(":");
        append_int(line).
            append("\t>> ");
    }
}

void LogWriter::log_line_start_foo(const char * foo, int line)
{
    if (nullptr == foo)
    {
        m_buffer.
            append(tt::dt::from_current_date().as_string(false /*add_day*/, true /*add_time*/)).
            append(" ln:");
        append_int(line).
            append("\t>> ");
    }
    else
    {
        m_buffer.
            append(tt::dt::from_current_date().as_string(false /*add_day*/, true /*add_time*/)).
            append(" ").
            append(foo).
            append(":");
        append_int(line).
            append("\t>> ");
    }
}

LogWriter& LogWriter::operator << (const char* str)
{
    if (str != nullptr)
        m_buffer.append(str);
    return *this;
}

LogWriter& LogWriter::operator << (const std::string& str)
{
    m_buffer.append(str);
    return *this;
}

LogWriter& LogWriter::operator << (const std::stringstream& str)
{
    m_buffer.append(str.str());
    return *this;
}

LogWriter& LogWriter::operator << (std::string_view str)
{
    m_buffer.append(str);
    return *this;
}

LogWriter& LogWriter::operator << (const std::filesystem::path& path)
{
    m_buffer.append(path.string());
    return *this;
}

LogWriter& LogWriter::operator << (const byte_buff& buff)
{
    m_buffer.append("buff sz=");
    append_int(buff.size());
    return *this;
}

LogWriter& LogWriter::operator << (const tools::time::dt& dt)
{
    m_buffer.
        append("time=").
        append(dt.as_string(true/*add_day*/, true /*add_time*/, true /*month_as_name*/));
    return *this;
}

LogWriter& LogWriter::operator << (int32_t i32)
{
    append_int(i32);
    return *this;
}

LogWriter& LogWriter::operator << (uint32_t ui32)
{
    append_int(ui32);
    return *this;
}

LogWriter& LogWriter::operator << (int64_t i64)
{
    append_int(i64);
    return *this;
}

LogWriter& LogWriter::operator << (uint64_t ui64)
{
    append_int(ui64);
    return *this;
}

LogWriter& LogWriter::operator << (float f)
{
    char buff[64] = {};
    const int len = snprintf(buff, sizeof(buff), "%.8f", f);
    m_buffer.append(buff, len);
    return *this;
}

LogWriter& LogWriter::operator << (bool b)
{
    m_buffer.append(b ? "yes" : "no");
    return *this;
}

LogWriter& LogWriter::operator << (DlgCode code)
{
    append_int(cast::to_u16(code));
    switch (code)
    {
    case DlgCode::progress:
        m_buffer.append("=progress");
        break;
    case DlgCode::ok:
        m_buffer.append("=ok");
        break;
    case DlgCode::yes:
        m_buffer.append("=yes");
        break;
    case DlgCode::no:
        m_buffer.append("=no");
        break;
    case DlgCode::cancel:
        m_buffer.append("=cancel");
        break;
    default:
        LogError(ExcepCategory::switch_enum, "=unknown DlgCode " << cast::to_u32(code));
    }
    return *this;
}

LogWriter& LogWriter::operator << (const type_info& type)
{
    m_buffer.append(type.name()).append("/").append(type.raw_name());
    return *this;
}

LogWriter& LogWriter::operator << (const LogLineEnd&)
{
    m_buffer.append("\n");
    return *this;
}

LogWriter& LogWriter::operator << (const LogHex& hex_item)
{
    char buff[32] = {};
    int len = snprintf(buff, sizeof(buff), "0x%llX", hex_item.i);
    m_buffer.append(buff, len);
    return *this;
}

LogWriter& LogWriter::operator << (const LogPtr& ptr)
{
    char buff[32] = {};
    int len = snprintf(buff, sizeof(buff), "%p", ptr.ptr);
    m_buffer.append(buff, len);
    return *this;
}

} // namespace glog

