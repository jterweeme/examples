//file: lsq/toolbox.cpp

#include "toolbox.h"
#include <stdexcept>

bool Toolbox::isxdigit(char c)
{
    QString inc("0123456789abcdefABCDEF");
    return inc.contains(c);
}

char Toolbox::hex4(quint8 n)
{
    if (n > 0xf)
        throw std::range_error("Mag niet groter dan 0xf");

    return n <= 9 ? '0' + char(n) : 'A' + char(n - 10);
}

quint8 Toolbox::nibble2(char c)
{
    if (c >= '0' && c <= '9')
        return quint8(c - '0');

    if (c >= 'a' && c <= 'f')
        return quint8(c - 'a') + 0xa;

    if (c >= 'A' && c <= 'F')
        return quint8(c - 'A') + 0xa;

    throw std::range_error("geen hexadecimaal karakter");
    return 0;
}

//quint8 to hexadecimal string
QString Toolbox::hex8q(quint8 b)
{
    QString ret;
    ret.push_back(hex4(b >> 4 & 0xf));
    ret.push_back(hex4(b >> 0 & 0xf));
    return ret;
}

//quint16 to hexadecimal string
QString Toolbox::hex16q(quint16 w)
{
    QString ret;
    ret.append(hex8q(w >> 8 & 0xff));
    ret.append(hex8q(w >> 0 & 0xff));
    return ret;
}

//quint32 to hexadecimal string
QString Toolbox::hex32q(quint32 dw)
{
    QString ret;
    ret.append(hex16q(dw >> 16 & 0xffff));
    ret.append(hex16q(dw >>  0 & 0xffff));
    return ret;
}

//integer power 32bit
quint32 Toolbox::iPow32(quint32 base, quint32 exp)
{
    quint32 ret = base;

    while (--exp > 0)
        ret *= base;

    return ret;
}

//integer power 64bit
quint64 Toolbox::iPow64(quint64 base, quint64 exp)
{
    quint64 ret = base;

    while (--exp > 0)
        ret *= base;

    return ret;
}

//quint8 to decimal string padded with zero's
QString Toolbox::dec8q(quint8 b)
{
    QString ret;
    ret.push_back(hex4(b / 100 % 10));
    ret.push_back(hex4(b / 10 % 10));
    ret.push_back(hex4(b / 1 % 10));
    return ret;
}

//quint16 to decimal string padded with zero's
QString Toolbox::dec16q(quint16 w)
{
    QString ret;

    for (quint32 i = 10000; i > 0; i /= 10)
        ret.push_back(hex4(w / i % 10));

    return ret;
}

//quint32 to decimal string
QString Toolbox::dec32q(quint32 dw)
{
    QString ret;
    const quint32 zeros = iPow32(10, 9);
    bool flag = false;

    for (quint32 i = zeros; i > 0; i /= 10)
    {
        quint8 digit = dw / i % 10;

        if (digit > 0 || i == 1)
            flag = true;

        if (flag)
            ret.push_back(hex4(digit));
    }

    return ret;
}

QString Toolbox::dec64q(quint64 qw)
{
    QString ret;
    const quint64 zeros = iPow64(10, 19);
    bool flag = false;

    for (quint64 i = zeros; i > 0; i /= 10)
    {
        quint8 digit = qw / i % 10;

        if (digit > 0 || i == 1)
            flag = true;

        if (flag)
            ret.push_back(hex4(digit));
    }

    return ret;
}

QString Toolbox::dec64sq(qint64 val)
{
    if (val >= 0)
        return dec64q(val);

    QString ret;
    val *= -1;
    ret.push_back('-');
    ret.append(dec64q(val));
    return ret;
}

QString Toolbox::padding(const QString &s, char c, size_t n)
{
    QString ret;

    for (int i = s.length(); i < n; ++i)
    {
        ret.push_back(c);
    }

    ret.append(s);
    return ret;
}


