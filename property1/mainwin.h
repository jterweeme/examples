#ifndef MAINWIN_H
#define MAINWIN_H

#include "window.h"

class WinClass;

class MainWindow : public Window
{
private:
    static MainWindow *_instance;
    LRESULT _wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
public:
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    MainWindow(WinClass *wc);
    void create();
};

#endif

