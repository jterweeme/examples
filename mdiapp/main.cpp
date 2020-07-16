#include "winclass.h"
#include "mainwin.h"
#include "mdi_unit.h"
#include "main.h"
#include <commctrl.h>

wchar_t g_szChild[] = L"MyMDIChild";
HINSTANCE g_hInst;
HWND g_hMDIClient;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpszCmdParam, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpszCmdParam;
    InitCommonControls();
    g_hInst = hInstance;
    WinClass wc1(WndProc, hInstance, L"MyMDIWindow", L"MAIN", (HBRUSH)(COLOR_3DSHADOW+1));
    wc1.registerClass();
    WinClass wc2(MDIChildWndProc, hInstance, g_szChild, nullptr, (HBRUSH)(COLOR_3DFACE+1));
    wc2.registerClass();
    MainWindow mainWindow(&wc1);
    mainWindow.alles(nCmdShow);

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


