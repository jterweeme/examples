/*
  C source for GNU CHESS

  Revision: 1990-09-30  Daryl Baker


  Based on Ideas and code segments of Charles Petzold from artices in
  MicroSoft Systems Journal January 1990 Vol 5 Number1.


  This file is part of CHESS.

  CHESS is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY.  No author or distributor accepts responsibility to anyone for
  the consequences of using it or for whether it serves any particular
  purpose or works at all, unless he says so in writing.  Refer to the CHESS
  General Public License for full details.

  Everyone is granted permission to copy, modify and redistribute CHESS, but
  only under the conditions described in the CHESS General Public License.
  A copy of this license is supposed to have been given to you along with
  CHESS so you can know your rights and responsibilities.  It should be in a
  file named COPYING.  Among other things, the copyright notice and this
  notice must be preserved on all copies.
*/

#include "board.h"

Board::Board()
{

}

void Board::_drawOneSquare(HDC hdc, short x, short y)
{
    POINT aptl[4];
    QuerySqCoords(x, y, aptl);
    ::Polygon(hdc, aptl, 4);
}

void Board::QueryBoardSize(POINT *pptl)
{
    pptl->x = 2 * BRD_HORZMARGIN + 8 * BRD_HORZFRONT;
    pptl->y = BRD_BACKMARGIN + 8 * BRD_VERT + 2 * BRD_FRONTMARGIN + 2 * BRD_EDGE;
}

void Board::QuerySqOrigin(short x, short y, POINT *pptl)
{
    pptl->x = BRD_HORZMARGIN + y * (BRD_HORZFRONT - BRD_HORZBACK) / 2 +
             x * (y * BRD_HORZBACK + (8 - y) * BRD_HORZFRONT) / 8;

    pptl->y = (BRD_BACKMARGIN + 8 * BRD_VERT + BRD_FRONTMARGIN) - y * BRD_VERT;
}

//geeft de vier coordinaten van een enkele square in aptl[]
void Board::QuerySqCoords(short x, short y, POINT aptl[])
{
    QuerySqOrigin(x, y, aptl + 0);
    QuerySqOrigin(x + 1, y, aptl + 1);
    QuerySqOrigin(x + 1, y + 1, aptl + 2);
    QuerySqOrigin(x, y + 1, aptl + 3);
}

/*
   Draw the board. Pass the routine the upper left connor and the
   colors to draw the squares.
*/
void Board::Draw_Board(HDC hDC, int reverse, COLORREF darkColor, COLORREF lightColor)
{
    HBRUSH hBrush_lt = ::CreateSolidBrush(lightColor);
    HBRUSH hBrush_dk = ::CreateSolidBrush(darkColor);
    HBRUSH hOldBrush = HBRUSH(::SelectObject(hDC, hBrush_lt));
    HPEN hOldPen = HPEN(::SelectObject(hDC, ::GetStockObject(BLACK_PEN)));

    //draw squares
    for (int y = 0; y < 8; ++y)
    {
        for (int x = 0; x < 8; ++x)
        {
            if (reverse == 0)
            {
                ::SelectObject(hDC, (x + y) & 1 ? hBrush_lt : hBrush_dk);
                _drawOneSquare(hDC, x, y);
            }
            else
            {
                ::SelectObject(hDC, ((7 - x) + (7 - y)) & 1 ? hBrush_lt : hBrush_dk);
                _drawOneSquare(hDC, 7 - x, 7 - y);
            }
        }
    }

    /* Now draw the bottom edge of the board */
    for (int x = 0; x < 8; ++x)
    {
        POINT aptl[4];
        QuerySqCoords(x, 0, aptl);
        aptl[2].x = aptl[1].x;
        aptl[2].y = aptl[1].y + BRD_EDGE;
        aptl[3].x = aptl[0].x;
        aptl[3].y = aptl[0].y + BRD_EDGE;
        ::SelectObject(hDC, x & 1 ? hBrush_lt : hBrush_dk);
        ::Polygon(hDC, aptl, 4);
    }

    ::SelectObject(hDC, hOldPen);
    ::SelectObject(hDC, hOldBrush);
    ::DeleteObject(hBrush_lt);
    ::DeleteObject(hBrush_dk);
}

