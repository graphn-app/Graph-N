
namespace tools
{
namespace time
{

uint16_t dt::current_year()
{
    return from_current_date().year();
}

dt dt::from_current_date()
{
    __time64_t t = 0;
    _time64(&t);
    tm ptm = {};
    _localtime64_s(&ptm, &t);

    dt date = {};

    date.m_year = cast::to_u16(ptm.tm_year + 1900);
    date.m_month = cast::to_u8(ptm.tm_mon + 1);
    date.m_day = cast::to_u8(ptm.tm_mday);

    date.m_hour = cast::to_u8(ptm.tm_hour);
    date.m_minute = cast::to_u8(ptm.tm_min);
    date.m_second = cast::to_u8(ptm.tm_sec);

    date.m_time =
        (time_type(date.m_year) << 48) | (time_type(date.m_month) << 40) | (time_type(date.m_day) << 32) |
        (time_type(date.m_hour) << 24) | (time_type(date.m_minute) << 16) | (time_type(date.m_second) << 8);

    return date;
}

dt dt::from_time_t(time_type t)
{
    dt date = {};

    date.m_time = t;

    date.m_year = (uint16_t)(t >> 48) & 0xffff;
    date.m_month = (byte)(t >> 40) & 0xff;
    date.m_day = (byte)(t >> 32) & 0xff;

    date.m_hour = (byte)(t >> 24) & 0xff;
    date.m_minute = (byte)(t >> 16) & 0xff;
    date.m_second = (byte)(t >> 8) & 0xff;

    return date;
}

dt dt::from_date(uint16_t y, byte m, byte d, bool allow_invalid /*= false*/)
{
    if (!allow_invalid && !check_valid(y, m, d))
        return {};

    dt date = {};

    date.m_year = y;
    date.m_month = m;
    date.m_day = d;

    date.m_time =
        (time_type(date.m_year) << 48) | (time_type(date.m_month) << 40) | (time_type(date.m_day) << 32) |
        (time_type(date.m_hour) << 24) | (time_type(date.m_minute) << 16) | (time_type(date.m_second) << 8);

    return date;
}

dt dt::from_jdn_as_gregorian_date(uint64_t jdn)
{
    if (0 == jdn)
        return {};

    // https://ru.wikipedia.org/wiki/%D0%AE%D0%BB%D0%B8%D0%B0%D0%BD%D1%81%D0%BA%D0%B0%D1%8F_%D0%B4%D0%B0%D1%82%D0%B0

    const uint64_t a = jdn + 32044;
    const uint64_t b = ((4 * a) + 3) / 146097;
    const uint64_t c = a - uint64_t((146097 * b) / 4);
    const uint64_t d = ((4 * c) + 3) / 1461;
    const uint64_t e = c - uint64_t((1461 * d) / 4);
    const uint64_t m = ((5 * e) + 2) / 153;

    dt date = {};

    date.m_day = cast::to_u8(e - uint64_t(((153 * m) + 2) / 5) + 1);
    date.m_month = cast::to_u8(m + 3 - 12 * uint64_t(m / 10));
    date.m_year = cast::to_u16((100 * b) + d - 4800 + uint64_t(m / 10));

    date.m_time =
        (time_type(date.m_year) << 48) | (time_type(date.m_month) << 40) | (time_type(date.m_day) << 32) |
        (time_type(date.m_hour) << 24) | (time_type(date.m_minute) << 16) | (time_type(date.m_second) << 8);

    return date;
}

dt dt::from_jdn_as_julian_date(uint64_t jdn)
{
    if (0 == jdn)
        return {};

    // https://ru.wikipedia.org/wiki/%D0%AE%D0%BB%D0%B8%D0%B0%D0%BD%D1%81%D0%BA%D0%B0%D1%8F_%D0%B4%D0%B0%D1%82%D0%B0

    const uint64_t c = jdn + 32082;
    const uint64_t d = ((4 * c) + 3) / 1461;
    const uint64_t e = c - uint64_t(1461 * d / 4);
    const uint64_t m = ((5 * e) + 2) / 153;

    dt date = {};

    date.m_day = cast::to_u8(e - uint64_t(((153 * m) + 2) / 5) + 1);
    date.m_month = cast::to_u8(m + 3 - (12 * uint64_t(m / 10)));
    date.m_year = cast::to_u16(d - 4800 + uint64_t(m / 10));

    date.m_time =
        (time_type(date.m_year) << 48) | (time_type(date.m_month) << 40) | (time_type(date.m_day) << 32) |
        (time_type(date.m_hour) << 24) | (time_type(date.m_minute) << 16) | (time_type(date.m_second) << 8);

    return date;
}

bool dt::check_valid(byte m, byte d)
{
    if (m < 1 || m > 12 || d < 1 || d > 31)
        return false;

    constexpr byte days_in_months[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    const byte days_in_month = days_in_months[m - 1];
    return d <= days_in_month;
}

bool dt::check_valid(uint16_t y, byte m, byte d)
{
    if (0 == y)
        return check_valid(m, d);

    if (2 == m) // Feb
    {
        if (is_leap_year(y) && d <= 29 && d >= 1)
            return true;
    }

    return check_valid(m, d);
}

byte dt::num_of_days_in_month(uint16_t y, byte m)
{
    if (y != 0 && 2 == m && is_leap_year(y))
        return 29;
    if (0 == y && 2 == m)
        return 29;
    if (0 == m)
        return 31;
    constexpr byte days_in_months[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    const byte days_in_month = days_in_months[m - 1];
    return days_in_month;
}

byte dt::num_of_days_in_month()
{
    const auto dt = from_current_date();
    return num_of_days_in_month(dt.year(), dt.month());
}

bool dt::is_leap_year(uint16_t y)
{
    return (((y % 4 == 0) && (y % 100 != 0)) || (y % 400 == 0));
}

dt dt::from_time(byte h, byte m, byte s)
{
    dt date = {};

    date.m_hour = h;
    date.m_minute = m;
    date.m_second = s;

    date.m_time =
        (time_type(date.m_year) << 48) | (time_type(date.m_month) << 40) | (time_type(date.m_day) << 32) |
        (time_type(date.m_hour) << 24) | (time_type(date.m_minute) << 16) | (time_type(date.m_second) << 8);

    return date;
}

dt dt::from_date_time(uint16_t y, byte mth, byte d, byte h, byte mnt, byte s)
{
    if (!check_valid(y, mth, d, h, mnt, s))
        return {};

    dt date = {};

    date.m_year = y;
    date.m_month = mth;
    date.m_day = d;
    date.m_hour = h;
    date.m_minute = mnt;
    date.m_second = s;

    date.m_time =
        (time_type(date.m_year) << 48) | (time_type(date.m_month) << 40) | (time_type(date.m_day) << 32) |
        (time_type(date.m_hour) << 24) | (time_type(date.m_minute) << 16) | (time_type(date.m_second) << 8);

    return date;
}

bool dt::check_valid(uint16_t y, byte m, byte d, byte h, byte mnt, byte s)
{
    if (!check_valid(y, m, d))
        return false;
    if (h < 0 || h > 23)
        return false;
    if (mnt < 0 || mnt > 59)
        return false;
    if (s < 0 || s > 59)
        return false;
    return true;
}

byte dt::day_of_week(uint16_t y, byte m, byte d)
{
    if (!check_valid(y, m, d))
        return cast::to_u8(-1);

    if (0 == y)
        return d % 7 - 1;

    // https://cs.uwaterloo.ca/~alopez-o/math-faq/node73.html

    const int yi = y - (m < 3);
    const int mi = m;
    const int di = d;
    return (yi + yi / 4 - yi / 100 + yi / 400 + "-bed=pen+mad."[mi] + di) % 7;
}

std::string dt::dates_term(const dt& from, const dt& to)
{
    if (!from.is_valid() || !to.is_valid())
        return "";

    auto days = from.diff_days(to);
    std::string res;

    if (days <= 1)
        res = "One day";
    else if (days < 30)
        res = std::to_string(days) + " days";
    else
    {
        auto y = days / 365;
        if (y > 0)
        {
            days = days - (y * 365);
            res = std::to_string(y) + (y > 1 ? " years, " : " year, ");
        }
        auto m = days / 30;
        if (m > 0)
        {
            days = days - (m * 30);
            res += std::to_string(m) + (m > 1 ? " months, " : " month, ");
        }
        if (days > 0)
            res += std::to_string(days) + " days";
    }

    return tools::str::trim(res, " ,");
}

int dow(int m, int d, int y)
{
    y -= m < 3;
    return (y + y / 4 - y / 100 + y / 400 + "-bed=pen+mad."[m] + d) % 7;
}

const char* dt::week_day_name(byte d, int length /*= 0*/)
{
    switch (d)
    {
    case 0:
        return (1 == length ? "S" : (3 == length ? "Sun" : "Sunday"));
    case 1:
        return (1 == length ? "M" : (3 == length ? "Mon" : "Monday"));
    case 2:
        return (1 == length ? "T" : (3 == length ? "Tue" : "Tuesday"));
    case 3:
        return (1 == length ? "W" : (3 == length ? "Wed" : "Wednesday"));
    case 4:
        return (1 == length ? "T" : (3 == length ? "Thu" : "Thursday"));
    case 5:
        return (1 == length ? "F" : (3 == length ? "Fri" : "Friday"));
    case 6:
        return (1 == length ? "S" : (3 == length ? "Sat" : "Saturday"));
    }

    LogAndThrow("=Undefined day " << d);
}

const char* dt::month_name(byte m, int length /*= 0*/)
{
    switch (m)
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
    //	LogAndThrow("=Undefined month " << m);
}

const char* dt::month_name(int length /*= 5*/, const char* def /*= ""*/) const
{
    const auto name = dt::month_name(m_month, length);
    if (0 == *name)
        return def;
    return name;
}

void dt::clear()
{
    m_year = 0;
    m_month = 0;
    m_day = 0;
    m_hour = 0;
    m_minute = 0;
    m_second = 0;
    m_time = 0;
}

int dt::compare(const dt& date) const
{
    auto cmp = [](int left, int right)
    {
        if (left < right)
            return -1;
        if (left > right)
            return 1;
        return 0;
    };

    int r = cmp(m_year, date.m_year);
    if (0 == r)
    {
        r = cmp(m_month, date.m_month);
        if (0 == r)
        {
            r = cmp(m_day, date.m_day);
            if (0 == r)
            {
                r = cmp(m_hour, date.m_hour);
                if (0 == r)
                {
                    r = cmp(m_minute, date.m_minute);
                    if (0 == r)
                    {
                        r = cmp(m_second, date.m_second);
                    }
                }
            }
        }
    }

    return r;
}

__time64_t dt::to_time_t(const dt& date, uint16_t /*add_years*/) const
{
    tm time = {};
    time.tm_isdst = -1;
    time.tm_year = date.year() - 1900/* + add_years*/;
    time.tm_mon = date.month() - 1;
    time.tm_mday = date.day();
    time.tm_hour = date.hour();
    time.tm_min = date.minute();
    time.tm_sec = date.second();
    return _mktime64(&time);
}

uint64_t dt::get_jdn_by_gregorian_calendar(uint64_t* months /*= nullptr*/, uint64_t* years /*= nullptr*/) const
{
    // https://ru.wikipedia.org/wiki/%D0%AE%D0%BB%D0%B8%D0%B0%D0%BD%D1%81%D0%BA%D0%B0%D1%8F_%D0%B4%D0%B0%D1%82%D0%B0

    if (0 == day() || 0 == month() || 0 == year())
        return 0;

    const uint64_t a = (14 - month()) / 12;
    const uint64_t y = year() + 4800 - a;
    const uint64_t m = month() + (12 * a) - 3;
    const uint64_t jdn = day() + uint64_t(((153 * m) + 2) / 5) + (365 * y) + uint64_t(y / 4) - uint64_t(y / 100) + uint64_t(y / 400) - 32045;

    if (months != nullptr)
        *months = m;
    if (years != nullptr)
        *years = y;

    return jdn;
}

uint64_t dt::get_jdn_by_julian_calendar(uint64_t* months /*= nullptr*/, uint64_t* years /*= nullptr*/) const
{
    // https://ru.wikipedia.org/wiki/%D0%AE%D0%BB%D0%B8%D0%B0%D0%BD%D1%81%D0%BA%D0%B0%D1%8F_%D0%B4%D0%B0%D1%82%D0%B0

    if (0 == day() || 0 == month() || 0 == year())
        return 0;

    const uint64_t a = (14 - month()) / 12;
    const uint64_t y = year() + 4800 - a;
    const uint64_t m = month() + (12 * a) - 3;
    const uint64_t jdn = day() + uint64_t(((153 * m) + 2) / 5) + (365 * y) + uint64_t(y / 4) - 32083;

    if (months != nullptr)
        *months = m;
    if (years != nullptr)
        *years = y;

    return jdn;
}

uint64_t dt::diff_seconds(const dt& date) const
{
    const auto make_time_point = [](const dt& date) -> std::chrono::system_clock::time_point
    {
        std::tm timeinfo = {};
        timeinfo.tm_year = date.year() - 1900;
        timeinfo.tm_mon = date.month() - 1;
        timeinfo.tm_mday = date.day();
        timeinfo.tm_hour = date.hour();
        timeinfo.tm_min = date.minute();
        timeinfo.tm_sec = date.second();
        std::time_t time_c = std::mktime(&timeinfo);
        return std::chrono::system_clock::from_time_t(time_c);
    };

    const auto this_tp = make_time_point(*this);
    const auto their_tp = make_time_point(date);
    const auto difference = std::chrono::duration_cast<std::chrono::seconds>(this_tp - their_tp);
    return difference.count() * (difference.count() < 0 ? -1 : 1);
}

uint64_t dt::diff_days(const dt& date) const
{
    const auto this_days = get_jdn_by_gregorian_calendar();
    const auto their_days = date.get_jdn_by_gregorian_calendar();
    if (0 == this_days || 0 == their_days)
        return 0;

    return std::max(this_days, their_days) - std::min(this_days, their_days);
}

uint64_t dt::diff_years(const dt& date) const
{
    const auto this_year = year();
    const auto this_month = month();
    const auto this_day = day();

    const auto their_year = date.year();
    const auto their_month = date.month();
    const auto their_day = date.day();

    if (this_year == their_year)
        return 0;

    uint16_t hi_year = 0, low_year = 0;
    byte hi_month = 0, hi_day = 0;
    byte low_month = 0, low_day = 0;

    if (this_year > their_year)
    {
        hi_year = this_year; low_year = their_year;
        hi_month = this_month; low_month = their_month;
        hi_day = this_day; low_day = their_day;
    }
    else
    {
        hi_year = their_year; low_year = this_year;
        hi_month = their_month; low_month = this_month;
        hi_day = their_day; low_day = this_day;
    }

    const bool date_equal = (hi_month == low_month) && (hi_day == low_day);

    const auto diff_next_year = [=]() -> uint64_t
    {
        if (date_equal)
            return 1;

        if (low_month > hi_month)
            return 0;
        if (low_month < hi_month)
            return 1;

        if (low_day > hi_day)
            return 0;
        return 1; // <=
    };

    return hi_year - low_year - 1 + diff_next_year();
}

bool dt::is_equal_to_date(const dt& other) const
{
    return (m_day == other.m_day && m_month == other.m_month && m_year == other.m_year);
}

bool dt::is_less_than_date(const dt& other) const
{
    if (m_year < other.m_year) return true;
    if (m_year > other.m_year) return false;
    if (m_month < other.m_month) return true;
    if (m_month > other.m_month) return false;
    return (m_day < other.m_day);
}

bool dt::is_greater_than_date(const dt& other) const
{
    if (m_year > other.m_year) return true;
    if (m_year < other.m_year) return false;
    if (m_month > other.m_month) return true;
    if (m_month < other.m_month) return false;
    return (m_day > other.m_day);
}

bool dt::is_equal_to_date_time(const dt& other) const
{
    return is_equal_to_date(other) && (m_hour == other.m_hour && m_minute == other.m_minute && m_second == other.m_second);
}

bool dt::is_less_than_date_time(const dt& other) const
{
    if (is_less_than_date(other)) return true;
    if (m_hour < other.m_hour) return true;
    if (m_hour > other.m_hour) return false;
    if (m_minute < other.m_minute) return true;
    if (m_minute > other.m_minute) return false;
    return (m_second < other.m_second);
}

bool dt::is_greater_than_date_time(const dt& other) const
{
    if (is_greater_than_date(other)) return true;
    if (m_hour > other.m_hour) return true;
    if (m_hour < other.m_hour) return false;
    if (m_minute > other.m_minute) return true;
    if (m_minute < other.m_minute) return false;
    return (m_second > other.m_second);
}

std::string dt::as_string(bool add_day, bool add_time, bool month_as_name /*= false*/) const
{
    if (0 == m_time)
        return "";

    std::stringstream as_day;
    if (month_as_name)
    {
        as_day
            << month_name(m_month, 3)
            << " "
            << int(m_day)
            << ", "
            << std::setw(4) << std::setfill('0') << std::to_string(m_year)
            ;
    }
    else
    {
        as_day
            << std::setw(4) << std::setfill('0') << std::to_string(m_year) << "/"
            << std::setw(2) << std::setfill('0') << std::to_string(m_month) << "/"
            << std::setw(2) << std::setfill('0') << std::to_string(m_day)
            ;
    }
    std::stringstream as_time;
    as_time
        << std::setw(2) << std::setfill('0') << std::to_string(m_hour) << ":"
        << std::setw(2) << std::setfill('0') << std::to_string(m_minute) << ":"
        << std::setw(2) << std::setfill('0') << std::to_string(m_second)
        ;

    auto ret = (add_day ? as_day.str() : "");
    if (add_day && add_time)
        ret += " " + as_time.str();
    else if (add_time)
        ret = as_time.str();
    return ret;
}

std::string dt::as_filename() const
{
    if (0 == m_time)
        return "";

    std::stringstream ret;
    ret
        << std::setw(2) << std::setfill('0') << std::to_string(m_year - 2000)
        << std::setw(2) << std::setfill('0') << std::to_string(m_month)
        << std::setw(2) << std::setfill('0') << std::to_string(m_day)
        << "_"
        << std::setw(2) << std::setfill('0') << std::to_string(m_hour)
        << std::setw(2) << std::setfill('0') << std::to_string(m_minute)
        << std::setw(2) << std::setfill('0') << std::to_string(m_second)
        ;

    return ret.str();
}

}	// namespace time
}	//	namespace tools
