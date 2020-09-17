#ifndef TOOLBOX_H
#define TOOLBOX_H

#include <string>
#include <iostream>
#include <windows.h>

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
    static char bin(BYTE b);
    static char nibble(BYTE n);
    static std::string bin8(BYTE b);
    static std::string hex8(BYTE b);
    static void hex8(std::ostream &os, BYTE b);
    static std::string hex16(WORD w);
    static std::string hex32(DWORD dw);
    static std::string hex64(DWORD64 dw64);
    char *utoa8(BYTE b, char *s, int base) const;
    std::string utoa32(DWORD dw, int base) const;
    static void reverseStr(char *s, size_t n);
    static void reverseStr(std::string &s);
    static std::string padding(const std::string &s, char c, size_t n);
    static std::wstring strtowstr(const std::string &s);
    static std::string wstrtostr(const std::wstring &ws);
    void hexdump(std::ostream &os, const BYTE *data, DWORD len) const;
    static void errorBox(HWND hwnd, LPCSTR err);
    static void errorBox(HWND hwnd, LPCWSTR err);
    static void errorBox(LPCSTR err);
    static void errorBox(LPCWSTR err);
};
#endif

