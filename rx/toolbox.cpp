#include "toolbox.h"
#include <algorithm>

char Toolbox::bin(uint8_t b)
{
    if (b == 1)
        return '1';

    if (b == 0)
        return '0';

    throw std::string("Not a binary");
    return 'x';
}

char Toolbox::nibble(uint8_t n)
{
    return n <= 9 ? '0' + char(n) : 'A' + char(n - 10);
}

std::string Toolbox::bin8(uint8_t b)
{
    std::string ret;

    for (uint8_t i = 8; i > 0;)
        ret += b & 1 << --i ? '1' : '0';

    return ret;
}

std::string Toolbox::hex8(uint8_t b)
{
    std::string ret;
    ret += nibble(b >> 4 & 0xf);
    ret += nibble(b >> 0 & 0xf);
    return ret;
}

void Toolbox::hex8(std::ostream &os, uint8_t b)
{
    os.put(nibble(b >> 4 & 0xf));
    os.put(nibble(b >> 0 & 0xf));
}

std::string Toolbox::hex16(uint16_t w)
{
    std::string ret;
    ret += hex8(w >> 8 & 0xff);
    ret += hex8(w >> 0 & 0xff);
    return ret;
}

std::string Toolbox::hex32(uint32_t dw)
{
    std::string ret;
    ret += hex16(dw >> 16 & 0xffff);
    ret += hex16(dw >>  0 & 0xffff);
    return ret;
}

std::string Toolbox::hex64(uint64_t dw64)
{
    std::string ret;
    ret += hex32(dw64 >> 32 & 0xffffffff);
    ret += hex32(dw64 >>  0 & 0xffffffff);
    return ret;
}

char *Toolbox::utoa8(uint8_t b, char *s, int base) const
{
    if (b == 0)
    {
        s[0] = '0';
        s[1] = 0;
        return s;
    }

    int i = 0;
    while (b > 0)
    {
        uint8_t rem = b % base;
        s[i++] = rem > 9 ? (rem - 10) + 'a' : rem + '0';
        b = b / base;
    }

    s[i] = '\0';
    reverseStr(s, i);
    return s;
}

std::string Toolbox::utoa32(uint32_t dw, int base) const
{
    if (dw == 0)
        return std::string("0");

    std::string ret;

    while (dw > 0)
    {
        uint32_t rem = dw % base;
        char c = rem > 9 ? (rem - 10) + 'a' : rem + '0';
        ret.push_back(c);
        dw = dw / base;
    }

    reverseStr(ret);
    return ret;
}

template <class T> void mynReverse(T first, T last)
{
    while (first != last && first != --last)
        std::iter_swap(first++, last);
}

void Toolbox::reverseStr(char *s, size_t n)
{
    mynReverse(s, s + n);
}

void Toolbox::reverseStr(std::string &s)
{
    mynReverse(s.begin(), s.end());
}

std::string Toolbox::padding(const std::string &s, char c, size_t n)
{
    std::string ret;

    for (size_t i = s.length(); i < n; ++i)
        ret.push_back(c);

    ret.append(s);
    return ret;
}

std::wstring Toolbox::strtowstr(const std::string &s)
{
    std::wstring ret;
    std::string::size_type len = s.length();

    for (std::string::size_type i = 0; i < len; ++i)
        ret.push_back(wchar_t(s.at(i)));

    return ret;
}

std::string Toolbox::wstrtostr(const std::wstring &ws)
{
    std::string ret;
    std::string::size_type len = ws.length();

    for (std::string::size_type i = 0; i < len; ++i)
        ret.push_back(char(ws.at(i)));

    return ret;

}

void Toolbox::hexdump(std::ostream &os, const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i += 16)
    {
        uint32_t j = i;

        while (j < i + 16 && j < len)
        {
            hex8(os, data[j++]);
            os.put(' ');
        }

        while (j < i + 16)
        {
            os << "   ";
            j++;
        }

        for (j = i; j < i + 16 && j < len; ++j)
        {
            if (isprint(data[j]))
                os.put(data[j]);
            else
                os.put('.');
        }

        os << "\r\n";
    }
}



