#ifndef MAINWIN_H
#define MAINWIN_H

#include <windows.h>

class AbstractMenuBar;

class MainWindow
{
private:
    static MainWindow *_instance;
    HINSTANCE _hInstance;
    HWND _hwnd;
    AbstractMenuBar *_menuBar;
    void _commandProc(HWND hwnd, WPARAM wParam);
    LRESULT _wndProc(HWND, UINT, WPARAM, LPARAM);
public:
    MainWindow(HINSTANCE hInstance);
    void create();
    void show(int nCmdShow);
    void update();
    static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);
};

#endif

