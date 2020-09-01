#include "winclass.h"
#include "mainwin.h"

#ifdef WINCE
#define LPXSTR LPTSTR
#else
#define LPXSTR LPSTR
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPXSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    WinClass wc(MainWindow::WndProc, hInstance, L"MyWindowClass");
    MainWindow win(&wc);

    try
    {
        wc.registerClass();
        win.create();
        win.show(nCmdShow);
        win.update();
    }
#ifndef WINCE
    catch (LPCSTR err)
    {
        ::MessageBoxA(NULL, err, "Error", 0);
    }
#endif
    catch (LPCWSTR err)
    {
        ::MessageBoxW(NULL, err, L"Error", 0);
    }
    catch (...)
    {
        ::MessageBox(NULL, TEXT("Unknown Error"), TEXT("Error"), 0);
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
    return msg.wParam;
}


