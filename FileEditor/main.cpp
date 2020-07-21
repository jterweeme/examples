#include "winclass.h"
#include "mainwin.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    WinClass wc(MainWindow::WndProc, hInstance, L"MyWindowClass");
    MainWindow win(&wc);

    try
    {
        wc.registerClass();
        win.create();
        win.show(nCmdShow);
        win.update();
    }
    catch (LPCSTR err)
    {
        ::MessageBoxA(0, err, "Error", 0);
    }
    catch (...)
    {
        ::MessageBoxW(0, L"Unknown Error", L"Error", 0);
    }


    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
    return msg.wParam;
}


