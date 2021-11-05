#ifndef ZEIT_H
#define ZEIT_H

#include "chess.h"

class SystemTime
{
private:
    SYSTEMTIME _sysTime;
public:
    static CONSTEXPR BYTE JANUARY = 1;
    static CONSTEXPR BYTE FEBRUARY = 2;
    static CONSTEXPR BYTE MARCH = 3;
    static CONSTEXPR BYTE APRIL = 4;
    static CONSTEXPR BYTE MAY = 5;
    static CONSTEXPR BYTE JUNE = 6;
    static CONSTEXPR BYTE JULY = 7;
    static CONSTEXPR BYTE AUGUST = 8;
    static CONSTEXPR BYTE SEPTEMBER = 9;
    static CONSTEXPR BYTE OCTOBER = 10;
    static CONSTEXPR BYTE NOVEMBER = 11;
    static CONSTEXPR BYTE DECEMBER = 12;
    void getTime();
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

#endif

