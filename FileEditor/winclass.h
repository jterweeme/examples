#ifndef WINCLASS_H
#define WINCLASS_H
#include <windows.h>

class WinClass
{
private:
    WNDCLASSEX _wc;
public:
    WinClass(WNDPROC wndProc, HINSTANCE hInstance, LPCTSTR className);
    void registerClass();
    HINSTANCE hInstance() const;
    LPCTSTR className() const;
};

#endif

