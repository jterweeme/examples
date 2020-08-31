#include "dialog.h"
#include "resource.h"

AboutDlg *AboutDlg::_instance;

AboutDlg::AboutDlg(HINSTANCE hInstance) : _hInstance(hInstance)
{
    _instance = this;
}

INT_PTR AboutDlg::run(HWND hwnd)
{
    return ::DialogBox(_hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, dlgProc);
}

INT_PTR AboutDlg::dlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        return INT_PTR(TRUE);
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            ::EndDialog(hDlg, LOWORD(wParam));
            return INT_PTR(TRUE);
        }
        break;
    }
    return INT_PTR(FALSE);
}

