#ifndef BOARD_H
#define BOARD_H

#include <windows.h>

extern void QueryBoardSize(POINT *pptl);
extern void QuerySqOrigin(short x, short y, POINT *pptl);
extern void QuerySqCoords(short x, short y, POINT aptl[]);

class Board
{
public:
    Board();
    void Draw_Board(HDC hDC, int reverse, COLORREF DarkColor, COLORREF LightColor);
    void DrawCoords(HDC hDC, int reverse, DWORD clrBackGround, DWORD clrText);
    void HiliteSquare(HWND hWnd, int Square);
    void DrawOneSquare(HDC hdc, short x, short y);
    void UnHiliteSquare(HWND hWnd, int square);
};

#endif

