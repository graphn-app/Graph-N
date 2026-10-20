/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "tools/tools_datetime.h"

namespace tools
{
dt dt::m_start_time;

dt dt::create_from_raw(time_t raw, uint16_t millis, bool utc) noexcept
{
    if (raw > 100'000'000'000)
        raw /= 1000;

    std::tm ptm = {};
    bool success = false;

    if (utc)
    {
#ifdef _WIN32
        success = (gmtime_s(&ptm, &raw) == 0);
#else
        success = (gmtime_r(&raw, &ptm) != nullptr);
#endif
    }
    else
    {
#ifdef _WIN32
        success = (localtime_s(&ptm, &raw) == 0);
#else
        success = (localtime_r(&raw, &ptm) != nullptr);
#endif
    }

    if (!success)
    {
        LogWarnS("time conversion failed");
        return {};
    }

    dt date = {};
    date.m_raw = raw;
    date.m_is_utc = utc;

    date.m_year = cast::to_i32(ptm.tm_year + 1900);
    date.m_month = cast::to_i32(ptm.tm_mon + 1);
    date.m_day = cast::to_i32(ptm.tm_mday);

    date.m_hour = cast::to_i32(ptm.tm_hour);
    date.m_minute = cast::to_i32(ptm.tm_min);
    date.m_second = cast::to_i32(ptm.tm_sec);
    date.m_millisecond = cast::to_i32(millis);

    return date;
}

dt dt::from_current_date() noexcept
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
    return create_from_raw(t, static_cast<uint16_t>(millis), false);
}

dt dt::from_current_date_utc() noexcept
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
    return create_from_raw(t, static_cast<uint16_t>(millis), true);
}

dt dt::from_time(time_t raw) noexcept
{
    // millis extraction is removed here since raw time_t only has seconds.
    // if you need exact ms from raw, you should pass it as an argument.
    return create_from_raw(raw, 0, false);
}

dt dt::from_time_utc(time_t raw) noexcept
{
    return create_from_raw(raw, 0, true);
}

const char* dt::month_name(int32_t length /*= 5*/) const noexcept
{
    if (0 == length)
        length = 5;

    switch (m_month)
    {
    case 1:
        return (1 == length ? "J" : (length <= 3 ? "Jan" : "January"));
    case 2:
        return (1 == length ? "F" : (length <= 3 ? "Feb" : "February"));
    case 3:
        return (1 == length ? "M" : (length <= 3 ? "Mar" : "March"));
    case 4:
        return (1 == length ? "A" : (length <= 3 ? "Apr" : "April"));
    case 5:
        return (1 == length ? "M" : (length <= 3 ? "May" : "May"));
    case 6:
        return (1 == length ? "J" : (length <= 3 ? "Jun" : "June"));
    case 7:
        return (1 == length ? "J" : (length <= 3 ? "Jul" : "July"));
    case 8:
        return (1 == length ? "A" : (length <= 3 ? "Aug" : "August"));
    case 9:
        return (1 == length ? "S" : (length <= 3 ? "Sep" : "September"));
    case 10:
        return (1 == length ? "O" : (length <= 3 ? "Oct" : "October"));
    case 11:
        return (1 == length ? "N" : (length <= 3 ? "Nov" : "November"));
    case 12:
        return (1 == length ? "D" : (length <= 3 ? "Dec" : "December"));
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
    return fmt::format(FMT_STRING("{:02}{:02}{:02}_{:02}{:02}{:02}"),
        m_year % 100, m_month, m_day,
        m_hour, m_minute, m_second);
}

std::string dt::as_full_date(bool add_ms) const
{
    if (add_ms)
    {
        // YYYY/MMM/DD HH:MM:SS.mmm
        return fmt::format(FMT_STRING("{:04}/{}/{:02} {:02}:{:02}:{:02}.{:03}"),
            m_year, month_name(3), m_day,
            m_hour, m_minute, m_second, m_millisecond);
    }

    // YYYY/MMM/DD HH:MM:SS
    return fmt::format(FMT_STRING("{:04}/{}/{:02} {:02}:{:02}:{:02}"),
        m_year, month_name(3), m_day,
        m_hour, m_minute, m_second);
}

std::string dt::as_full_date_readable(bool add_ms) const
{
    if (add_ms)
    {
        // YYYY MMM D, HH:MM:SS.mmm
        return fmt::format(FMT_STRING("{} {:02}, {:04}. {:02}:{:02}:{:02}.{:03}"),
            month_name(3), m_day, m_year, 
            m_hour, m_minute, m_second, m_millisecond);
    }

    // YYYY/MMM/DD_HH:MM:SS
    return fmt::format(FMT_STRING("{} {:02}, {:04}. {:02}:{:02}:{:02}"),
        month_name(3), m_day, m_year,
        m_hour, m_minute, m_second);
}

std::string dt::as_iso8601() const
{
    // ISO 8601 with 'Z' MUST be in UTC.
    // If this object holds local time components, dynamically calculate UTC
    // to prevent sending fake future timestamps.
    if (!m_is_utc)
        return from_time_utc(m_raw).as_iso8601();

    // 2026-10-19T20:18:00.000Z
    return fmt::format(FMT_STRING("{:04}-{:02}-{:02}T{:02}:{:02}:{:02}.{:03}Z"),
        m_year, m_month, m_day, m_hour, m_minute, m_second, m_millisecond);
}
}	//	namespace tools
