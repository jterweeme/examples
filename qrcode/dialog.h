#ifndef DIALOG_H
#define DIALOG_H

#include <windows.h>

class NewDlg
{
private:
    static NewDlg *_instance;
    HINSTANCE _hInstance;
    LPSTR _buf;
    INT_PTR _dlgProc(HWND, UINT, WPARAM, LPARAM);
public:
    NewDlg(HINSTANCE hInstance, LPSTR buf);
    INT_PTR run(HWND hwnd);
    static INT_PTR CALLBACK dlgProc(HWND, UINT, WPARAM, LPARAM);
};

#endif

