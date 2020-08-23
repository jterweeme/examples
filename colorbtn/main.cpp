#include <windows.h>
#include <iostream>

class Button
{
private:
    static Button *_instance;
    HINSTANCE _hInstance;
    HWND _hwnd;
    INT _x, _y, _w, _h;
    WNDPROC _originalWndProc;

    LRESULT _wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
public:
    Button(HINSTANCE hInstance, INT x, INT y, INT w, INT h);
    void create(HWND hParent);
    static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);
    void colorize();
};

Button *Button::_instance;

Button::Button(HINSTANCE hInstance, INT x, INT y, INT w, INT h)
    : _hInstance(hInstance), _x(x), _y(y), _w(w), _h(h)
{
    _instance = this;
}

LRESULT Button::_wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        //HDC hdc = BeginPaint(hwnd, &ps);
        //::SetBkColor(hdc, RGB(255,0,0));
        //EndPaint(hwnd, &ps);
        std::cout << "Debug bericht\n";
        std::cout.flush();
    }
        break;
    }

    return CallWindowProc(_originalWndProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK Button::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return _instance->_wndProc(hwnd, msg, wParam, lParam);
}

void Button::colorize()
{
    SetWindowLong(_hwnd, GWL_WNDPROC, LONG(wndProc));
}

void Button::create(HWND hParent)
{
    const DWORD style = WS_VISIBLE | WS_CHILD;
    _hwnd = CreateWindow(TEXT("BUTTON"), TEXT("Dinges"), style, _x, _y, _w, _h, hParent, NULL, _hInstance, NULL);

    if (_hwnd == NULL)
        throw TEXT("Cannot create button");

    _originalWndProc = WNDPROC(GetWindowLong(_hwnd, GWL_WNDPROC));
}

class MainWindow
{
private:
    static MainWindow *_instance;
    HINSTANCE _hInstance;
    HWND _hwnd;
    Button *_button;

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
        _button = new Button(_hInstance, 10, 10, 100, 30);
        _button->create(hwnd);
        _button->colorize();
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
    WNDCLASS wc;
    wc.hIcon = NULL;
    wc.style = 0;
    wc.hCursor = NULL;
    wc.hInstance = _hInstance;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.lpszMenuName = NULL;
    wc.hbrBackground = HBRUSH(CreateSolidBrush(0xffffff));
    wc.lpszClassName = TEXT("classname");
    wc.lpfnWndProc = wndProc;

    if (!RegisterClass(&wc))
        throw TEXT("Cannot register winclass");

    const DWORD style = WS_OVERLAPPEDWINDOW;
    const INT x = CW_USEDEFAULT;
    const INT y = CW_USEDEFAULT;
    const INT w = CW_USEDEFAULT;
    const INT h = CW_USEDEFAULT;
    _hwnd = CreateWindow(wc.lpszClassName, TEXT("Colorbutton"), style, x, y, w, h, NULL, NULL, _hInstance, NULL);

    if (_hwnd == NULL)
        throw TEXT("Cannot create main window");
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

