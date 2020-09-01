// main.cpp
// TicTac1 - Simple tic-tac-toe game
//
// Written for the book Programming Windows CE
// Copyright (C) 2003 Douglas Boling
// 2020 Jasper ter Weeme

#include <windows.h>

#if __cplusplus >= 201103L
#define CONSTEXPR constexpr
#define OVERRIDE override
#else
#define CONSTEXPR const
#define OVERRIDE
#endif

class MainWindow
{
private:
    static MainWindow *_instance;
    HINSTANCE _hInstance;
    HWND _hwnd;
    LRESULT _wndProc(HWND, UINT, WPARAM, LPARAM);
    RECT rectBoard;
    RECT rectPrompt;
    BYTE bBoard[9];
    BYTE bTurn;

    void _sizeProc(HWND hwnd);
    void _drawBoard(HDC hdc, RECT *prect);
    void _drawXO(HDC hdc, HPEN, RECT *prect, INT nCell, INT nType);
    void _paintProc(HWND hwnd);
    void _lButtonUpProc(HWND hwnd, LPARAM lParam);
public:
    MainWindow(HINSTANCE hInstance);
    void create();
    void show(int nCmdShow);
    void update();
    static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);
};

MainWindow *MainWindow::_instance;

MainWindow::MainWindow(HINSTANCE hInstance)
    : _hInstance(hInstance), bTurn(0)
{
    _instance = this;
}

void MainWindow::show(int nCmdShow)
{
    ShowWindow(_hwnd, nCmdShow);
}

void MainWindow::update()
{
    UpdateWindow(_hwnd);
}

void MainWindow::_sizeProc(HWND hwnd)
{
    RECT rect;
    GetClientRect(hwnd, &rect);

    if (rectBoard.right == 0)
    {
        for (INT i = 0; i < 9; ++i)
            bBoard[i] = 0;
    }

    rectBoard = rect;
    rectPrompt = rect;

    if (rect.right - rect.left > rect.bottom - rect.top)
    {
        rectBoard.top += 10;
        rectBoard.left += 20;
        rectBoard.bottom -= 10;
        rectBoard.right = rectBoard.bottom - rectBoard.top + 10;
        rectPrompt.left = rectBoard.right + 10;
    }
    else
    {
        rectBoard.left += 20;
        rectBoard.right -= 20;
        rectBoard.top += 10;
        rectBoard.bottom = rectBoard.right - rectBoard.left + 10;
        rectPrompt.top = rectBoard.bottom + 10;
    }
}

void MainWindow::_drawXO(HDC hdc, HPEN, RECT *prect, INT nCell, INT nType)
{
    POINT pt[2];
    INT cx, cy;
    RECT rect;

    cx = (prect->right - prect->left)/3;
    cy = (prect->bottom - prect->top)/3;

    // Compute the dimensions of the target cell.
    rect.left = (cx * (nCell % 3) + prect->left) + 10;
    rect.right = rect.left + cx - 20;
    rect.top = cy * (nCell / 3) + prect->top + 10;
    rect.bottom = rect.top + cy - 20;

    // Draw an X ?
    if (nType == 1)
    {
        pt[0].x = rect.left;
        pt[0].y = rect.top;
        pt[1].x = rect.right;
        pt[1].y = rect.bottom;
        Polyline (hdc, pt, 2);

        pt[0].x = rect.right;
        pt[1].x = rect.left;
        Polyline (hdc, pt, 2);
    // How about an O ?
    }
    else if (nType == 2)
    {
        Ellipse(hdc, rect.left, rect.top, rect.right, rect.bottom);
    }
    return;
}

