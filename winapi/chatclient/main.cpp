//chatclient
//main.cpp

#include "menubar.h"
#include "resource.h"

#ifdef WINCE
#define LPXSTR LPTSTR
#else
#define LPXSTR LPSTR
#endif

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
    LRESULT _wndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT _commandProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void _createProc(HWND hwnd);
public:
    MainWindow(HINSTANCE hInstance);
    HINSTANCE hInstance() const;
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

HINSTANCE MainWindow::hInstance() const
{
    return _hInstance;
}

void MainWindow::create()
{
    WNDCLASS wc;
    wc.hIcon = NULL;
    wc.style = 0;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hInstance = _hInstance;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.lpfnWndProc = wndProc;
    wc.lpszMenuName = NULL;
    wc.hbrBackground = HBRUSH(GetStockObject(WHITE_BRUSH));
    wc.lpszClassName = TEXT("MainWindowClass");

    if (RegisterClass(&wc) == 0)
        throw TEXT("Cannot register winclass");

    CONSTEXPR DWORD style = WS_VISIBLE | WS_SYSMENU;
    CONSTEXPR INT x = CW_USEDEFAULT;
    CONSTEXPR INT y = CW_USEDEFAULT;
    CONSTEXPR INT w = CW_USEDEFAULT;
    CONSTEXPR INT h = CW_USEDEFAULT;
    _hwnd = CreateWindow(wc.lpszClassName, TEXT("Chat Client"), style, x, y, w, h, 0, NULL, _hInstance, 0);

    if (_hwnd == 0)
        throw TEXT("Cannot create main window");
}

void MainWindow::_createProc(HWND hwnd)
{
    _menuBar = new MenuBar(hInstance());
    _menuBar->enable(hwnd);
}

LRESULT MainWindow::_commandProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
    case IDM_EXIT:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT MainWindow::_wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        _createProc(hwnd);
        return 0;
    case WM_COMMAND:
        return _commandProc(hwnd, msg, wParam, lParam);
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

void MainWindow::show(int nCmdShow)
{
    ShowWindow(_hwnd, nCmdShow);
}

void MainWindow::update()
{
    UpdateWindow(_hwnd);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPXSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    MainWindow mainWin(hInstance);
    mainWin.create();
    mainWin.show(nCmdShow);
    mainWin.update();

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

