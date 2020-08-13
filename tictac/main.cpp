// TicTac1 - Simple tic-tac-toe game
//
// Written for the book Programming Windows CE
// Copyright (C) 2003 Douglas Boling
// 2020 Jasper ter Weeme

#include <windows.h>

static RECT rectBoard;
static RECT rectPrompt;
static BYTE bBoard[9];
static BYTE bTurn = 0;

static void sizeProc(HWND hwnd)
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

static void drawXO(HDC hdc, HPEN, RECT *prect, INT nCell, INT nType)
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

static void drawBoard(HDC hdc, RECT *prect)
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
        drawXO(hdc, hPen, &rectBoard, i, bBoard[i]);

    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

static void paintProc(HWND hwnd)
{
    RECT rect;
    GetClientRect(hwnd, &rect);
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    drawBoard(hdc, &rectBoard);
    HFONT hFont = HFONT(GetStockObject(SYSTEM_FONT));
    HFONT hOldFont = HFONT(SelectObject(hdc, hFont));

    if (bTurn == 0)
        DrawText(hdc, TEXT(" X turn"), -1, &rectPrompt, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    else
        DrawText(hdc, TEXT(" O turn"), -1, &rectPrompt, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, hOldFont);
    EndPaint(hwnd, &ps);
}

static void lButtonUpProc(HWND hwnd, LPARAM lParam)
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

static LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE:
        sizeProc(hwnd);
        return 0;
    case WM_PAINT:
        paintProc(hwnd);
        return 0;
    case WM_LBUTTONUP:
        lButtonUpProc(hwnd, lParam);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)cmdLine;
    rectBoard.top = 0;
    rectBoard.left = 0;
    rectBoard.right = 0;
    rectBoard.bottom = 0;
    WNDCLASS wc;
    wc.style = 0;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = HBRUSH(GetStockObject(WHITE_BRUSH));
    wc.lpszMenuName = NULL;
    wc.lpszClassName = TEXT("tictac");

    if (RegisterClass(&wc) == 0)
        throw TEXT("Cannot register class");

    const DWORD style = WS_VISIBLE | WS_CAPTION | WS_SYSMENU;
    HWND hwnd = CreateWindowEx(0, wc.lpszClassName, TEXT("TicTacToe"), style,
                               CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                               NULL, NULL, hInstance, NULL);

    if (!IsWindow(hwnd))
        throw TEXT("Cannot create main window");

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;


}

