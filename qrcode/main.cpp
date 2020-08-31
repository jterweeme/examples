#include "mainwin.h"

#ifdef WINCE
#define LPXSTR LPTSTR
#else
#define LPXSTR LPSTR
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPXSTR lpCmdLine, INT nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
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

