// CeChat - A Windows CE communication demo
//
// Written for the book Programming Windows CE
// Copyright (C) 2003 Douglas Boling
// 2020 Jasper ter Weeme

#include "resource.h"
#include <windows.h>
#include <commctrl.h>

static HINSTANCE hInst;

static LRESULT CALLBACK
wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {

    }
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDM_EXIT:
            SendMessage(hwnd, WM_CLOSE, 0, 0);
            return 0;
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

#ifdef WINCE
#define LPXSTR LPTSTR
#else
#define LPXSTR LPSTR
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        LPXSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    hInst = hInstance;
    WNDCLASS wc;
    wc.style = 0;
    wc.lpfnWndProc = wndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInst;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = HBRUSH(GetStockObject(WHITE_BRUSH));
    wc.lpszMenuName = NULL;
    wc.lpszClassName = TEXT("CeChat");

    if (RegisterClass(&wc) == 0)
        return 0;

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    HWND hwnd = CreateWindow(wc.lpszClassName, TEXT ("CeChat"),
                         WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                         CW_USEDEFAULT, CW_USEDEFAULT, NULL,
                         NULL, hInst, NULL);

    if (!IsWindow(hwnd))
        return 0;
#ifdef WINCE
    HWND hwndCB = CommandBar_Create(hInst, hwnd, IDC_CMDBAR);
    CommandBar_InsertMenubar(hwndCB, hInst, ID_MENU, 0);
    CommandBar_AddAdornments(hwndCB, 0, 0);
#endif
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

