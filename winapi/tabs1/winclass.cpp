#include "winclass.h"

WinClass::WinClass(WNDPROC wndProc, HINSTANCE hInstance, LPCWSTR className)
{
    _wc.cbSize = sizeof(WNDCLASSEX);
    _wc.style = 0;
    _wc.lpfnWndProc = wndProc;
    _wc.cbClsExtra = 0;
    _wc.cbWndExtra = 0;
    _wc.hInstance = hInstance;
    _wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    _wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    _wc.hbrBackground = HBRUSH(COLOR_WINDOW + 1);
    _wc.lpszMenuName = L"MAINMENU";
    _wc.lpszClassName = className;
    _wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
}

void WinClass::registerClass()
{
    if (!::RegisterClassEx(&_wc))
        throw "Window registration failed";
}

LPCWSTR WinClass::className() const
{
    return _wc.lpszClassName;
}

HINSTANCE WinClass::hInstance() const
{
    return _wc.hInstance;
}

