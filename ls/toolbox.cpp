//file: toolbox.cpp

#include "toolbox.h"

char Toolbox::hex4(uint8_t n)
{
    return n <= 9 ? '0' + char(n) : 'A' + char(n - 10);
}

std::string Toolbox::hex8(uint8_t b)
{
    std::string ret;
    ret += hex4(b >> 4 & 0xf);
    ret += hex4(b >> 0 & 0xf);
    return ret;
}

void Toolbox::hex8(std::ostream &os, uint8_t b)
{
    os.put(hex4(b >> 4 & 0xf));
    os.put(hex4(b >> 0 & 0xf));
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

std::string Toolbox::padding(const std::string &s, char c, size_t n)
{
    std::string ret;

    for (std::string::size_type i = s.length(); i < n; ++i)
        ret.push_back(c);

    ret.append(s);
    return ret;
}

uint32_t Toolbox::iPow32(uint32_t base, uint32_t exp)
{
    uint32_t ret = base;

    while (--exp > 0)
        ret *= base;

    return ret;
}

uint64_t Toolbox::iPow64(uint64_t base, uint64_t exp)
{
    uint64_t ret = base;

    while (--exp > 0)
        ret *= base;

    return ret;
}

std::string Toolbox::dec32(uint32_t dw)
{
    std::string ret;
    const uint32_t zeros = iPow32(10, 9);
    bool flag = false;

    for (uint32_t i = zeros; i > 0; i /= 10)
    {
        uint8_t digit = dw / i % 10;

        if (digit > 0 || i == 1)
            flag = true;

        if (flag)
            ret.push_back(hex4(digit));
    }

    return ret;
}

std::string Toolbox::reverseStr(const std::string &s)
{
    std::string ret;
    int len = s.length();

    for (int i = 0; i < len; ++i)
    {
        ret.push_back(s.at(len - (i + 1)));
    }

    return ret;
}


