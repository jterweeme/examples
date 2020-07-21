#ifndef MAINWIN_H
#define MAINWIN_H
#include "element.h"

class Toolbar;
class StatusBar;
class WinClass;

class MainWindow : public Element
{
private:
    WinClass *_wc;
    static MainWindow *_instance;
    Toolbar *_tb;
    StatusBar *_sb;
    LRESULT _wndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
    LRESULT _childProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
    void _sizeProc(HWND hwnd);
    void _createProc(HWND hwnd);
    void _commandProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
public:
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK childProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
    MainWindow(WinClass *wc);
    ~MainWindow();
    int alles(int nCmdShow);
};

#endif

