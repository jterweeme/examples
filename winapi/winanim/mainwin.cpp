#include "mainwin.h"
#include "winclass.h"

MainWin *MainWin::_instance;

MainWin::MainWin(WinClass *wc) : _wc(wc)
{
    _instance = this;
}

void MainWin::create()
{
    _hwnd = CreateWindowExW(WS_EX_CLIENTEDGE, _wc->className(), L"A Bitmap Program",
                    WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                    320, 240, NULL, NULL, _wc->hInstance(), NULL);

    if (_hwnd == 0)
        throw "Cannot create window";
}

void MainWin::show(int nCmdShow)
{
    ::ShowWindow(_hwnd, nCmdShow);
}

void MainWin::update()
{
    ::UpdateWindow(_hwnd);
}

LRESULT CALLBACK MainWin::wndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    return _instance->_wndProc(hwnd, Message, wParam, lParam);
}

LRESULT MainWin::_wndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
    case WM_CREATE:
        _ball.create(_wc->hInstance(), hwnd);
        break;
    case WM_TIMER:
        _ball.timer(hwnd);
        break;
    case WM_PAINT:
        _ball.paint(hwnd);
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        _ball.destroy(hwnd);
        PostQuitMessage(0);
        break;
    default:
        break;
    }
    return ::DefWindowProc(hwnd, Message, wParam, lParam);
}
