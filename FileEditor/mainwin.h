#ifndef MAINWIN_H
#define MAINWIN_H

#include <windows.h>

class WinClass;

class MainWindow
{
private:
    static MainWindow *_instance;
    WinClass *_wc;
    HWND _hwnd;
    BOOL LoadFile(HWND hEdit, LPWSTR pszFileName);
    BOOL DoFileOpenSave(HWND hwnd, BOOL bSave);
    BOOL SaveFile(HWND hEdit, LPWSTR pszFileName);
    LRESULT _wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
public:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    MainWindow(WinClass *wc);
    int create();
    void show(int nCmdShow);
    void update();
};

#endif

