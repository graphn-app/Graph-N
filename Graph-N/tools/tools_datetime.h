
#pragma once

namespace tools
{

class dt final
{
public:
    inline static void save_app_start_time() { m_start_time = from_current_date(); }
    inline static const dt& get_app_start_time() { return m_start_time; }

	static dt from_current_date();
    static dt from_time(time_t t);

    inline bool null() const { return time() == 0; }

    inline bool operator==(const dt& other) { return m_raw == other.m_raw; }
    inline bool operator!=(const dt& other) { return m_raw != other.m_raw; }

    inline bool operator==(const time_t other) { return m_raw == other; }
    inline bool operator!=(const time_t other) { return m_raw != other; }

    inline int32_t day() const { return m_day; }
    inline int32_t month() const { return m_month; }
    inline int32_t year() const { return m_year; }
    inline int32_t hour() const { return m_hour; }
    inline int32_t minute() const { return m_minute; }
    inline int32_t second() const { return m_second; }
    inline int32_t millisecond() const { return m_millisecond; }

    inline time_t time() const { return m_raw; }

    std::string as_log_line() const;
	std::string as_filename() const;
    std::string as_full_date(bool add_ms) const;
    std::string as_full_date_readable(bool add_ms) const;

    /* length can be 1 (first char), 3 (first 3 chars) or more for full name */
	const char* month_name(int32_t length = 5) const;

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

