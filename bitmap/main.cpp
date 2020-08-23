#include "resource.h"
#include <windows.h>

class MainWindow
{
private:
    static MainWindow *_instance;
    HINSTANCE _hInstance;
    HWND _hwnd;
    HBITMAP _hCastle;
    BITMAP _bCastle;
    LRESULT _wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
public:
    MainWindow(HINSTANCE hInstance);
    void create();
    void show(int nCmdShow);
    void update();
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
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
        _hCastle = LoadBitmap(_hInstance, MAKEINTRESOURCE(IDB_CASTLE));
        //_hCastle = LoadBitmap(_hInstance, MAKEINTRESOURCE(53));

        if (_hCastle == NULL)
            throw TEXT("Cannot load bitmap");

        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        GetObject(_hCastle, sizeof(_bCastle), &_bCastle);
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hBitmap = HBITMAP(SelectObject(hdcMem, _hCastle));
        BitBlt(hdc, 10, 10, _bCastle.bmWidth, _bCastle.bmHeight, hdcMem, 0, 0, SRCCOPY);
        SelectObject(hdcMem, hBitmap);
        DeleteDC(hdcMem);
        EndPaint(hwnd, &ps);
    }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void MainWindow::create()
{
    WNDCLASS wc;
    wc.hIcon = NULL;
    wc.style = 0;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hInstance = _hInstance;
    wc.lpfnWndProc = wndProc;
    wc.hbrBackground = HBRUSH(GetStockObject(WHITE_BRUSH));
    wc.lpszClassName = TEXT("WinClass");
    wc.lpszMenuName = NULL;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;

    if (RegisterClass(&wc) == 0)
        throw TEXT("Cannot register window class");

    _hwnd = CreateWindow(wc.lpszClassName, TEXT("Bitmap Example"), WS_VISIBLE,
                         CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                         NULL, NULL, _hInstance, NULL);

    if (!IsWindow(_hwnd))
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

LRESULT CALLBACK MainWindow::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret = 0;

    try {
        ret = _instance->_wndProc(hwnd, msg, wParam, lParam);
    }
    catch (LPCWSTR err)
    {
        MessageBox(hwnd, err, L"Error", 0);
    }
    catch (...)
    {
        MessageBox(hwnd, TEXT("Unknown Error"), TEXT("Error"), 0);
    }

    return ret;
}

#ifdef WINCE
#define LPXSTR LPTSTR
#else
#define LPXSTR LPSTR
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPXSTR cmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)cmdLine;
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

    return int(msg.wParam);
}

