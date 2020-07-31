#ifndef COLORDLG_H
#define COLORDLG_H

#include <windows.h>

class ColorDlg
{
private:
    HINSTANCE _hInstance;
    UINT _variant;
    static ColorDlg *_instance;
    void _initDialogProc(HWND hwnd);
    INT_PTR _commandProc(HWND hwnd, WPARAM wParam);
    INT_PTR _dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
public:
    ColorDlg(HINSTANCE hInstance, UINT variant);
    INT_PTR run(HWND hwnd, LPARAM lParam);
    static INT_PTR CALLBACK dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

#endif

