#ifndef MAINWIN_H
#define MAINWIN_H
#include <windows.h>

class Board;
class Sim;
class WinClass;

class MainWindow
{
private:
    WinClass *_wc;
    Sim *_sim;
    Board *_board;
    HWND _hwnd;
    int _firstSq;         /* Flag is a square is selected */
    int _gotFirst;
    int EditActive;   /* Edit mode? */
    int User_Move;     /* User or computer's turn */
    HMENU hMainMenu;
    void _makeHelpPathName(TCHAR *szFileName);
    static MainWindow *_instance;
    LRESULT _wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void _createProc(HWND hwnd);
    void _paintProc(HWND hwnd) const;
    void _userMoveProc(HWND hwnd);
    void _commandProc(HWND hwnd, WPARAM wParam);
    static void _initMenus(WPARAM wParam, LPARAM lParam);
    static void _createChildren(HWND hWnd, HINSTANCE hInst, short xchar, short ychar);
    static void _setStandardColors();
    static void _getStartupColors();
    HBRUSH _hBrushBackGround;
public:
    MainWindow(WinClass *wc, Sim *sim);
    HWND hwnd() const;
    void create(LPCTSTR caption);
    HINSTANCE hInstance() const;
    void show(int nCmdShow);
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

#endif

