#ifndef WINCLASS_H
#define WINCLASS_H

#include <windows.h>

class WinClass
{
private:
    WNDCLASSEX _wc;
public:
    WinClass(HINSTANCE hInstance, WNDPROC wndProc, LPCTSTR name);
    void registerClass();
    HINSTANCE hInstance() const;
    LPCTSTR className() const;
};

#endif

