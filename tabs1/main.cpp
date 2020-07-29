#include "winclass.h"
#include "mainwin.h"
#include "toolbox.h"
#include "resource.h"

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
        Toolbox().errorBoxA(0, err);
        return -1;
    }
    catch (LPCWSTR err)
    {
        Toolbox().errorBoxW(0, err);
        return -1;
    }
    catch (...)
    {
        Toolbox().errorBox(hInstance, 0, IDS_UNKNOWNERR);
        return -1;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
    return msg.wParam;
}

