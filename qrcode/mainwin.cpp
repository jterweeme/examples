#include "mainwin.h"
#include "menubar.h"
#include "resource.h"
#include "dialog.h"

MainWindow *MainWindow::_instance;

MainWindow::MainWindow(HINSTANCE hInstance) : _hInstance(hInstance)
{
    _instance = this;
}

void MainWindow::_commandProc(HWND hwnd, WPARAM wParam)
{
    switch (LOWORD(wParam))
    {
    case IDM_NEW:
    {
        NewDlg newDlg(_hInstance);
        newDlg.run(hwnd);
    }
        return;
    case IDM_EXIT:
        PostQuitMessage(0);
        return;
    }
}

LRESULT MainWindow::_wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
        _commandProc(hwnd, wParam);
        return 0;
    case WM_CREATE:
#ifdef WINCE
        _menuBar = new MenuBarCE(_hInstance, IDM_MAIN);
#else
        _menuBar = new MenuBar(_hInstance, IDM_MAIN);
#endif
        _menuBar->enable(hwnd);
        break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        HDC hMemDC = CreateCompatibleDC(hdc);

        DeleteDC(hMemDC);
        EndPaint(hwnd, &ps);
    }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK MainWindow::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return _instance->_wndProc(hwnd, msg, wParam, lParam);
}

void MainWindow::create()
{
    WNDCLASS wc;
    wc.hInstance = _hInstance;
    wc.hIcon = NULL;
    wc.style = 0;
    wc.hCursor = NULL;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.lpszMenuName = NULL;
    wc.hbrBackground = 0;
    wc.lpszClassName = TEXT("QRClass");
    wc.lpfnWndProc = wndProc;

    if (!RegisterClass(&wc))
        throw TEXT("Cannot register main window class");

    const int x = CW_USEDEFAULT;
    const int y = CW_USEDEFAULT;
    const int w = CW_USEDEFAULT;
    const int h = CW_USEDEFAULT;
    _hwnd = CreateWindow(wc.lpszClassName, TEXT("QR Code"), 0, x, y, w, h, NULL, NULL, _hInstance, NULL);

    if (_hwnd == NULL)
        throw TEXT("Cannot create main window");
}

void MainWindow::show(int nCmdShow)
{
    ShowWindow(_hwnd, nCmdShow);
}

void MainWindow::update()
{
    UpdateWindow(_hwnd);
}

