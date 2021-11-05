#include "winclass.h"

WinClass::WinClass(WNDPROC wndProc, HINSTANCE hInstance, LPCTSTR className)
{
    _wc.style = 0;
    _wc.lpfnWndProc = wndProc;
    _wc.cbClsExtra = 0;
    _wc.cbWndExtra = 0;
    _wc.hInstance = hInstance;
    _wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    _wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    _wc.hbrBackground = HBRUSH(COLOR_WINDOW + 1);
    _wc.lpszMenuName = TEXT("MAINMENU");
    _wc.lpszClassName = className;
}

void WinClass::registerClass()
{
    if (!::RegisterClass(&_wc))
        throw TEXT("Window registration failed");
}

LPCTSTR WinClass::className() const
{
    return _wc.lpszClassName;
}

HINSTANCE WinClass::hInstance() const
{
    return _wc.hInstance;
}

