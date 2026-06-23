
#include "core/base/version.h"
#include "core/managers/db_manager.h"

namespace glog
{

const char* Logger::m_log_file_mark = "__log_enabled__";
std::atomic_bool Logger::m_enabled = false;

/////////////////////////////////////////////////////////////////

static void cleanup(const std::filesystem::path& dir, const std::string& prefix, size_t num)
{
    const auto sort_callback = [](const std::filesystem::path& lhs, const std::filesystem::path& rhs)
    {
        return lhs.u8string() > rhs.u8string();
    };

    auto list = tools::path::enumerate_directory(dir, false, false, true, false);
    std::sort(list.begin(), list.end(), sort_callback);

    std::list<std::filesystem::path> filtered;
    for (auto& file : list)
    {
        if (file.filename().u8string().find(prefix) == 0)
        {
            filtered.push_back(std::move(file));
        }
    }

    if (filtered.size() <= num + 1)
        return;

    size_t to_delete = filtered.size() - (num + 1);
    for (auto it = filtered.rbegin(); it != filtered.rend(); ++it)
    {
        if (0 == to_delete)
            break;
        
        tools::path::remove(*it);
        to_delete--;
    }
}

void Logger::cleanup()
{
    const auto path = fs::get_log_directory_path();
    glog::cleanup(path, APP_CAPTION, 3);
    glog::cleanup(path, APP_COMFYUI, 3);
}

bool Logger::check_log_file_mark()
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

    if (enable)
    {
        tools::path::make_file_path(path, true /*overwrite*/, "");
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
    m_do_flush.store(true, std::memory_order_relaxed);
    m_cv.notify_one();
}

void Logger::log_start()
{
    if (get_log_enabled())
        return;

    m_enabled.store(false, std::memory_order_relaxed);
    if (!check_log_file_mark())
        return;

    auto path = fs::get_log_directory_path();
    path /= APP_CAPTION "_" + tools::dt::get_app_start_time().as_filename() + ".log";
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
        m_enabled.store(true, std::memory_order_relaxed);
        m_working_thread = std::thread(&Logger::working_thread, this);

        const auto db_meta = mngr::DbManager::read_db_metadata();

        LogWriter writer;
        writer
            << "\n\n"
            << "Log started: " << tools::dt::get_app_start_time().as_log_line() << "\n"
            << "App version: " << ver::ver_as_string() << "\n"
            << "Db meta: "
        ;
        if (db_meta.db_ver != 0)
            writer << "db ver " << db_meta.db_ver << ", app ver " << db_meta.app_ver << ", last run " << db_meta.last_run << "\n";
        else
            writer << "\n";
        writer
            << "Generated client id: " << base::GlobalID::client_id() << "\n"
#ifdef _WIN32
            << "Process ID: " << cast::to_u32(::GetCurrentProcessId()) << "\n"
            << "Main thread ID: " << cast::to_u32(::GetCurrentThreadId()) << "\n"
            << "Command line: " << " " << ::GetCommandLineA() << "\n"
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

    m_do_close.store(true, std::memory_order_relaxed);
    m_cv.notify_one();

    if (m_working_thread.joinable())
        m_working_thread.join();

    m_enabled.store(false, std::memory_order_relaxed);
    if (m_logfs.is_open() && !m_logfs.bad())
        m_logfs.close();
}

void Logger::write(LogWriter& writer)
{
    if (!get_log_enabled())
        return;

    auto buffer = writer.extract_buffer();

#if defined _WIN32 && defined _DEBUG
    ::OutputDebugStringA((APP_CAPTION "__: " + buffer).c_str());
#endif

    std::lock_guard<std::mutex> lock(m_lock);
    m_buffer.emplace(std::move(buffer));
    if (m_buffer_size.fetch_add(1, std::memory_order_relaxed) + 1 >= m_overflow_limit)
        m_cv.notify_one();
}

void Logger::working_thread()
{
    if (!get_log_enabled())
        return;

    std::queue<std::string> temp_buffer;

    while (get_log_enabled() && !m_do_close.load(std::memory_order_relaxed))
    {
        try
        {
            {
                std::unique_lock<std::mutex> lock(m_lock);
                m_cv.wait_for(lock, std::chrono::milliseconds(m_flush_timeout), [this] {
                    return
                        m_do_flush.load(std::memory_order_relaxed) ||
                        m_do_close.load(std::memory_order_relaxed) ||
                        m_buffer_size.load(std::memory_order_relaxed) >= m_overflow_limit;
                });

                m_do_flush.store(false, std::memory_order_relaxed);
                if (0 == m_buffer_size.load(std::memory_order_relaxed) && !m_do_close.load(std::memory_order_relaxed))    // spurious wakeup or flush on empty buffer
                    continue;
            
                temp_buffer.swap(m_buffer);
                m_buffer_size.store(0, std::memory_order_relaxed);
            }

            if (!temp_buffer.empty() && m_logfs.is_open())
            {
                while (!temp_buffer.empty())
                {
                    m_logfs << temp_buffer.front();
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

void LogWriter::log_line_start(const void* this_, const char* foo, int32_t line)
{
    if (nullptr == foo)
    {
#ifdef _WIN32
        fmt::format_to(std::back_inserter(m_buffer), FMT_COMPILE("[{:0>5} {:0>12X} {} ln:{}] "),
            ::GetThreadId(::GetCurrentThread()),
            cast::to_ptr(this_),
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
        fmt::format_to(std::back_inserter(m_buffer), FMT_COMPILE("[{:0>5} {:0>12X} {} {}:{}] "),
            ::GetThreadId(::GetCurrentThread()),
            cast::to_ptr(this_),
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
        fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("{}"), str);
    return *this;
}

LogWriter& LogWriter::operator << (const std::string& str)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("{}"), str);
    return *this;
}

LogWriter& LogWriter::operator << (std::string_view str)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("{}"), str.data());
    return *this;
}

LogWriter& LogWriter::operator << (const std::filesystem::path& path)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("{}"), path.string());
    return *this;
}

LogWriter& LogWriter::operator << (const byte_buff& buff)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("buff sz={}"), buff.size());
    return *this;
}

