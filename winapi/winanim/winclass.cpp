#include "winclass.h"

WinClass::WinClass(HINSTANCE hInstance, WNDPROC wndProc, LPCTSTR name)
{
    _wc.cbSize = sizeof(WNDCLASSEX);
    _wc.style = 0;
    _wc.lpfnWndProc = wndProc;
    _wc.cbClsExtra = 0;
    _wc.cbWndExtra = 0;
    _wc.hInstance = hInstance;
    _wc.hIcon = ::LoadIcon(NULL, IDI_APPLICATION);
    _wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    _wc.hbrBackground = HBRUSH(COLOR_BTNFACE + 1);
    _wc.lpszMenuName = NULL;
    _wc.lpszClassName = name;
    _wc.hIconSm = ::LoadIcon(NULL, IDI_APPLICATION);
}

void WinClass::registerClass()
{
    ATOM ret = ::RegisterClassExW(&_wc);

    if (ret == 0)
        throw "Cannot register class";
}

HINSTANCE WinClass::hInstance() const
{
    return _wc.hInstance;
}

LPCTSTR WinClass::className() const
{
    return _wc.lpszClassName;
}
