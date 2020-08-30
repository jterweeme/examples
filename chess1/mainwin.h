#ifndef MAINWIN_H
#define MAINWIN_H
#include "chess.h"

class AbstractHitTest;
class AbstractMenuBar;
class Board;
class Sim;
class WinClass;

class MainWindow
{
private:
    static CONSTEXPR LONG PIECE_XAXIS = 32, PIECE_YAXIS = 32;
    static short CONSTEXPR ConvertCoordToIndex(short x, short y) { return y * 8 + x; }

    WinClass *_wc;
    Sim *_sim;
    Board *_board;
    HWND _hwnd;
    int coords;
    int _firstSq;         /* Flag is a square is selected */
    int _gotFirst;
    int _editActive;   /* Edit mode? */
    int User_Move;     /* User or computer's turn */
    HMENU hMainMenu;
    HBRUSH _hBrushBackGround;
    COLORREF _clrBackGround, _clrBlackSquare, _clrWhiteSquare;
    COLORREF _clrBlackPiece, _clrWhitePiece, _clrText;
    HACCEL _hAccel;
    AbstractHitTest *_hitTest;
    PIECEBITMAP *_pieces;
    HWND _hComputerColor, _hMsgHuman;
    AbstractMenuBar *_menuBar;

    static void GiveHint(HWND hWnd);
    void _makeHelpPathName(TCHAR *szFileName);
    static MainWindow *_instance;
    LRESULT _wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void _createProc(HWND hwnd);
    void _paintProc(HWND hwnd) const;
    void _userMoveProc(HWND hwnd);
    void _commandProc(HWND hwnd, WPARAM wParam);
    void _initMenus(WPARAM wParam, LPARAM lParam);
    void _createChildren(HWND hWnd, HINSTANCE hInst, short xchar, short ychar);
    void _setStandardColors();
    void _getStartupColors();
    void _entryPoint(HWND hwnd);
    void _lButtonDownProc(HWND hwnd, LPARAM lParam);
    int _verifyMove(HWND hWnd, TCHAR *s, short iop, WORD *mv);
    static void QuerySqCenter(short x, short y, POINT *pptl);
    static void PieceOriginFromCenter(POINT *pptl);
    static void QuerySqPieceOrigin(short x, short y, POINT *pptl);
    static void ShowPiece(HDC hdc, POINT *pptl, PIECEBITMAP *Piece_bitmap, COLORREF color);
    static void DrawOnePiece(HDC hdc, short x, short y, PIECEBITMAP *piece, COLORREF color);

    static void DrawAllPieces(HDC hdc, PIECEBITMAP *pieces, int reverse,
                    short *pbrd, short *color, COLORREF xblack, COLORREF xwhite);
public:
    MainWindow(WinClass *wc, Sim *sim, HACCEL hAccel);
    HWND hwnd() const;
    HWND hComputerColor() const;
    void create(LPCTSTR caption);
    HINSTANCE hInstance() const;
    void show(int nCmdShow);
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

#endif

