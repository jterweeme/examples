#ifndef BOARD_H
#define BOARD_H

#include <windows.h>

class Board
{
public:
    Board();
    void Draw_Board(HDC hDC, int reverse, COLORREF DarkColor, COLORREF LightColor);
    void DrawCoords(HDC hDC, int reverse, DWORD clrBackGround, DWORD clrText);
};

#endif