//geeft de nummers a1 t/m f8 weer
void Board::DrawCoords(HDC hDC, int reverse, COLORREF clrBackGround, COLORREF clrText)
{
    COLORREF OldBkColor = ::SetBkColor(hDC, clrBackGround);
    int OldBkMode = ::SetBkMode(hDC, TRANSPARENT);
    COLORREF OldTextColor = ::SetTextColor(hDC, clrText);
    TEXTMETRIC tm;
    ::GetTextMetrics(hDC, &tm);
    const short xchar = tm.tmMaxCharWidth;
    const short ychar = tm.tmHeight;

    for (int i = 0; i < 8; ++i)
    {
        POINT pt;
        QuerySqOrigin(0, i, &pt);
        const int x1 = pt.x - xchar;
        const int y1 = pt.y - BRD_VERT / 2 - ychar / 2;
        RECT rect;
        rect.top = y1;
        rect.left = x1;
        rect.right = x1 + xchar;
        rect.bottom = y1 + ychar;
        TCHAR numbers1[] = TEXT("12345678");
        TCHAR numbers2[] = TEXT("87654321");
        LPCTSTR numbers = reverse ? numbers2 + i : numbers1 + i;
        DrawText(hDC, numbers, 1, &rect, DT_LEFT);
        QuerySqOrigin(i, 0, &pt);
        const int x2 = pt.x + BRD_HORZFRONT / 2 - xchar / 2;
        const int y2 = pt.y + BRD_EDGE;
        rect.top = y2;
        rect.left = x2;
        rect.right = x2 + xchar;
        rect.bottom = y2 + ychar;
        TCHAR letters1[] = TEXT("abcdefgh");
        TCHAR letters2[] = TEXT("hgfedcba");
        LPCTSTR letters = reverse ? letters2 + i : letters1 + i;
        DrawText(hDC, letters, 1, &rect, DT_LEFT);
    }

    ::SetBkColor(hDC, OldBkColor);
    ::SetBkMode(hDC, OldBkMode);
    ::SetTextColor(hDC, OldTextColor);
}

void Board::HiliteSquare(HWND hWnd, int Square)
{

    POINT aptl[4];
    int y = Square / 8;
    int x = Square % 8;
    QuerySqCoords(x, y, aptl + 0);
    HRGN hRgn;
#ifdef WINCE
    hRgn = CreateRectRgn(aptl[0].x, aptl[0].y, aptl[2].x, aptl[2].y);
    HDC hDC = GetDC(hWnd);
    FillRgn(hDC, hRgn, CreateSolidBrush(RGB(255, 0, 0)));
    ReleaseDC(hWnd, hDC);
#else
    hRgn = CreatePolygonRgn(aptl, 4, WINDING);
    HDC hDC = GetDC(hWnd);
    InvertRgn(hDC, hRgn);
    ReleaseDC(hWnd, hDC);
#endif
    DeleteObject(hRgn);
    HilitSq = Square;
}

void Board::UnHiliteSquare(HWND hWnd, int square)
{
    if (HilitSq == -1)
        return;

    int y = square / 8;
    int x = square % 8;
    POINT aptl[4];
    QuerySqCoords(x, y, aptl + 0);
    HRGN hRgn;
#ifdef WINCE
#else
    hRgn = ::CreatePolygonRgn(aptl, 4, WINDING);
    HDC hDC = ::GetDC(hWnd);
    ::InvertRgn(hDC, hRgn);
    ::ReleaseDC(hWnd, hDC);
    ::DeleteObject(hRgn);
#endif
    HilitSq = -1;
}

