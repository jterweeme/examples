#ifndef MAINWIN_H
#define MAINWIN_H

#include "window.h"
#include "tabcontrol.h"

class MainWin : public Window
{
private:
    static MainWin *_instance;
    LRESULT _wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    HWND _hwndTab;
    TabControl _tc;
    void _createProc(HWND hwnd);
    void _onSelChanged(HWND hwnd);
public:
    MainWin(WinClass *wc);
    void create();
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

#endif

