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
#define NOEXCEPT noexcept
#define MAKRO constexpr
#else
#define CONSTEXPR const
#define NULLPTR NULL
#define NOEXCEPT
#define MAKRO inline
#endif

class StrException : public std::exception
{
private:
    std::string _s;
public:
    StrException(const char *fmt, ...);
    const char *what() const NOEXCEPT override;
};

class Toolbox
{
public:
    template <class T> static const T& myMin(const T& a, const T &b)
    {
        return a < b ? a : b;
    }

    template <class T> static const T& myMax(const T &a, const T &b)
    {
        return a < b ? b : a;
    }

    static char nibble(BYTE n);
    static std::string hex8(BYTE b);
    static void hex8(std::ostream &os, BYTE b);
    static std::string hex16(WORD w);
    static std::string hex32(DWORD dw);
    static std::string hex64(DWORD64 dw64);
    static std::wstring strtowstr(const std::string &s);
    static void hexdump(std::ostream &os, BYTE *data, DWORD len);
    static void messageBox(HINSTANCE hInstance, HWND hwnd, UINT errId, UINT captionId);
};
#endif

