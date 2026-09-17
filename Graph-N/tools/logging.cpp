#include "tools/logging.h"
#ifndef TOOLS_DETACHED_STATE
#include "core/base/version.h"
#include "core/managers/db_manager.h"
#endif // TOOLS_DETACHED_STATE

namespace glog
{

constexpr const char* g_log_file_mark = "__log_enabled__";
bool Logger::m_log_file_mark_enabled = false;
#ifndef TOOLS_DETACHED_STATE
constexpr const char* g_log_web_dump_file_mark = "__dump_web_response__";
constexpr const char* g_log_object_info_file_mark = "__dump_object_info__";
constexpr const char* g_log_invalid_json_mark = "__dump_invalid_json__";

bool Logger::m_log_web_dump_file_mark_enabled = false;
bool Logger::m_log_object_info_file_mark_enabled = false;
bool Logger::m_log_invalid_json_mark_enabled = false;
#endif // TOOLS_DETACHED_STATE
std::atomic_bool Logger::m_enabled = false;

/////////////////////////////////////////////////////////////////

namespace
{
void cleanup(const std::filesystem::path& dir, const std::string& prefix, size_t num)
{
    const auto sort_callback = [](const std::filesystem::path& lhs, const std::filesystem::path& rhs)
    {
        return lhs.u8string() > rhs.u8string();
    };

    auto list = tools::path::enumerate_directory(dir, false, false, true, false);
    std::ranges::sort(list, sort_callback);

    std::list<std::filesystem::path> filtered;
    for (auto& file : list)
    {
        if (tools::path::path_to_string(file.filename()).starts_with(prefix))
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
} // static 

void Logger::cleanup() noexcept
{
    try
    {
        const auto path = fs::get_log_directory_path();
        glog::cleanup(path, APP_CAPTION, 3);
        glog::cleanup(path, APP_COMFYUI, 3);
    }
    catch (...) {}
}

namespace
{
bool check_log_file_mark_exists(const char* file_mark)
{
    auto path = fs::get_log_directory_path();
    path /= file_mark;
    return tools::path::exists(path);
}
} //namespace

void Logger::check_log_file_mark()
{
    m_log_file_mark_enabled = check_log_file_mark_exists(g_log_file_mark);
    m_log_web_dump_file_mark_enabled = check_log_file_mark_exists(g_log_web_dump_file_mark);
    m_log_object_info_file_mark_enabled = check_log_file_mark_exists(g_log_object_info_file_mark);
    m_log_invalid_json_mark_enabled = check_log_file_mark_exists(g_log_invalid_json_mark);
}

void Logger::set_log_enabled(bool enabled)
{
    set_log_enabled(enabled, g_log_file_mark);
}

void Logger::set_log_web_response_enabled(bool enabled)
{
    set_log_enabled(enabled, g_log_web_dump_file_mark);
}

void Logger::set_log_object_info_enabled(bool enabled)
{
    set_log_enabled(enabled, g_log_object_info_file_mark);
}

void Logger::set_log_dump_invalid_json_enabled(bool enabled)
{
    set_log_enabled(enabled, g_log_invalid_json_mark);
}

void Logger::set_log_enabled(bool enable, const char* log_mark)
{
    auto path = fs::get_log_directory_path();
    path /= log_mark;

    if (enable)
    {
        tools::path::make_file_path(path, true /*overwrite*/, "");
        //instance().log_start();
    }
    else
    {
        //instance().log_stop();
        tools::path::remove(path);
    }

    check_log_file_mark();
}

Logger& Logger::instance() noexcept
{
    static Logger logger;
    return logger;
}

void Logger::stop() noexcept
{
    log_stop();
}

Logger::Logger() noexcept
{
    log_start();
}

Logger::~Logger()
{
    log_stop();
}

void Logger::force_flush() noexcept
{
    m_do_flush.store(true, std::memory_order_relaxed);
    m_cv.notify_one();
}

void Logger::log_start() noexcept
{
    check_log_file_mark();
    if (get_log_started() || !get_log_enabled())
        return;

    try
    {
        m_enabled.store(false, std::memory_order_relaxed);

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

#ifndef TOOLS_DETACHED_STATE
            const auto db_meta = mngr::DbManager::read_db_metadata();
#endif // TOOLS_DETACHED_STATE

            LogWriter writer;
            writer
                << "\n\n"
                << "Log started: " << tools::dt::get_app_start_time().as_log_line() << "\n"
#ifndef TOOLS_DETACHED_STATE
                << "App version: " << ver::ver_as_string() << "\n"
                << "Db meta: "
#endif // TOOLS_DETACHED_STATE
            ;
#ifndef TOOLS_DETACHED_STATE
            if (db_meta.db_ver != 0)
                writer << "db ver " << db_meta.db_ver << ", app ver " << db_meta.app_ver << ", last run " << db_meta.last_run << "\n";
            else
#endif // TOOLS_DETACHED_STATE
                writer << "\n";
            writer
#ifndef TOOLS_DETACHED_STATE
                << "Generated client id: " << base::GlobalID::client_id() << "\n"
#endif // TOOLS_DETACHED_STATE
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
    catch (...) {}
}

void Logger::log_stop() noexcept
{
    if (!get_log_started())
        return;

    try
    {
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
    catch (...)
    {}
}

void Logger::write(LogWriter& writer) noexcept
{
    if (!get_log_started())
        return;

    try
    {
        auto buffer = writer.extract_buffer();

#if defined _WIN32 && defined _DEBUG
        ::OutputDebugStringA((APP_CAPTION "__: " + buffer).c_str());
#endif

        std::lock_guard<std::mutex> lock(m_lock);
        m_buffer.emplace(std::move(buffer));
        if (m_buffer_size.fetch_add(1, std::memory_order_relaxed) + 1 >= m_overflow_limit)
            m_cv.notify_one();
    }
    catch (...) {}
}

void Logger::working_thread()
{
    if (!get_log_started())
        return;

    std::queue<std::string> temp_buffer;

    while (get_log_started() && !m_do_close.load(std::memory_order_relaxed))
    {
        try
        {
            {
                std::unique_lock<std::mutex> lock(m_lock);
                m_cv.wait_for(lock, std::chrono::milliseconds(m_flush_timeout), [this]() noexcept {
                    return
                        m_do_flush.load(std::memory_order_relaxed) ||
                        m_do_close.load(std::memory_order_relaxed) ||
                        m_buffer_size.load(std::memory_order_relaxed) >= m_overflow_limit;
                });

                m_do_flush.store(false, std::memory_order_relaxed);
                if (0 == m_buffer_size.load(std::memory_order_relaxed) && !m_do_close.load(std::memory_order_relaxed)) [[unlikely]]    // spurious wakeup or flush on empty buffer
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
            set_log_stopped();
            (void)e;
            SAVE_EXCEPTION(e.what());
            DEBUG_CATCH;
        }
    }
}

////////////////////////////////////////////////////
///////////////		LogWriter

LogWriter::LogWriter()
{
    m_buffer.reserve(1024);
}

void LogWriter::log_line_start(const void* this_, const char* foo, int32_t line)
{
    if (nullptr == foo)
    {
#ifdef _WIN32
        fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("[{:0>5} {:0>12X} {} ln:{}] "),
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
    else [[likely]]
    {
#ifdef _WIN32
        fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("[{:0>5} {:0>12X} {} {}:{}] "),
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
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("{}"), str);
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

#ifndef TOOLS_DETACHED_STATE
LogWriter& LogWriter::operator << (const ImVec2& vec2)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("x={}, y={}"), vec2.x, vec2.y);
    return *this;
}

LogWriter& LogWriter::operator << (const ImVec4& vec4)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("x={}, y={}, z={}, w={}"), vec4.x, vec4.y, vec4.z, vec4.w);
    return *this;
}

LogWriter& LogWriter::operator << (const ImRect& rc)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("Min.x={}, Min.y={}, Max.x={}, Max.y={}"), rc.Min.x, rc.Min.y, rc.Max.x, rc.Max.y);
    return *this;
}
#endif // TOOLS_DETACHED_STATE

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

#ifdef _WIN32
LogWriter& LogWriter::operator << (DWORD dw)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("{}"), uint32_t(dw));
    return *this;
}
#endif // WIN32

#ifndef TOOLS_DETACHED_STATE
LogWriter& LogWriter::operator << (const number& n)
{
    static_assert(std::same_as<number_t, double>, "Check this function if number_t changes");
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("{:.8f}"), n.underlying_type());
    return *this;
}

LogWriter& LogWriter::operator << (DlgCode code)
{
    fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("{}"), cast::to_u64(code));
    switch (code)
    {
    case DlgCode::Progress: [[likely]]
        fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("=progress"));
        break;
    case DlgCode::Cancel:
        fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("=cancel"));
        break;
    default:
        fmt::format_to(std::back_inserter(m_buffer), FMT_STRING("=unknown"));
        LogError("=unknown DlgCode " << cast::to_u64(code));
    }
    return *this;
}
#endif // TOOLS_DETACHED_STATE

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

