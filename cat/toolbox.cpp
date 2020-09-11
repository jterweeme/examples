#include "toolbox.h"

char Toolbox::bin(uint8_t b)
{
    if (b == 1)
        return '1';

    if (b == 0)
        return '0';
#if 0
    throw TEXT("Not a binary");
#endif
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
    size_t len = s.length();

    for (size_t i = 0; i < len; ++i)
        ret.push_back(wchar_t(s.at(i)));

    return ret;
}

std::string Toolbox::wstrtostr(const std::wstring &ws)
{
    std::string ret;
    size_t len = ws.length();

    for (size_t i = 0; i < len; ++i)
        ret.push_back(char(ws.at(i)));

    return ret;

}

Toolbox::vw Toolbox::explode(const wchar_t *s, wchar_t delim)
{
    vw ret;
    std::wstring buf;

    for (const wchar_t *pc = s; true; ++pc)
    {
        if (*pc == 0)
        {
            if (!buf.empty())
                ret.push_back(buf);

            break;
        }
        
        if (*pc == delim)
        {
            if (!buf.empty())
                ret.push_back(buf);

            buf.clear();
            continue;
        }

        buf.push_back(*pc);
    }

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


