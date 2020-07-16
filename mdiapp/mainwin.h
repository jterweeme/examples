#ifndef MAINWIN_H
#define MAINWIN_H
#include <windows.h>

#define ID_STATUSBAR       4997
#define ID_TOOLBAR         4998

#define ID_MDI_CLIENT      4999
#define ID_MDI_FIRSTCHILD  50000

#define IDC_CHILD_EDIT      2000

LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MDIChildWndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL LoadFile(HWND hEdit, LPCWSTR pszFileName);
BOOL GetFileName(HWND hwnd, LPWSTR pszFileName, BOOL bSave);
BOOL SaveFile(HWND hEdit, LPCWSTR pszFileName);

class WinClass;

class MainWindow
{
private:
    WinClass *_wc;
public:
    MainWindow(WinClass *wc);
    int alles(int nCmdShow);
};

#endif

