#include "winclass.h"
#include "resource.h"

WinClass::WinClass(WNDPROC wndProc, HINSTANCE hInstance, LPCTSTR className)
{
    _wc.cbSize = sizeof(WNDCLASSEX);
    _wc.style = CS_HREDRAW | CS_VREDRAW;
    _wc.lpfnWndProc = wndProc;
    _wc.cbClsExtra = 0;
    _wc.cbWndExtra = 0;
    _wc.hInstance = hInstance;
    _wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    _wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    _wc.hbrBackground = HBRUSH(GetStockObject(WHITE_BRUSH));
    _wc.lpszMenuName = MAKEINTRESOURCE(IDR_MAIN_MENU);
    _wc.lpszClassName = className;
    _wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
}

void WinClass::registerClass()
{
    if (!::RegisterClassEx(&_wc))
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

