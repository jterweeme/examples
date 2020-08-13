/* 
   Name: WinAnim
   Author: Brook Miles, Jasper ter Weeme
   Description: Making an animation in windows
*/

#include "winclass.h"
#include "mainwin.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    WinClass wc(hInstance, MainWin::wndProc, L"MyWindowClass");
    MainWin win(&wc);
    wc.registerClass();
    win.create();
    win.show(nCmdShow);
    win.update();

    MSG Msg;
    while (GetMessage(&Msg, NULL, 0, 0))
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}

