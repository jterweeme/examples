#include "mainwin.h"
#include "menubar.h"
#include "resource.h"
#include "dialog.h"
#include "qrcode.h"
#include <iostream>

MainWindow *MainWindow::_instance;

MainWindow::MainWindow(HINSTANCE hInstance) : _hInstance(hInstance)
{
    _instance = this;

    //terminate string
    _qrInput[0] = 0;
}

void MainWindow::_commandProc(HWND hwnd, WPARAM wParam)
{
    switch (LOWORD(wParam))
    {
    case IDM_NEW:
    {
        NewDlg newDlg(_hInstance, _qrInput);

        if (newDlg.run(hwnd))
        {
            RECT r;
            GetClientRect(hwnd, &r);
            HDC hdc = GetDC(hwnd);

            FillRect(hdc, &r, HBRUSH(GetStockObject(WHITE_BRUSH)));
            ReleaseDC(hwnd, hdc);

            InvalidateRect(hwnd, &r, FALSE);
        }

    }
        return;
    case IDM_EXIT:
        PostQuitMessage(0);
        return;
    }
}

void MainWindow::_drawQrCode(HDC hdc, INT x, INT y, INT w, INT h, LPCSTR s)
{
    const QrCode::Ecc errCorLvl = QrCode::Ecc::LOW;
    const QrCode qr = QrCode::encodeText(s, errCorLvl);
    const HBRUSH blackBrush = HBRUSH(GetStockObject(BLACK_BRUSH));
    const HBRUSH whiteBrush = HBRUSH(GetStockObject(WHITE_BRUSH));

    for (int iy = 0; iy < qr.getSize(); ++iy)
    {
        for (int ix = 0; ix < qr.getSize(); ++ix)
        {
            const HBRUSH brush = qr.getModule(ix, iy) ? blackBrush : whiteBrush;
            const LONG left = ix *w + x, top = iy * h + y;
            const RECT r = {left, top, left + w, top + y};
            FillRect(hdc, &r, brush);
        }
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
        std::cout << "WM_PAINT\n";
        std::cout.flush();

        if (strlen(_qrInput) > 1)
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            _drawQrCode(hdc, 10, 10, 10, 10, _qrInput);
            EndPaint(hwnd, &ps);
        }
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
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.lpszMenuName = NULL;
    wc.hbrBackground = 0;
    wc.lpszClassName = TEXT("QRClass");
    wc.lpfnWndProc = wndProc;

    if (!RegisterClass(&wc))
        throw TEXT("Cannot register main window class");

    const DWORD style = WS_OVERLAPPEDWINDOW;
    const int x = CW_USEDEFAULT;
    const int y = CW_USEDEFAULT;
    const int w = CW_USEDEFAULT;
    const int h = CW_USEDEFAULT;
    _hwnd = CreateWindow(wc.lpszClassName, TEXT("QR Code"), style, x, y, w, h, NULL, NULL, _hInstance, NULL);

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

