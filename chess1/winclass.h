#ifndef WINCLASS_H
#define WINCLASS_H

#include <windows.h>

class WinClass
{
private:
    WNDCLASS _wc;
public:
    WinClass(HINSTANCE hInstance, WNDPROC wndProc, LPCTSTR className);
    void registerClass();
};

#endif

