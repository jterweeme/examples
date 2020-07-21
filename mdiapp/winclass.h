#ifndef WINCLASS_H
#define WINCLASS_H
#include <windows.h>

class WinClass
{
public:
    WNDCLASSEX _wc;
public:
    WinClass(WNDPROC wndProc, HINSTANCE hInstance, LPCTSTR className, LPCTSTR menu, HBRUSH bg);
    LPCTSTR className() const;
    HINSTANCE hInstance() const;
    void registerClass();
};
#endif

