#ifndef TOOLBOX_H
#define TOOLBOX_H

#include <string>
#include <iostream>
#include <vector>

#ifdef WINCE
#include <windows.h>
#endif

#if __cplusplus >= 201103L
#define CONSTEXPR constexpr
#define NULLPTR nullptr
#else
#define CONSTEXPR const
#define NULLPTR NULL
#endif

#ifdef WINCE
typedef BYTE uint8_t;
typedef WORD uint16_t;
typedef DWORD uint32_t;
typedef DWORD64 uint64_t;
#endif

class Toolbox
{
public:
    typedef std::vector<std::wstring> vw;
    typedef vw::iterator vwi;
    static char bin(uint8_t b);
    static char nibble(uint8_t n);
    static std::string bin8(uint8_t b);
    static std::string hex8(uint8_t b);
    static void hex8(std::ostream &os, uint8_t b);
    static std::string hex16(uint16_t w);
    static std::string hex32(uint32_t dw);
    static std::string hex64(uint64_t dw64);
    static void reverseStr(char *s, size_t n);
    static void reverseStr(std::string &s);
    static std::string padding(const std::string &s, char c, size_t n);
    static std::wstring strtowstr(const std::string &s);
    static std::string wstrtostr(const std::wstring &ws);
    static void hexdump(std::ostream &os, const uint8_t *data, uint32_t len);
    static vw explode(const wchar_t *s, wchar_t delim);
};
#endif

