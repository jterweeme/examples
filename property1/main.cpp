#include "resource.h"
#include "winclass.h"
#include "mainwin.h"
#include "toolbox.h"
#include <windowsx.h>
#include <commctrl.h>

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;

    WinClass wc(MainWindow::wndProc, hInstance, TEXT("PropSheetClass"));
    MainWindow win(&wc);

    try
    {
        InitCommonControls();
        wc.registerClass();
        win.create();
        win.show(nCmdShow);
        win.update();
    }
    catch (...)
    {
        Toolbox().errorBox(hInstance, 0, IDS_UNKNOWNERR);
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0x00, 0x00))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return int(msg.wParam);
}

