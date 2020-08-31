#ifndef DIALOG_H
#define DIALOG_H

#include <windows.h>

class AboutDlg
{
private:
    static AboutDlg *_instance;
    HINSTANCE _hInstance;
public:
    AboutDlg(HINSTANCE hInstance);
    INT_PTR run(HWND hwnd);
    static INT_PTR CALLBACK dlgProc(HWND, UINT, WPARAM, LPARAM);
};

#endif

