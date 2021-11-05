#ifndef MAINWIN_H
#define MAINWIN_H

#include "window.h"
#include "editbox.h"

class WinClass;

class MainWindow : public Window
{
private:
    static MainWindow *_instance;
    EditBox _editBox;
    BOOL _loadFile(HWND hEdit, LPCTSTR pszFileName);
    BOOL DoFileOpenSave(HWND hwnd, BOOL bSave);
    BOOL SaveFile(HWND hEdit, LPCTSTR pszFileName);
    LRESULT _wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
public:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    MainWindow(WinClass *wc);
    void create();
};

#endif

