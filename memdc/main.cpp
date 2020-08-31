// Win32_MemDC.cpp : Defines the entry point for the application.
//

#include "MemDcUsage.h"
#include "resource.h"
#include "dialog.h"

class MainWindow
{
private:
    static MainWindow *_instance;
    HINSTANCE _hInstance;
    HWND _hwnd;
    static const UINT_PTR k_animate = 2011;
    bool g_isDragging;
    bool g_isRightClick;
    POINT g_startPoint;
    POINT g_adjustedPoint;
    long g_adjustmentDiff;
    LRESULT _wndProc(HWND, UINT, WPARAM, LPARAM);
public:
    MainWindow(HINSTANCE hInstance);
    void create();
    void show(int nCmdShow);
    void update();
    static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);
};

MainWindow *MainWindow::_instance;

MainWindow::MainWindow(HINSTANCE hInstance)
  :
    _hInstance(hInstance),
    g_isDragging(false),
    g_isRightClick(false),
    g_adjustmentDiff(0)
{
    _instance = this;
    g_startPoint.x = 0;
    g_startPoint.y = 0;
    g_adjustedPoint.x = 0;
    g_adjustedPoint.y = 0;
}

void MainWindow::show(int nCmdShow)
{
    ShowWindow(_hwnd, nCmdShow);
}

void MainWindow::update()
{
    UpdateWindow(_hwnd);
}

LRESULT CALLBACK MainWindow::wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return _instance->_wndProc(hWnd, message, wParam, lParam);
}

LRESULT MainWindow::_wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;

    switch (message)
    {
    case WM_CREATE:
        // Create a time to run at the specified fps.
        ::SetTimer(hWnd, k_animate, GetFrameRateDelay(), NULL);
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDM_ABOUT:
        {
            AboutDlg aboutDlg(_hInstance);
            aboutDlg.run(hWnd);
        }
            break;
        case IDM_EXIT:
            ::DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_RBUTTONDOWN:
        ::SetCapture(hWnd);
        g_isRightClick = true;
        break;
    case WM_RBUTTONUP:
        if (g_isRightClick)
        {
            POINT pt = {LOWORD(lParam), HIWORD(lParam)};
            RECT  client;
            ::GetClientRect(hWnd, &client);
            if (::PtInRect(&client, pt))
            {
                ::ReleaseCapture();
                // Toggle the use of the back buffer.
                EnableBackBuffer(!IsBackBufferEnabled());
            }
        }

        g_isRightClick = false;
        break;
    case WM_LBUTTONDOWN:
        ::SetCapture(hWnd);

        // Mark the starting point of the drag operation.
        g_startPoint.x = LOWORD(lParam);
        g_startPoint.y = HIWORD(lParam);
        g_adjustedPoint = g_startPoint;
        g_adjustmentDiff = 0;
        g_isDragging = true;
        break;
    case WM_LBUTTONUP:
        ::ReleaseCapture();
        g_isDragging = false;
        break;
    case WM_MOUSEMOVE:
        if (g_isDragging)
        {
            // Move the position of the Highlight based on the users drag actions.
            int curX   = LOWORD(lParam);
            int offset = curX - (g_adjustedPoint.x - g_adjustmentDiff);
            int appliedOffset = AdjustAnimation(hWnd, offset);

            // Adjust the starting point by the applied offset
            // to keep the movements of the mouse relative to the animation.
            g_adjustmentDiff   = offset - appliedOffset;
            g_adjustedPoint.x += appliedOffset;
        }
        break;
    case WM_SIZE:
    case WM_SIZING:
        // Flush the back buffer,
        // The size of the image that needs to be painted has changed.
        FlushBackBuffer();

        // Allow the default processing to take care of the rest.
        return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_TIMER:
    {
        if (k_animate == wParam)
        {
            // Skip processing if the user is in dragging mode.
            if (!g_isDragging)
            {
                StepAnimation(hWnd);
            }
        }
    }
        break;
    case WM_ERASEBKGND:
        // Disable background erase functionality.
        // This will be taken care of in the WM_PAINT message.
        // The PAINTSTRUCT::fErase value will be TRUE by returning a non-zero value.
        return 1;
    case WM_PAINT:
    {
        HDC hdc = ::BeginPaint(hWnd, &ps);
        PaintAnimation(hWnd, hdc);
        ::EndPaint(hWnd, &ps);
    }
        break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

void MainWindow::create()
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = wndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = _hInstance;
    wcex.hIcon = ::LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_WIN32_MEMDC));
    wcex.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = HBRUSH(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCE(IDC_WIN32_MEMDC);
    wcex.lpszClassName = TEXT("ExampleClass");
    wcex.hIconSm = ::LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    if (!RegisterClassEx(&wcex))
        throw TEXT("Cannot create winclass");

    _hwnd = ::CreateWindow(wcex.lpszClassName, TEXT("Win32_MemDC"), WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, _hInstance, NULL);

    if (!_hwnd)
    {
        throw TEXT("Cannot create main window");
    }

    RECT client;
    ::GetClientRect(_hwnd, &client);
    Init(client.right - client.left, client.bottom - client.top);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
    MainWindow mainWin(hInstance);
    mainWin.create();
    mainWin.show(nCmdShow);
    mainWin.update();
    HACCEL hAccelTable = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WIN32_MEMDC));

    MSG msg;
    while (::GetMessage(&msg, NULL, 0, 0))
	{
        if (!::TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
		}
	}

    Term();
    return int(msg.wParam);
}





