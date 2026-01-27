
namespace tools
{
dt dt::m_start_time;

dt dt::from_current_date()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm ptm;
    localtime_s(&ptm, &t);

    const auto duration = now.time_since_epoch();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

    dt date = {};

    date.m_year = cast::to_u16(ptm.tm_year + 1900);
    date.m_month = cast::to_u8(ptm.tm_mon + 1);
    date.m_day = cast::to_u8(ptm.tm_mday);

    date.m_hour = cast::to_u8(ptm.tm_hour);
    date.m_minute = cast::to_u8(ptm.tm_min);
    date.m_second = cast::to_u8(ptm.tm_sec);
    date.m_millisecond = cast::to_u16(millis);

    return date;
}

const char* dt::month_name(int32_t length /*= 0*/) const
{
    switch (m_month)
    {
    case 1:
        return (1 == length ? "J" : (3 == length ? "Jan" : "January"));
    case 2:
        return (1 == length ? "F" : (3 == length ? "Feb" : "February"));
    case 3:
        return (1 == length ? "M" : (3 == length ? "Mar" : "March"));
    case 4:
        return (1 == length ? "A" : (3 == length ? "Apr" : "April"));
    case 5:
        return (1 == length ? "M" : (3 == length ? "May" : "May"));
    case 6:
        return (1 == length ? "J" : (3 == length ? "Jun" : "June"));
    case 7:
        return (1 == length ? "J" : (3 == length ? "Jul" : "July"));
    case 8:
        return (1 == length ? "A" : (3 == length ? "Aug" : "August"));
    case 9:
        return (1 == length ? "S" : (3 == length ? "Sep" : "September"));
    case 10:
        return (1 == length ? "O" : (3 == length ? "Oct" : "October"));
    case 11:
        return (1 == length ? "N" : (3 == length ? "Nov" : "November"));
    case 12:
        return (1 == length ? "D" : (3 == length ? "Dec" : "December"));
    }

    return "";
}

std::string dt::as_log_line() const
{
    // HH:MM:SS.mmm
    return fmt::format(FMT_COMPILE("{:02}:{:02}:{:02}.{:03}"), m_hour, m_minute, m_second, m_millisecond);
}

std::string dt::as_filename() const
{
    // YYMMDD_HHMMSS
    return fmt::format("{:02}{:02}{:02}_{:02}{:02}{:02}",
        m_year % 100, m_month, m_day,
        m_hour, m_minute, m_second);
}

std::string dt::as_full_date() const
{
    // YYYY/MMM/DD_HH:MM:SS.mmm
    return fmt::format("{:04}/{}/{:02}_{:02}:{:02}:{:02}.{:03}",
        m_year, month_name(3), m_day,
        m_hour, m_minute, m_second, m_millisecond);
}

}	//	namespace tools
