#ifndef WINCLASS_H
#define WINCLASS_H
#include <windows.h>

class WinClass
{
public:
    WNDCLASSEX _wc;
public:
    WinClass(WNDPROC wndProc, HINSTANCE hInstance, LPCWSTR className, LPCWSTR menu, HBRUSH bg);
    LPCWSTR className() const;
    HINSTANCE hInstance() const;
    int registerClass();
};
#endif

