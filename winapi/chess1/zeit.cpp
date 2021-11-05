#include "zeit.h"

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

BOOL SystemTime::leapYear(WORD year)
{
    if (year % 400 == 0)
        return TRUE;

    if (year % 100 == 0)
        return FALSE;

    return year % 4 == 0 ? TRUE : FALSE;
}

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

