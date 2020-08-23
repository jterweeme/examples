#ifndef BOARD_H
#define BOARD_H

#include "chess.h"

class Board
{
private:
    static CONSTEXPR LONG BRD_HORZFRONT = 48, BRD_HORZBACK = 32, BRD_VERT = 32;
    static CONSTEXPR LONG BRD_EDGE = 8, BRD_HORZMARGIN = 32, BRD_BACKMARGIN = 5, BRD_FRONTMARGIN = 5;
public:
    static void QueryBoardSize(POINT *pptl);
    static void QuerySqOrigin(short x, short y, POINT *pptl);
    static void QuerySqCoords(short x, short y, POINT aptl[]);
    Board();
    void Draw_Board(HDC hDC, int reverse, COLORREF DarkColor, COLORREF LightColor);
    void DrawCoords(HDC hDC, int reverse, DWORD clrBackGround, DWORD clrText);
    void HiliteSquare(HWND hWnd, int Square);
    void DrawOneSquare(HDC hdc, short x, short y);
    void UnHiliteSquare(HWND hWnd, int square);
};

#endif

