#include "mainwin.h"
#include "winclass.h"
#include <commctrl.h>
#include <strsafe.h>

MainWin *MainWin::_instance;

MainWin::MainWin(WinClass *wc) :
    Window(wc, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480),
    _tc(this, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT)
{
    _instance = this;
}

void MainWin::create()
{
    Window::create(L"Tab example");
}

void MainWin::_createProc(HWND hwnd)
{
    _tc.create(_wc->hInstance(), hwnd);
    TCITEMW tie;
    TCHAR achTemp[256];
    tie.mask = TCIF_TEXT | TCIF_IMAGE;
    tie.iImage = -1;
    tie.pszText = achTemp;
    StringCbPrintf(achTemp, 256, L"Maandag");
    _tc.insert(0, &tie);
    StringCbPrintf(achTemp, 256, L"Dinsdag");
    _tc.insert(1, &tie);
}

LRESULT MainWin::_wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        _createProc(hwnd);
        break;
    case WM_SIZE:
        _tc.move(0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        break;
    case WM_CLOSE:
        ::DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        break;
    default:
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK MainWin::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return _instance->_wndProc(hwnd, msg, wParam, lParam);
}

