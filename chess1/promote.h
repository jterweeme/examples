#ifndef PROMOTE_H
#define PROMOTE_H

#include <windows.h>

class PromoteDlg
{
private:
    HINSTANCE _hInstance;
    static PromoteDlg *_instance;
    INT_PTR _dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
public:
    PromoteDlg(HINSTANCE hInstance);
    INT_PTR run(HWND hwnd);
    static INT_PTR CALLBACK dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

#endif

