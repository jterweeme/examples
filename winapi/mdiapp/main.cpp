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
        WinClass wc1(MainWindow::wndProc, hInstance, TEXT("MyMDIWindow"), TEXT("MAIN"), HBRUSH(COLOR_3DSHADOW+1));
        WinClass wc2(MainWindow::childProc, hInstance, g_szChild, nullptr, HBRUSH(COLOR_3DFACE+1));
        MainWindow mainWindow(&wc1);
        InitCommonControls();
        wc1.registerClass();
        wc2.registerClass();
        mainWindow.alles(nCmdShow);
    }
    catch (...)
    {
        ::MessageBox(0, TEXT("Unknown Error"), TEXT("Error"), 0);
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


