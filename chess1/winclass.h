#ifndef WINCLASS_H
#define WINCLASS_H

#include <windows.h>

class WinClass
{
private:
    WNDCLASS _wc;
public:
    WinClass(HINSTANCE hInstance, WNDPROC wndProc, LPCTSTR className);
    HINSTANCE hInstance() const;
    LPCTSTR className() const;
    void registerClass();
};

#endif

