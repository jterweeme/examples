#include "winclass.h"

WinClass::WinClass(HINSTANCE hInstance, WNDPROC wndProc, LPCTSTR className)
{
    _wc.cbClsExtra = 0;
    _wc.cbWndExtra = 0;
    _wc.style = NULL;
    _wc.lpfnWndProc = wndProc;
    _wc.hInstance = hInstance;
    _wc.hIcon = ::LoadIcon(hInstance, className);
    _wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    _wc.hbrBackground = HBRUSH(GetStockObject(WHITE_BRUSH));
#ifdef WINCE
    _wc.lpszMenuName = NULL;
#else
    //_wc.lpszMenuName = className;
    _wc.lpszMenuName = NULL;
#endif
    _wc.lpszClassName = className;
}

void WinClass::registerClass()
{
    if (!::RegisterClass(&_wc))
        throw TEXT("Error registering class");
}

LPCTSTR WinClass::className() const
{
    return _wc.lpszClassName;
}

HINSTANCE WinClass::hInstance() const
{
    if (_wc.hInstance == NULL)
        throw TEXT("No hInstance!");

    return _wc.hInstance;
}

