#include <windows.h>
#include <iostream>

class WinClass
{
private:
    WNDCLASS _wc;
public:
    WinClass(HINSTANCE hInstance, LPCTSTR className, WNDPROC wndProc, HBRUSH bg);
    void registerClass();
    LPCTSTR className() const;
};

LPCTSTR WinClass::className() const
{
    return _wc.lpszClassName;
}

void WinClass::registerClass()
{
    RegisterClass(&_wc);
}

WinClass::WinClass(HINSTANCE hInstance, LPCTSTR className, WNDPROC wndProc, HBRUSH bg)
{
    _wc.hInstance = hInstance;
    _wc.style = CS_HREDRAW | CS_VREDRAW;
    _wc.lpfnWndProc = wndProc;
    _wc.lpszClassName = className;
    _wc.lpszMenuName = NULL;
    _wc.cbClsExtra = 0;
    _wc.cbWndExtra = 0;
    _wc.hIcon = 0;
    _wc.hbrBackground = bg;
    _wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
}

class ButtonClass
{
private:
    HINSTANCE _hInstance;
    static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);
public:
    ButtonClass(HINSTANCE hInstance);
    void registerClass();
};

LRESULT CALLBACK ButtonClass::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        return 0;
    case WM_PAINT:
    {
#if 0
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT r;
        r.top = 0;
        r.left = 0;
        r.right = 100;
        r.bottom = 100;
        FillRect(hdc, &r, HBRUSH(GetStockObject(GRAY_BRUSH)));
        EndPaint(hwnd, &ps);
#endif
    }
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

ButtonClass::ButtonClass(HINSTANCE hInstance) : _hInstance(hInstance)
{

}

void ButtonClass::registerClass()
{
    WinClass wc(_hInstance, TEXT("ColorButton"), wndProc, HBRUSH(GetStockObject(GRAY_BRUSH)));
    wc.registerClass();
}

class MainWindow
{
private:
    static MainWindow *_instance;
    HINSTANCE _hInstance;
    HWND _hwnd;
    //Button *_button;

    LRESULT _wndProc(HWND, UINT, WPARAM, LPARAM);
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

LRESULT MainWindow::_wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        break;
    case WM_PAINT:
        break;
    case WM_CTLCOLORSTATIC:
        break;
    case WM_ERASEBKGND:
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK MainWindow::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret;

    try
    {
        ret = _instance->_wndProc(hwnd, msg, wParam, lParam);
    }
    catch (...)
    {

    }
    return ret;
}

void MainWindow::create()
{
    WinClass wc(_hInstance, TEXT("MainWindow"), wndProc, HBRUSH(COLOR_WINDOW + 1));
    wc.registerClass();

    const DWORD style = WS_OVERLAPPEDWINDOW;
    const INT x = CW_USEDEFAULT;
    const INT y = CW_USEDEFAULT;
    const INT w = CW_USEDEFAULT;
    const INT h = CW_USEDEFAULT;
    _hwnd = CreateWindow(wc.className(), TEXT("Colorbutton"), style, x, y, w, h, NULL, NULL, _hInstance, NULL);

    if (_hwnd == NULL)
        throw TEXT("Cannot create main window");

    CreateWindow(TEXT("ColorButton"), TEXT("onzin"), WS_VISIBLE | WS_CHILD, 0, 0, 100, 100, _hwnd, NULL, _hInstance, NULL);
}

void MainWindow::show(int nCmdShow)
{
    ::ShowWindow(_hwnd, nCmdShow);
}

void MainWindow::update()
{
    ::UpdateWindow(_hwnd);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    ButtonClass btc(hInstance);
    btc.registerClass();
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

