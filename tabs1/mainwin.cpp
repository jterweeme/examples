#include "mainwin.h"
#include "winclass.h"
#include "toolbox.h"
#include "resource.h"
#include <commctrl.h>
#include <strsafe.h>
#include <iostream>

MainWin *MainWin::_instance;

MainWin::MainWin(WinClass *wc) :
    Window(wc, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480),
    _tc(this)
{
    _instance = this;
}

void MainWin::create()
{
    Window::create(L"Tab example");
}

void MainWin::_createProc(HWND hwnd)
{
    INITCOMMONCONTROLSEX iccex;
    iccex.dwSize = sizeof(iccex);
    iccex.dwICC = ICC_TAB_CLASSES;
    ::InitCommonControlsEx(&iccex);
    _tc.create(_wc->hInstance(), hwnd);
    TCITEMW tie;
    TCHAR achTemp[256];
    tie.mask = TCIF_TEXT | TCIF_IMAGE;
    tie.iImage = -1;
    tie.pszText = achTemp;
    StringCbPrintf(achTemp, 256, L"Maandag");
    _tc.insert(&tie);
    StringCbPrintf(achTemp, 256, L"Dinsdag");
    _tc.insert(&tie);
    _onSelChanged(hwnd);
}

INT_PTR CALLBACK dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        SetWindowPos(hwnd, NULL, 10, 40, 300, 300, SWP_SHOWWINDOW);
        return 0;
    }

    return 0;
    return ::DefDlgProc(hwnd, msg, wParam, lParam);
}

void MainWin::_onSelChanged(HWND)
{
    //int sel = _tc.getCurSel();

    HRSRC res = FindResource(0, MAKEINTRESOURCE(IDD_MAANDAG), RT_DIALOG);
    HGLOBAL hGlobal = LoadResource(hInstance(), res);

    LPCDLGTEMPLATE dlg = LPCDLGTEMPLATE(LockResource(hGlobal));

    ::CreateDialogIndirect(hInstance(), dlg, _tc.hwnd(), dlgProc);
}

LRESULT MainWin::_wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Toolbox t;

    switch (msg)
    {
    case WM_CREATE:
        _createProc(hwnd);
        break;
    case WM_SIZE:
        _tc.move(0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        break;
    case WM_NOTIFY:
    {
        std::cout << msg << " 0x" << t.hex32(lParam) << "\r\n";
        std::cout.flush();

        switch (lParam)
        {
            break;
        }
    }
        break;
    case WM_CLOSE:
        ::DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        break;
    default:
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK MainWin::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return _instance->_wndProc(hwnd, msg, wParam, lParam);
}

