#include "winclass.h"
#include "mainwin.h"
#include "mdi_unit.h"
#include "main.h"
#include <commctrl.h>

wchar_t g_szChild[] = L"MyMDIChild";
HWND g_hMDIClient;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpszCmdParam, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpszCmdParam;

    try
    {
        WinClass wc1(MainWindow::wndProc, hInstance, L"MyMDIWindow", L"MAIN", HBRUSH(COLOR_3DSHADOW+1));
        WinClass wc2(MainWindow::childProc, hInstance, g_szChild, nullptr, HBRUSH(COLOR_3DFACE+1));
        InitCommonControls();
        wc1.registerClass();
        wc2.registerClass();
        MainWindow mainWindow(&wc1);
        mainWindow.alles(nCmdShow);
    }
    catch (...)
    {
        ::MessageBoxW(0, L"Unknown Error", L"Error", 0);
    }

    MSG Msg;
    while (GetMessage(&Msg, NULL, 0, 0))
    {
        if (!TranslateMDISysAccel(g_hMDIClient, &Msg))
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
    }
    return Msg.wParam;
}


