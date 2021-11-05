#ifndef WINCLASS_H
#define WINCLASS_H
#include <windows.h>

class WinClass
{
private:
    WNDCLASS _wc;
public:
    WinClass(WNDPROC wndProc, HINSTANCE hInstance, LPCTSTR className);
    void registerClass();
    HINSTANCE hInstance() const;
    LPCTSTR className() const;
};

#endif

