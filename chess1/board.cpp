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

#include "protos.h"

/* All units defined in pixels */

#define BRD_HORZFRONT   48
#define BRD_HORZBACK    32
#define BRD_VERT        32
#define BRD_EDGE        8
#define BRD_HORZMARGIN  32
#define BRD_BACKMARGIN  5
#define BRD_FRONTMARGIN 5

static void DrawOneSquare(HDC hDC, short x, short y);
static int HilitSq;

void QueryBoardSize(POINT *pptl)
{
    pptl->x = 2*BRD_HORZMARGIN + 8*BRD_HORZFRONT;
    pptl->y = BRD_BACKMARGIN + 8*BRD_VERT + 2*BRD_FRONTMARGIN + 2*BRD_EDGE;
}

void QuerySqSize(POINT *pptl)
{
    pptl->x = BRD_HORZFRONT;
    pptl->y = BRD_VERT;
}

void QuerySqOrigin ( short x, short y, POINT *pptl)
{
   pptl->x = BRD_HORZMARGIN + y * (BRD_HORZFRONT-BRD_HORZBACK)/2 +
             x * (y*BRD_HORZBACK + (8-y)*BRD_HORZFRONT)/8;
   pptl->y = (BRD_BACKMARGIN+8*BRD_VERT+BRD_FRONTMARGIN)  - y*BRD_VERT;
}

void QuerySqCoords(short x, short y, POINT aptl[])
{
    QuerySqOrigin(x,  y,  aptl+0);
    QuerySqOrigin(x+1,y,  aptl+1);
    QuerySqOrigin(x+1,y+1,aptl+2);
    QuerySqOrigin(x,  y+1,aptl+3);
}

static void DrawOneSquare ( HDC hDC, short x, short y)
{
   POINT aptl[4];

   QuerySqCoords ( x,y, aptl);
   Polygon( hDC, aptl, 4);
}

/*
   Draw the board.  Pass the routine the upper left connor and the
   colors to draw the squares.
*/

void Draw_Board(HDC hDC, int reverse, DWORD DarkColor, DWORD LightColor )
{
    POINT aptl[4];

    HBRUSH hBrush_lt = ::CreateSolidBrush(LightColor);
    HBRUSH hBrush_dk = ::CreateSolidBrush(DarkColor);

    HBRUSH hOldBrush = HBRUSH(::SelectObject(hDC, hBrush_lt));
    HPEN hOldPen = HPEN(::SelectObject(hDC, ::GetStockObject(BLACK_PEN)));
    int OldPolyMode = ::SetPolyFillMode(hDC, WINDING);

    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8; x++)
        {
            if (reverse == 0)
            {
                SelectObject(hDC, ((x+y)&1) ? hBrush_lt : hBrush_dk);
                DrawOneSquare(hDC, x, y);
            }
            else
            {
                SelectObject(hDC, (((7-x)+(7-y))&1) ? hBrush_lt : hBrush_dk);
                DrawOneSquare(hDC, 7-x, 7-y);
            }
        }
    }

    /* Now draw the bottom edge of the board */
    for (int x = 0; x < 8; x++)
    {
        QuerySqCoords(x, 0, aptl);
        aptl[2].x = aptl[1].x;
        aptl[2].y = aptl[1].y + BRD_EDGE;
        aptl[3].x = aptl[0].x;
        aptl[3].y = aptl[0].y + BRD_EDGE;
        SelectObject(hDC, x & 1 ? hBrush_lt : hBrush_dk);
        Polygon(hDC, aptl, 4);
    }

    SetPolyFillMode (hDC, OldPolyMode);
    SelectObject(hDC, hOldPen);
    SelectObject(hDC, hOldBrush);
    DeleteObject(hBrush_lt);
    DeleteObject(hBrush_dk);
}

void DrawCoords(HDC hDC, int reverse, DWORD clrBackGround, DWORD clrText)
{
    int i;
    POINT pt;
    TEXTMETRIC tm;

    HFONT hFont = ::CreateFont(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        DEFAULT_QUALITY, FIXED_PITCH | FF_SWISS, TEXT("Helv"));

    HFONT hOldFont = HFONT(::SelectObject(hDC, hFont));
    DWORD OldBkColor = SetBkColor(hDC, clrBackGround);
    int OldBkMode = SetBkMode(hDC, TRANSPARENT);
    DWORD OldTextColor = SetTextColor(hDC, clrText);
    ::GetTextMetrics(hDC, &tm);
    short xchar = tm.tmMaxCharWidth;
    short ychar = tm.tmHeight;

    for (i = 0; i < 8; i++)
    {
        QuerySqOrigin(0, i, &pt);

        TextOut(hDC, pt.x-xchar, pt.y-BRD_VERT/2-ychar/2,
              (reverse ? TEXT("87654321") + i : TEXT("12345678") +i) ,1);

        QuerySqOrigin(i, 0, &pt);

        TextOut(hDC, pt.x+BRD_HORZFRONT/2-xchar/2, pt.y+BRD_EDGE,
              (reverse ? TEXT("hgfedcba") + i : TEXT("abcdefgh") + i), 1);
    }
   
    ::SelectObject(hDC, hOldFont);
    ::DeleteObject(hFont);
    ::SetBkColor(hDC, OldBkColor);
    ::SetBkMode(hDC, OldBkMode);
    ::SetTextColor(hDC, OldTextColor);
}

void DrawWindowBackGround(HDC hDC, HWND hWnd, DWORD bkcolor)
{
    RECT rect;
    HBRUSH hBrush = CreateSolidBrush(bkcolor);
    HBRUSH hOldBrush = HBRUSH(SelectObject(hDC, hBrush));
    GetClientRect(hWnd, &rect);
    FillRect(hDC, &rect, hBrush);
    SelectObject(hDC, hOldBrush);
    DeleteObject(hBrush);
}

void HiliteSquare(HWND hWnd, int Square)
{
    POINT aptl[4];
    int y = Square / 8;
    int x = Square % 8;
    QuerySqCoords(x, y, aptl + 0);
    HRGN hRgn = CreatePolygonRgn(aptl, 4, WINDING);
    HDC hDC = GetDC(hWnd);
    InvertRgn(hDC, hRgn);
    ReleaseDC(hWnd, hDC);
    DeleteObject(hRgn);
    HilitSq = Square;
}

void UnHiliteSquare(HWND hWnd, int Square)
{
    HDC hDC;
    int x,y;
    POINT aptl[4];
    HRGN hRgn;

    if (HilitSq == -1)
        return;

    y = Square / 8;
    x = Square % 8;

    QuerySqCoords(x,y, aptl+0);
#ifndef WINCE
    hRgn = CreatePolygonRgn( aptl, 4, WINDING);
#endif
    hDC = GetDC(hWnd);
#ifndef WINCE
    InvertRgn(hDC, hRgn);
#endif
    ReleaseDC(hWnd, hDC);
    DeleteObject(hRgn);
    HilitSq = -1;
}

