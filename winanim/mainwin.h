#ifndef MAINWIN_H
#define MAINWIN_H

#include "ball.h"

class WinClass;

class MainWin
{
private:
    static MainWin *_instance;
    HWND _hwnd;
    WinClass *_wc;
    LRESULT _wndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
    Ball _ball;
public:
    MainWin(WinClass *wc);
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
    void create();
    void show(int nCmdShow);
    void update();
};

#endif

