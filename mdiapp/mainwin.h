#ifndef MAINWIN_H
#define MAINWIN_H
#include <windows.h>

class Toolbar;
class WinClass;

class MainWindow
{
private:
    WinClass *_wc;
    static MainWindow *_instance;
    Toolbar *_tb;
    LRESULT _wndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
public:
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK childProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
    MainWindow(WinClass *wc);
    ~MainWindow();
    int alles(int nCmdShow);
};

#endif