void MainWindow::_drawBoard(HDC hdc, RECT *prect)
{
    HPEN hPen, hOldPen;
    POINT pt[2];
    LOGPEN lp;
    INT i, cx, cy;
    lp.lopnStyle = PS_SOLID;
    lp.lopnWidth.x = 5;
    lp.lopnWidth.y = 5;
    lp.lopnColor = RGB (0, 0, 0);
    hPen = CreatePenIndirect (&lp);
    hOldPen = HPEN(SelectObject(hdc, hPen));
    cx = (prect->right - prect->left)/3;
    cy = (prect->bottom - prect->top)/3;
    pt[0].x = cx + prect->left;
    pt[1].x = cx + prect->left;
    pt[0].y = prect->top;
    pt[1].y = prect->bottom;
    Polyline (hdc, pt, 2);
    pt[0].x += cx;
    pt[1].x += cx;
    Polyline (hdc, pt, 2);
    pt[0].x = prect->left;
    pt[1].x = prect->right;
    pt[0].y = cy + prect->top;
    pt[1].y = cy + prect->top;
    Polyline (hdc, pt, 2);

    pt[0].y += cy;
    pt[1].y += cy;
    Polyline (hdc, pt, 2);

    for (i = 0; i < 9; i++)
        _drawXO(hdc, hPen, &rectBoard, i, bBoard[i]);

    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

void MainWindow::_paintProc(HWND hwnd)
{
    RECT rect;
    GetClientRect(hwnd, &rect);
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    _drawBoard(hdc, &rectBoard);
    HFONT hFont = HFONT(GetStockObject(SYSTEM_FONT));
    HFONT hOldFont = HFONT(SelectObject(hdc, hFont));

    if (bTurn == 0)
        DrawText(hdc, TEXT(" X turn"), -1, &rectPrompt, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    else
        DrawText(hdc, TEXT(" O turn"), -1, &rectPrompt, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, hOldFont);
    EndPaint(hwnd, &ps);
}

void MainWindow::_lButtonUpProc(HWND hwnd, LPARAM lParam)
{
    POINT pt;
    pt.x = LOWORD(lParam);
    pt.y = HIWORD(lParam);

    // See if pen on board.  If so, determine which cell.
    if (!PtInRect(&rectBoard, pt))
        return;

    // Normalize point to upper left corner of board.
    pt.x -= rectBoard.left;
    pt.y -= rectBoard.top;

    // Compute size of each cell.
    INT cx = (rectBoard.right - rectBoard.left)/3;
    INT cy = (rectBoard.bottom - rectBoard.top)/3;

    // Find column.
    INT nCell = 0;
    nCell = (pt.x / cx);
    // Find row.
    nCell += (pt.y / cy) * 3;

    //cell already filled
    if (bBoard[nCell] != 0)
    {
        MessageBeep(0);
        return;
    }

    if (bTurn)
    {
        bBoard[nCell] = 2;
        bTurn = 0;
    }
    else
    {
        bBoard[nCell] = 1;
        bTurn = 1;
    }
    InvalidateRect(hwnd, NULL, FALSE);
}

LRESULT MainWindow::_wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE:
        _sizeProc(hwnd);
        return 0;
    case WM_PAINT:
        _paintProc(hwnd);
        return 0;
    case WM_LBUTTONUP:
        _lButtonUpProc(hwnd, lParam);
        return 0;
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
    rectBoard.top = 0;
    rectBoard.left = 0;
    rectBoard.right = 0;
    rectBoard.bottom = 0;
    WNDCLASS wc;
    wc.style = 0;
    wc.lpfnWndProc = wndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = _hInstance;
    wc.hIcon = LoadIcon(_hInstance, TEXT("BALLICON"));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = HBRUSH(GetStockObject(WHITE_BRUSH));
    wc.lpszMenuName = NULL;
    wc.lpszClassName = TEXT("tictac");

    if (RegisterClass(&wc) == 0)
        throw TEXT("Cannot register class");

    CONSTEXPR DWORD style = WS_VISIBLE | WS_CAPTION | WS_SYSMENU;
    CONSTEXPR INT x = CW_USEDEFAULT;
    CONSTEXPR INT y = CW_USEDEFAULT;
    CONSTEXPR INT w = CW_USEDEFAULT;
    CONSTEXPR INT h = CW_USEDEFAULT;
    _hwnd = CreateWindowEx(0, wc.lpszClassName, TEXT("TicTacToe"), style, x, y, w, h, NULL, NULL, _hInstance, NULL);

    if (!IsWindow(_hwnd))
        throw TEXT("Cannot create main window");
}

#ifdef WINCE
#define LPXSTR LPTSTR
#else
#define LPXSTR LPSTR
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPXSTR cmdLine, INT nCmdShow)
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

    return 0;


}

