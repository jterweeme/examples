#ifndef TOOLBOX_H
#define TOOLBOX_H

#include <string>
#include <iostream>
#include <windows.h>

#if __cplusplus >= 201103L
#define CPP11
#endif

#ifdef CPP11
#define CONSTEXPR constexpr
#define NULLPTR nullptr
#else
#define CONSTEXPR const
#define NULLPTR NULL

typedef unsigned char uint8_t;
typedef unsigned uint32_t;
#endif

class Toolbox
{
public:
    static char bin(BYTE b);
    static char nibble(BYTE n);
    static std::string bin8(BYTE b);
    static std::string hex8(BYTE b);
    static void hex8(std::ostream &os, BYTE b);
    static std::string hex16(WORD w);
    static std::string hex32(DWORD dw);
    static std::string hex64(DWORD64 dw64);
    static std::string padding(const std::string &s, char c, size_t n);
    static std::wstring strtowstr(const std::string &s);
    static std::string wstrtostr(const std::wstring &ws);
    void hexdump(std::ostream &os, const BYTE *data, DWORD len) const;
    static uint32_t swapEndian(uint32_t x);
    static uint32_t be32toh(uint32_t num);
};
#endif

