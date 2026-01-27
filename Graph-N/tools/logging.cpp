
#include "core/base/version.h"

namespace glog
{

const char* Logger::m_log_file_mark = "__log_enabled__";
std::atomic_bool Logger::m_enabled = false;

/////////////////////////////////////////////////////////////////

void Logger::cleanup()
{
    auto path = fs::get_log_directory_path();
    auto list = tools::path::enumerate_directory(path, false, false, true, false);

    constexpr int32_t to_save = 5 + 1;
    int32_t to_delete = cast::to_i32(list.size()) - to_save;
    if (to_delete <= 0)
        return;

    const auto sort_callback = [](const std::filesystem::path& lhs, const std::filesystem::path& rhs)
    {
        return lhs.wstring() < rhs.wstring();
    };
    std::sort(list.begin(), list.end(), sort_callback);

    for (const auto& file : list)
    {
        if (file.filename() == m_log_file_mark)
            continue;

        tools::path::remove(file);
        if (--to_delete <= 0)
            break;
    }
}

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
    cleanup();
    if (get_log_enabled())
        return;

    m_enabled = false;
    if (!check_log_file_mark())
        return;

    auto path = fs::get_log_directory_path();
    path /= tools::dt::get_app_start_time().as_filename() + ".log";
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
#error Not implemented
#elif defined __linux__
#error Not implemented
#endif

        m_enabled = true;        
        m_working_thread = std::thread(&Logger::working_thread, this);

        LogWriter writer;
        writer
            << "trylock__\n"
            << m_proc_id << "Log started: " << tools::dt::get_app_start_time().as_log_line() << "\n"
            << m_proc_id << "App version: " << ver::ver_as_string() << "\n"
            << m_proc_id << "Generated client id: " << base::GlobalID::client_id() << "\n"
#ifdef _WIN32
            << m_proc_id << "Command line: " << " " << GetCommandLineA() << "\n"
#elif defined __APPLE__
#error Not implemented
#elif defined __linux__
#error Not implemented
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
    writer << "Log ended: " << tools::dt::from_current_date().as_log_line() << "\n\n";
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
            SAVE_EXCEPTION(ExcepCategory::Logger, e.what());
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

void LogWriter::log_line_start_file(const char * file, int32_t line)
{
    if (nullptr == file)
    {
#ifdef _WIN32
        fmt::format_to(std::back_inserter(m_buffer), FMT_COMPILE("[{}] {} [ln:{}]  "),
            ::GetThreadId(::GetCurrentThread()),
            tools::dt::from_current_date().as_log_line(),
            line);
#elif defined __APPLE__
#error Not implemented
#elif defined __linux__
#error Not implemented
#endif
    }
    else
    {
        std::string sfile;
        static const auto app_dir = fs::get_root_directory_path().string();
        if (strlen(file) > app_dir.size() + 1)  // cut directory/
            file += app_dir.size() + 1;

#ifdef _WIN32
        fmt::format_to(std::back_inserter(m_buffer), FMT_COMPILE("[{}] {} [{}:{}]  "),
            ::GetThreadId(::GetCurrentThread()),
            tools::dt::from_current_date().as_log_line(),
            file, line);
#elif defined __APPLE__
#error Not implemented
#elif defined __linux__
#error Not implemented
#endif
    }
}

void LogWriter::log_line_start_foo(const char * foo, int32_t line)
{
    if (nullptr == foo)
    {
#ifdef _WIN32
        fmt::format_to(std::back_inserter(m_buffer), FMT_COMPILE("[{}] {} [ln:{}]  "),
            ::GetThreadId(::GetCurrentThread()),
            tools::dt::from_current_date().as_log_line(),
            line);
#elif defined __APPLE__
#error Not implemented
#elif defined __linux__
#error Not implemented
#endif
    }
    else
    {
#ifdef _WIN32
        fmt::format_to(std::back_inserter(m_buffer), FMT_COMPILE("[{}] {} [{}:{}]  "),
            ::GetThreadId(::GetCurrentThread()),
            tools::dt::from_current_date().as_log_line(),
            foo, line);
#elif defined __APPLE__
#error Not implemented
#elif defined __linux__
#error Not implemented
#endif
    }
}

LogWriter& LogWriter::operator << (const char* str)
{
    if (str != nullptr)
        fmt::format_to(std::back_inserter(m_buffer), FMT_COMPILE("{}"), str);
    return *this;
}

LogWriter& LogWriter::operator << (const std::string& str)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_COMPILE("{}"), str);
    return *this;
}

LogWriter& LogWriter::operator << (std::string_view str)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_COMPILE("{}"), str.data());
    return *this;
}

LogWriter& LogWriter::operator << (const std::filesystem::path& path)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_COMPILE("{}"), path.string());
    return *this;
}

LogWriter& LogWriter::operator << (const byte_buff& buff)
{
    fmt::format_to(std::back_inserter(m_buffer), "buff sz={}", buff.size());
    return *this;
}

LogWriter& LogWriter::operator << (const tools::dt& dt)
{
    fmt::format_to(std::back_inserter(m_buffer), "time={}", dt.as_full_date());
    return *this;
}

LogWriter& LogWriter::operator << (int32_t i32)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_COMPILE("{}"), i32);
    return *this;
}

LogWriter& LogWriter::operator << (uint32_t ui32)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_COMPILE("{}"), ui32);
    return *this;
}

LogWriter& LogWriter::operator << (int64_t i64)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_COMPILE("{}"), i64);
    return *this;
}

LogWriter& LogWriter::operator << (uint64_t ui64)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_COMPILE("{}"), ui64);
    return *this;
}

LogWriter& LogWriter::operator << (float f)
{
    fmt::format_to(std::back_inserter(m_buffer), "{:.8f}", f);
    return *this;
}

LogWriter& LogWriter::operator << (double d)
{
    fmt::format_to(std::back_inserter(m_buffer), "{:.8f}", d);
    return *this;
}

LogWriter& LogWriter::operator << (bool b)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_COMPILE("{}"), (b ? "yes" : "no"));
    return *this;
}

LogWriter& LogWriter::operator << (DlgCode code)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_COMPILE("{}"), cast::to_u64(code));
    switch (code)
    {
    case DlgCode::Progress:
        fmt::format_to(std::back_inserter(m_buffer), "=progress");
        break;
    case DlgCode::Cancel:
        fmt::format_to(std::back_inserter(m_buffer), "=cancel");
        break;
    default:
        LogError(ExcepCategory::SwitchEnum, "=unknown DlgCode " << cast::to_u64(code));
    }
    return *this;
}

LogWriter& LogWriter::operator << (const type_info& type)
{
    fmt::format_to(std::back_inserter(m_buffer), "{}/{}", type.name(), type.raw_name());
    return *this;
}

LogWriter& LogWriter::operator << (const LogLineEnd&)
{
    fmt::format_to(std::back_inserter(m_buffer), "\n");
    return *this;
}

LogWriter& LogWriter::operator << (const LogHex& hex_item)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_COMPILE("{}"), hex_item.i);
    return *this;
}

LogWriter& LogWriter::operator << (const LogPtr& ptr)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_COMPILE("{}"), ptr.ptr);
    return *this;
}

} // namespace glog

