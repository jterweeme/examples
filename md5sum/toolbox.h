#ifndef TOOLBOX_H
#define TOOLBOX_H

#include <string>
#include <iostream>

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
typedef unsigned short uint16_t;
typedef unsigned uint32_t;
#endif

class Toolbox
{
public:
    static char bin(uint8_t b);
    static char nibble(uint8_t n);
    static std::string bin8(uint8_t b);
    static std::string hex8(uint8_t b);
    static void hex8(std::ostream &os, uint8_t b);
    static std::string hex16(uint16_t w);
    static std::string hex32(uint32_t dw);
    static std::string hex64(uint64_t dw64);
    static std::string padding(const std::string &s, char c, size_t n);
    static std::wstring strtowstr(const std::string &s);
    static std::string wstrtostr(const std::wstring &ws);
    void hexdump(std::ostream &os, const uint8_t *data, uint32_t len) const;
    static uint32_t swapEndian(uint32_t x);
    static uint32_t be32tohost(uint32_t num);
};
#endif

