#ifndef TOOLBOX_H
#define TOOLBOX_H

#include <string>
#include <iostream>
//#include <windows.h>

#if __cplusplus >= 201103L
#define CONSTEXPR constexpr
#define NULLPTR nullptr
#else
#define CONSTEXPR const
#define NULLPTR NULL
#endif

template <class T> const T& mynMax(const T &a, const T &b)
{
    return a < b ? b : a;
}

template <class T> const T& mynMin(const T &a, const T &b)
{
    return a < b ? a : b;
}


template <class T> void mynSwap(T &a, T &b)
{
    T tmp = a;
    a = b;
    b = tmp;
}

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
    char *utoa8(uint8_t b, char *s, int base) const;
    std::string utoa32(uint32_t dw, int base) const;
    static void reverseStr(char *s, size_t n);
    static void reverseStr(std::string &s);
    static std::string padding(const std::string &s, char c, size_t n);
    static std::wstring strtowstr(const std::string &s);
    static std::string wstrtostr(const std::wstring &ws);
    static void hexdump(std::ostream &os, const uint8_t *data, uint32_t len);
};
#endif

