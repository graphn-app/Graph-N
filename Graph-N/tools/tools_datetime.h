
#pragma once

namespace tools
{

class dt final
{
public:
    static void save_app_start_time() { m_start_time = from_current_date(); }
    static const dt& get_app_start_time() { return m_start_time; }

	static dt from_current_date();

	int32_t day() const { return m_day; }
    int32_t month() const { return m_month; }
    int32_t year() const { return m_year; }
    int32_t hour() const { return m_hour; }
    int32_t minute() const { return m_minute; }
    int32_t second() const { return m_second; }
    int32_t millisecond() const { return m_millisecond; }

    std::string as_log_line() const;
	std::string as_filename() const;
    std::string as_full_date() const;

    /* length can be 1 (first char), 3 (first 3 chars) or more for full name */
	const char* month_name(int32_t length = 5) const;

	int32_t m_day = 0;
    int32_t m_month = 0;
    int32_t m_year = 0;

    int32_t m_hour = 0;
    int32_t m_minute = 0;
    int32_t m_second = 0;
    int32_t m_millisecond = 0;

    static dt m_start_time;
};

};	// namespace tools

