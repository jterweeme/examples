#include "menubar.h"
#include "resource.h"
#include <iostream>

#if __cplusplus >= 201103L
#define CONSTEXPR constexpr
#else
#define CONSTEXPR const
#endif

class MainWindow
{
private:
    static MainWindow *_instance;
    HINSTANCE _hInstance;
    HWND _hwnd;
    AbstractMenuBar *_menuBar;
    COLORREF _clrBackGround;
    HBRUSH _hBrushBackGround;

    LRESULT _wndProc(HWND, UINT, WPARAM, LPARAM);
    void _commandProc(HWND, WPARAM);
    void _bgColor(HWND hwnd, COLORREF color);
public:
    MainWindow(HINSTANCE hInstance);
    void create();
    void show(int nCmdShow);
    void update();
    static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);
};

MainWindow *MainWindow::_instance;

MainWindow::MainWindow(HINSTANCE hInstance) : _hInstance(hInstance)
{
    _instance = this;
}

void MainWindow::_bgColor(HWND hwnd, COLORREF color)
{
    _clrBackGround = color;
    _hBrushBackGround = HBRUSH(CreateSolidBrush(_clrBackGround));
    InvalidateRect(hwnd, NULL, TRUE);
}

void MainWindow::_commandProc(HWND hwnd, WPARAM wParam)
{
    switch (LOWORD(wParam))
    {
    case IDM_EXIT:
        PostQuitMessage(0);
        return;
    case IDM_WHITE:
        _bgColor(hwnd, RGB(255, 255, 255));
        return;
    case IDM_BLACK:
        _bgColor(hwnd, RGB(0, 0, 0));
        return;
    case IDM_RED:
        _bgColor(hwnd, RGB(255, 0, 0));
        return;
    case IDM_GREEN:
        _bgColor(hwnd, RGB(0, 255, 0));
        return;
    case IDM_BLUE:
        _bgColor(hwnd, RGB(0, 0, 255));
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
        _bgColor(hwnd, RGB(255, 255, 255));
#ifdef WINCE
        _menuBar = new MenuBarCE(_hInstance, IDM_MAIN);
#else
        _menuBar = new MenuBar(_hInstance, IDM_MAIN);
#endif
        _menuBar->enable(hwnd);
        break;
    case WM_ERASEBKGND:
    {
        RECT rect;
        ::GetClientRect(hwnd, &rect);
        ::FillRect(HDC(wParam), &rect, _hBrushBackGround);
    }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK MainWindow::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret;
    ret = _instance->_wndProc(hwnd, msg, wParam, lParam);
    return ret;
}

void MainWindow::create()
{
    WNDCLASS wc;
    wc.hIcon = NULL;
    wc.hInstance = _hInstance;
    wc.style = 0;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszMenuName = NULL;
    wc.lpfnWndProc = wndProc;
    wc.lpszClassName = TEXT("classname");
    wc.hbrBackground = HBRUSH(GetStockObject(WHITE_BRUSH));

    if (!RegisterClass(&wc))
        throw TEXT("Cannot register winclass");
#ifdef WINCE
    CONSTEXPR DWORD style = WS_VISIBLE;
#else
    CONSTEXPR DWORD style = WS_OVERLAPPEDWINDOW;
#endif
    CONSTEXPR INT x = CW_USEDEFAULT;
    CONSTEXPR INT y = CW_USEDEFAULT;
    CONSTEXPR INT w = CW_USEDEFAULT;
    CONSTEXPR INT h = CW_USEDEFAULT;

    _hwnd = CreateWindow(wc.lpszClassName, TEXT("Background color example"), style,
                         x, y, w, h, NULL, NULL, _hInstance, NULL);
}

void MainWindow::show(int nCmdShow)
{
    ShowWindow(_hwnd, nCmdShow);
}

void MainWindow::update()
{
    UpdateWindow(_hwnd);
}

#ifdef WINCE
#define LPXSTR LPTSTR
#else
#define LPXSTR LPSTR
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPXSTR lpCmdLine, INT nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;

    MainWindow win(hInstance);
    win.create();
    win.show(nCmdShow);
    win.update();

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

