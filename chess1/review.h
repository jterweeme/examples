#ifndef REVIEW_H
#define REVIEW_H

#include <windows.h>

class ReviewDialog
{
private:
    static ReviewDialog *_instance;
    HINSTANCE _hInstance;
    INT_PTR _dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
public:
    ReviewDialog(HINSTANCE hInstance);
    INT_PTR run(HWND hwnd);
    static INT_PTR CALLBACK dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

#endif

