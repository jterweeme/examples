#include "winclass.h"
#include "mainwin.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    WinClass wc(MainWindow::WndProc, hInstance, L"MyWindowClass");
    wc.registerClass();
    MainWindow win(&wc);
    win.create();
    win.show(nCmdShow);
    win.update();

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
    return msg.wParam;
}


