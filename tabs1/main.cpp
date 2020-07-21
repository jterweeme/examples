#include "winclass.h"
#include "mainwin.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    WinClass wc(MainWin::wndProc, hInstance, L"MyName");
    MainWin win(&wc);

    try
    {
        wc.registerClass();
        win.create();
        win.show(nCmdShow);
        win.update();
    }
    catch (LPCSTR err)
    {
        MessageBoxA(0, err, "Error", 0);
    }
    catch (...)
    {
        MessageBoxW(0, L"Unkown Error", L"Error", 0);
    }


    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
    return msg.wParam;
}

