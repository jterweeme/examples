#ifndef MANUAL_H
#define MANUAL_H

#include <windows.h>

class ManualDlg
{
private:
    static ManualDlg *_instance;
    HINSTANCE _hInstance;
    INT_PTR _dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
public:
    ManualDlg(HINSTANCE hInstance);
    INT_PTR run(HWND hwnd, TCHAR *szPrompt);
    static INT_PTR CALLBACK dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

#endif

