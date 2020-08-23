#ifndef DIALOG_H
#define DIALOG_H

#include <windows.h>

class Dialog
{
public:
    Dialog(HINSTANCE hInstance);
};

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

class ColorDlg
{
private:
    HINSTANCE _hInstance;
    UINT _variant;
    COLORREF * const _pclr;
    int _index;
    static ColorDlg *_instance;
    void _initDialogProc(HWND hwnd);
    INT_PTR _commandProc(HWND hwnd, WPARAM wParam);
    INT_PTR _dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
public:
    ColorDlg(HINSTANCE hInstance, UINT variant, COLORREF *item);
    INT_PTR run(HWND hwnd, LPARAM lParam);
    static INT_PTR CALLBACK dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

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

