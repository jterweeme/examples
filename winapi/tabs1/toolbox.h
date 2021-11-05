#ifndef TOOLBOX_H
#define TOOLBOX_H

#include <string>
#include <iostream>
#include <windows.h>

class Toolbox
{
public:
    static char nibble(BYTE n);
    static std::string hex8(BYTE b);
    static void hex8(std::ostream &os, BYTE b);
    static std::string hex16(WORD w);
    static std::string hex32(DWORD dw);
    static std::string hex64(DWORD64 dw64);
    static std::wstring strtowstr(const std::string &s);
    static void hexdump(std::ostream &os, BYTE *data, DWORD len);
    static void errorBoxA(HWND hwnd, LPCSTR err);
    static void errorBoxW(HWND hwnd, LPCWSTR err);
    static void errorBox(HWND hwnd, LPCTSTR err);
    static void errorBox(HINSTANCE hInstance, HWND hwnd, UINT errId);
    static void messageBox(HINSTANCE hInstance, HWND hwnd, UINT errId, UINT captionId);
};
#endif

