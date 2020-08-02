#ifndef MAINWIN_H
#define MAINWIN_H
#include "chess.h"
#include <windows.h>

class Board;
class Sim;
class WinClass;
class HitTest;

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
    void _createChildren(HWND hWnd, HINSTANCE hInst, short xchar, short ychar);
    void _setStandardColors();
    void _getStartupColors();
    int _verifyMove(HINSTANCE hInstance, HWND hWnd, TCHAR *s, short iop, WORD *mv);
    HBRUSH _hBrushBackGround;
    COLORREF _clrBackGround, _clrBlackSquare, _clrWhiteSquare;
    COLORREF _clrBlackPiece, _clrWhitePiece, _clrText;
    HACCEL _hAccel;
    HitTest *_hitTest;
    PIECEBITMAP *_pieces;
    HWND _hComputerColor;
public:
    MainWindow(WinClass *wc, Sim *sim);
    HWND hwnd() const;
    HWND hComputerColor() const;
    void create(LPCTSTR caption);
    HINSTANCE hInstance() const;
    HACCEL hAccel() const;
    void show(int nCmdShow);
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

#endif

