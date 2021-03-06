#ifndef DIALOG_H
#define DIALOG_H

#include <windows.h>

class Dialog
{
private:
    HINSTANCE _hInstance;
public:
    Dialog(HINSTANCE hInstance);
    HINSTANCE hInstance() const;
};

class AboutDlg : public Dialog
{
public:
    AboutDlg(HINSTANCE hInstance);
    INT_PTR run(HWND hwnd);
    static INT_PTR CALLBACK dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
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

class ColorDlg : public Dialog
{
private:
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

class NumDlg : public Dialog
{
private:
    static NumDlg *_instance;
    INT_PTR _dlgProc(HWND, UINT, WPARAM, LPARAM);
public:
    NumDlg(HINSTANCE hInstance);
    static INT_PTR CALLBACK dlgProc(HWND, UINT, WPARAM, LPARAM);
    int getInt(HWND hwnd, TCHAR *szPrompt, int def);
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
    static INT_PTR CALLBACK dlgProc(HWND, UINT, WPARAM, LPARAM);
};

class Sim;

class TestDlg : public Dialog
{
private:
    static TestDlg *_instance;
    INT_PTR _dlgProc(HWND, UINT, WPARAM, LPARAM);
    Sim *_sim;
public:
    TestDlg(HINSTANCE hInstance, Sim *sim);
    INT_PTR run(HWND hwnd);
    static INT_PTR CALLBACK dlgProc(HWND, UINT, WPARAM, LPARAM);
};

class TimeCtrlDlg : public Dialog
{
private:
    static TimeCtrlDlg *_instance;
public:
    TimeCtrlDlg(HINSTANCE hInstance);
    INT_PTR run(HWND hwnd, LPARAM param);
    static INT_PTR CALLBACK dlgProc(HWND, UINT, WPARAM, LPARAM);
};

#endif

