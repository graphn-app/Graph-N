
#pragma once

namespace tools
{
namespace time
{

class dt final
{
	// static foo
public:
    static void freeze_app_start_time() { m_start_time = from_current_date(); }
    static const dt& get_app_start_time() { return m_start_time; }

	static uint16_t current_year();

	static dt from_current_date();
	static dt from_time_t(time_type t);
	static dt from_date(uint16_t y, byte m, byte d, bool allow_invalid = false); // return empty if date is not valid
	static dt from_time(byte h, byte m, byte s);
	static dt from_date_time(uint16_t y, byte mth, byte d, byte h, byte mnt, byte s); // return empty if date is not valid

	/* return Gregorian/Julian calendar date by Julian day number (JDN) */
	static dt from_jdn_as_gregorian_date(uint64_t jdn);
	static dt from_jdn_as_julian_date(uint64_t jdn);

	static bool check_valid(byte m, byte d);
	static bool check_valid(uint16_t y, byte m, byte d);
	static bool check_valid(uint16_t y, byte m, byte d, byte h, byte mnt, byte s);

	static bool is_leap_year(uint16_t y);

	/* length can be 1 (first char), 3 (first 3 chars), any other length will be full name*/
	static const char* week_day_name(byte d, int length = 0); // 0 - sunday, 1 - monday, etc
	static const char* month_name(byte m, int length = 0);

	/* year can be 0 */
	static byte num_of_days_in_month(uint16_t y, byte m);
	/* means current date */
	static byte num_of_days_in_month();

	/* return day of week, where 0 - sunday, 1 - monday, etc*/
	static byte day_of_week(uint16_t y, byte m, byte d);

	static std::string dates_term(const dt& from, const dt& to);

	// object foo
public:
	time_type time() const { return m_time; }
	byte day() const { return m_day; }
	byte month() const { return m_month; }
	uint16_t year() const { return m_year; }
	byte hour() const { return m_hour; }
	byte minute() const { return m_minute; }
	byte second() const { return m_second; }

	bool is_valid() const { return m_time != 0; }
	void clear();

	std::string as_string(bool add_day, bool add_time, bool month_as_name = false) const;
	std::string as_filename() const;

	/* length can be 1 (first char), 3 (first 3 chars), any other length will be full name*/
	const char* month_name(int length = 5, const char* def = "") const;

	/* 
		return -1 if current date less than parameter
		return +1 if current date greater than parameter
		return  0 if dates are equal
	*/
	int compare(const dt& date) const;

	/* return Julian day number (JDN) by Gregorian/Julian calendar */
	uint64_t get_jdn_by_gregorian_calendar(uint64_t* months = nullptr, uint64_t* years = nullptr) const;
	uint64_t get_jdn_by_julian_calendar(uint64_t* months = nullptr, uint64_t* years = nullptr) const;

	/* return number of days/months/years between dates */
	uint64_t diff_seconds(const dt& date) const;
	uint64_t diff_days(const dt& date) const;
	uint64_t diff_years(const dt& date) const;

	bool is_equal_to_date(const dt& other) const;
	bool is_less_than_date(const dt& other) const;
	bool is_greater_than_date(const dt& other) const;

	bool is_equal_to_date_time(const dt& other) const;
	bool is_less_than_date_time(const dt& other) const;
	bool is_greater_than_date_time(const dt& other) const;


	bool operator == (const dt&) const = delete;
	bool operator != (const dt&) const = delete;
	bool operator < (const dt&) const = delete;
	bool operator > (const dt&) const = delete;

private:
	__time64_t to_time_t(const dt& date, uint16_t add_years) const;

private:
	byte m_day = 0;
	byte m_month = 0;
	uint16_t m_year = 0;

	byte m_hour = 0;
	byte m_minute = 0;
	byte m_second = 0;

	time_type m_time = 0;

    static dt m_start_time;
};

}	// namespace time

};	// namespace tools

