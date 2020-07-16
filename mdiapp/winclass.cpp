#include "winclass.h"

WinClass::WinClass(WNDPROC wndProc, HINSTANCE hInstance,
    LPCWSTR className, LPCWSTR menu, HBRUSH bg)
{
    _wc.cbSize = sizeof(WNDCLASSEX);
    _wc.style = CS_HREDRAW | CS_VREDRAW;
    _wc.lpfnWndProc = wndProc;
    _wc.cbClsExtra = 0;
    _wc.cbWndExtra = 0;
    _wc.hInstance = hInstance;
    _wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    _wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    _wc.hbrBackground = bg;
    _wc.lpszMenuName = menu;
    _wc.lpszClassName = className;
    _wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
}

int WinClass::registerClass()
{
    if (!RegisterClassEx(&_wc))
    {
        MessageBox(0, L"Could Not Register Window", L"Oh Oh...", MB_ICONEXCLAMATION | MB_OK);
        return -1;
    }

    return 0;
}

LPCWSTR WinClass::className() const
{
    return _wc.lpszClassName;
}

HINSTANCE WinClass::hInstance() const
{
    return _wc.hInstance;
}

