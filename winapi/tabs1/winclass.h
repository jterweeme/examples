#ifndef WINCLASS_H
#define WINCLASS_H
#include <windows.h>

class WinClass
{
private:
    WNDCLASSEX _wc;
public:
    WinClass(WNDPROC wndProc, HINSTANCE hInstance, LPCWSTR className);
    void registerClass();
    HINSTANCE hInstance() const;
    LPCWSTR className() const;
};

#endif