LogWriter& LogWriter::operator << (const tools::dt& dt)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("time={}"), dt.as_full_date(true));
    return *this;
}

LogWriter& LogWriter::operator << (const ImVec2& vec2)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_COMPILE("x={}, y={}"), vec2.x, vec2.y);
    return *this;
}

LogWriter& LogWriter::operator << (const ImVec4& vec4)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_COMPILE("x={}, y={}, z={}, w={}"), vec4.x, vec4.y, vec4.z, vec4.w);
    return *this;
}

LogWriter& LogWriter::operator << (const ImRect& rc)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_COMPILE("Min.x={}, Min.y={}, Max.x={}, Max.y={}"), rc.Min.x, rc.Min.y, rc.Max.x, rc.Max.y);
    return *this;
}

LogWriter& LogWriter::operator << (int32_t i32)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("{}"), i32);
    return *this;
}

LogWriter& LogWriter::operator << (uint32_t ui32)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("{}"), ui32);
    return *this;
}

LogWriter& LogWriter::operator << (int64_t i64)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("{}"), i64);
    return *this;
}

LogWriter& LogWriter::operator << (uint64_t ui64)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("{}"), ui64);
    return *this;
}

LogWriter& LogWriter::operator << (float f)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("{:.8f}"), f);
    return *this;
}

LogWriter& LogWriter::operator << (double d)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("{:.8f}"), d);
    return *this;
}

LogWriter& LogWriter::operator << (bool b)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("{}"), (b ? "yes" : "no"));
    return *this;
}

LogWriter& LogWriter::operator << (DlgCode code)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("{}"), cast::to_u64(code));
    switch (code)
    {
    case DlgCode::Progress:
        fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("=progress"));
        break;
    case DlgCode::Cancel:
        fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("=cancel"));
        break;
    default:
        LogError(ExcepCategory::SwitchEnum, "=unknown DlgCode " << cast::to_u64(code));
    }
    return *this;
}

LogWriter& LogWriter::operator << (const type_info& type)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("{}/{}"), type.name(), type.raw_name());
    return *this;
}

LogWriter& LogWriter::operator << (const LogLineEnd&)
{
    fmt::format_to(std::back_inserter(m_buffer), "\n");
    return *this;
}

LogWriter& LogWriter::operator << (const LogHex& hex_item)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("0x{:0>16X}"), hex_item.h);
    return *this;
}

LogWriter& LogWriter::operator << (const LogHRes& hres_item)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("0x{:0>16X}"), hres_item.h);
    return *this;
}

LogWriter& LogWriter::operator << (const LogPtr& ptr)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("0x{:0>16X}"), cast::to_ptr(ptr.ptr));
    return *this;
}

} // namespace glog

