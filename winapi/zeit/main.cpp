#include <iostream>
#include <ctime>
#include <windows.h>

class SystemTime
{
private:
    SYSTEMTIME _sysTime;
public:
    static constexpr BYTE JANUARY = 1;
    static constexpr BYTE FEBRUARY = 2;
    static constexpr BYTE MARCH = 3;
    static constexpr BYTE APRIL = 4;
    static constexpr BYTE MAY = 5;
    static constexpr BYTE JUNE = 6;
    static constexpr BYTE JULY = 7;
    static constexpr BYTE AUGUST = 8;
    static constexpr BYTE SEPTEMBER = 9;
    static constexpr BYTE OCTOBER = 10;
    static constexpr BYTE NOVEMBER = 11;
    static constexpr BYTE DECEMBER = 12;
    void getTime();
    void dump(std::ostream &os);
    WORD day() const;
    WORD month() const;
    WORD year() const;
    WORD hour() const;
    WORD mins() const;
    WORD sec() const;
    DWORD unix() const;
    DWORD unixDays() const;
    DWORD unixHours() const;
    DWORD unixMins() const;
    static BOOL leapYear(WORD year);
    static WORD daysInMonth(WORD month, BOOL leap = FALSE);
};

WORD SystemTime::daysInMonth(WORD month, BOOL leap)
{
    switch (month)
    {
    case JANUARY:
    case MARCH:
    case MAY:
    case JULY:
    case AUGUST:
    case OCTOBER:
    case DECEMBER:
        return 31;
    case FEBRUARY:
        return leap ? 29 : 28;
    case APRIL:
    case JUNE:
    case SEPTEMBER:
    case NOVEMBER:
        return 30;
    }

    return 0;
}

WORD SystemTime::day() const
{
    return _sysTime.wDay;
}

WORD SystemTime::month() const
{
    return _sysTime.wMonth;
}

WORD SystemTime::year() const
{
    return _sysTime.wYear;
}

WORD SystemTime::hour() const
{
    return _sysTime.wHour;
}

WORD SystemTime::mins() const
{
    return _sysTime.wMinute;
}

WORD SystemTime::sec() const
{
    return _sysTime.wSecond;
}

void SystemTime::getTime()
{
    GetSystemTime(&_sysTime);
}

void SystemTime::dump(std::ostream &os)
{
    os << day() << "/" << month() << "/" << year() << "\n";
}

BOOL SystemTime::leapYear(WORD year)
{
    if (year % 400 == 0)
        return TRUE;

    if (year % 100 == 0)
        return FALSE;

    return year % 4 == 0 ? TRUE : FALSE;
}

//days since unix epoch
DWORD SystemTime::unixDays() const
{
    DWORD ret = 0;
    WORD currentYear = this->year();
    WORD currentMonth = this->month();

    for (WORD year = 1970; year < currentYear; ++year)
        ret += leapYear(year) ? 366 : 365;

    for (WORD month = 1; month < currentMonth; ++month)
        ret += daysInMonth(month, leapYear(currentYear));

    return ret + day() - 1;
}

//hours since unix epoch
DWORD SystemTime::unixHours() const
{
    return unixDays() * 24 + hour();
}

//minutes since unix epoch
DWORD SystemTime::unixMins() const
{
    return unixHours() * 60 + mins();
}

//seconds since unix epoch
DWORD SystemTime::unix() const
{
    return unixMins() * 60 + sec();
}

int main()
{
    time_t zeit;
    time(&zeit);
    std::cout << zeit << "\n";
    SystemTime sysTime;
    sysTime.getTime();
    std::cout << sysTime.unix() << "\n";
    //std::cout << sysTime.unixDays() << "\n";
    //std::cout << sysTime.hour() << "\n";
    //sysTime.dump(std::cout);
    std::cout.flush();
    return 0;
}

