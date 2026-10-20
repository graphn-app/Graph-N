/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once

namespace tools
{

class dt final
{
public:
    static void save_app_start_time() noexcept { m_start_time = from_current_date(); }
    static const dt& get_app_start_time() noexcept { return m_start_time; }

    // local time factories
    static dt from_current_date() noexcept;
    static dt from_time(time_t t) noexcept;

    // UTC time factories
    static dt from_current_date_utc() noexcept;
    static dt from_time_utc(time_t t) noexcept;

    bool null() const noexcept { return time() == 0; }
    bool is_utc() const noexcept { return m_is_utc; }

    bool operator==(const dt& other) const noexcept { return m_raw == other.m_raw; }
    bool operator!=(const dt& other) const noexcept { return m_raw != other.m_raw; }

    bool operator==(const time_t other) const noexcept { return m_raw == other; }
    bool operator!=(const time_t other) const noexcept { return m_raw != other; }

    int32_t day() const noexcept { return m_day; }
    int32_t month() const noexcept { return m_month; }
    int32_t year() const noexcept { return m_year; }
    int32_t hour() const noexcept { return m_hour; }
    int32_t minute() const noexcept { return m_minute; }
    int32_t second() const noexcept { return m_second; }
    int32_t millisecond() const noexcept { return m_millisecond; }

    time_t time() const noexcept { return m_raw; }

    std::string as_log_line() const;
    std::string as_filename() const;
    std::string as_full_date(bool add_ms) const;
    std::string as_full_date_readable(bool add_ms) const;

    // always returns true UTC time with, recalculates if object is local
    std::string as_iso8601() const;

    // length can be 1 (first char), 2 or 3 (first 3 chars) or more (or zero) for full name. 
    const char* month_name(int32_t length = 5) const noexcept;

private:
    static dt create_from_raw(time_t raw, uint16_t millis, bool utc) noexcept;

    int32_t m_day = 0;
    int32_t m_month = 0;
    int32_t m_year = 0;

    int32_t m_hour = 0;
    int32_t m_minute = 0;
    int32_t m_second = 0;
    int32_t m_millisecond = 0;

    time_t m_raw = 0;
    bool m_is_utc = false;

    static dt m_start_time;
};

};	// namespace tools

