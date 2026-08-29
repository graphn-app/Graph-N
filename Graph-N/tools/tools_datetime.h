#pragma once

namespace tools
{

class dt final
{
public:
    static void save_app_start_time() noexcept { m_start_time = from_current_date(); }
    static const dt& get_app_start_time() noexcept { return m_start_time; }

	static dt from_current_date() noexcept;
    static dt from_time(time_t t) noexcept;

    bool null() const noexcept { return time() == 0; }

    bool operator==(const dt& other) noexcept { return m_raw == other.m_raw; }
    bool operator!=(const dt& other) noexcept { return m_raw != other.m_raw; }

    bool operator==(const time_t other) noexcept { return m_raw == other; }
    bool operator!=(const time_t other) noexcept { return m_raw != other; }

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

    /* length can be 1 (first char), 3 (first 3 chars) or more for full name */
	const char* month_name(int32_t length = 5) const noexcept;

private:
	int32_t m_day = 0;
    int32_t m_month = 0;
    int32_t m_year = 0;

    int32_t m_hour = 0;
    int32_t m_minute = 0;
    int32_t m_second = 0;
    int32_t m_millisecond = 0;

    time_t m_raw = 0;

    static dt m_start_time;
};

};	// namespace tools

