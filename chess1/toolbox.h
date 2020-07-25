#ifndef TOOLBOX_H
#define TOOLBOX_H

#ifdef STL
#include <string>
#include <iostream>
#endif
#include <windows.h>

class Toolbox
{
public:
    static char nibble(BYTE n);
#ifdef STL
    static std::string hex8(BYTE b);
    static void hex8(std::ostream &os, BYTE b);
    static std::string hex16(WORD w);
    static std::string hex32(DWORD dw);
    static std::string hex64(DWORD64 dw64);
    static std::wstring strtowstr(const std::string &s);
    static void hexdump(std::ostream &os, BYTE *data, DWORD len);
#endif
    static void messageBox(HINSTANCE hInstance, HWND hwnd, UINT errId, UINT captionId);
};
#endif

