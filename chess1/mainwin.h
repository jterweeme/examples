#ifndef MAINWIN_H
#define MAINWIN_H
#include <windows.h>

class WinClass;

class MainWindow
{
private:
    WinClass *_wc;
    HWND _hwnd;
    int FirstSq;         /* Flag is a square is selected */
    int GotFirst;
    int EditActive;   /* Edit mode? */
    int User_Move;     /* User or computer's turn */
    HMENU hMainMenu;
    void MakeHelpPathName(TCHAR *szFileName);
    static MainWindow *_instance;
    LRESULT _wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void _createProc(HWND hwnd, LPCTSTR appName);
    void _paintProc(HWND hwnd) const;
    void _userMoveProc(HWND hwnd);
    void _commandProc(HWND hwnd, WPARAM wParam, LPCTSTR appName);
    static void _initMenus(WPARAM wParam, LPARAM lParam);
    HBRUSH hBrushBackGround;
public:
    MainWindow(WinClass *wc);
    HWND hwnd() const;
    void create(LPCTSTR appName);
    HINSTANCE hInstance() const;
    void show(int nCmdShow);
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

#endif

