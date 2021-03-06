#include "dialog.h"
#include "resource.h"
#include "toolbox.h"
#include <string>

NewDlg *NewDlg::_instance;

NewDlg::NewDlg(HINSTANCE hInstance, LPSTR buf) : _hInstance(hInstance), _buf(buf)
{
    _instance = this;
}

INT_PTR NewDlg::run(HWND hwnd)
{
    return DialogBox(_hInstance, MAKEINTRESOURCE(IDD_NEW), hwnd, dlgProc);
}

INT_PTR NewDlg::_dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            WCHAR buf[100];
            GetDlgItemTextW(hwnd, IDC_DATA, buf, 100);
            std::wstring ws = std::wstring(buf);
            std::string s = Toolbox::wstrtostr(ws);
            strncpy(_buf, s.c_str(), 100);
            EndDialog(hwnd, 1);
        }
            return TRUE;
        case IDCANCEL:
            EndDialog(hwnd, 0);
            return TRUE;
        }
    }

    return 0;
}

INT_PTR CALLBACK NewDlg::dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return _instance->_dlgProc(hwnd, msg, wParam, lParam);
}

