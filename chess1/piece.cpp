/*
  C source for GNU CHESS

  Revision: 1990-09-30

  Modified by Daryl Baker for use in MS WINDOWS environment

  Based on Ideas and code segments of Charles Petzold from artices in
  MicroSoft Systems Journal.

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
#include "globals.h"

static constexpr LONG PIECE_XAXIS = 32, PIECE_YAXIS = 32;

static short constexpr ConvertCoordToIndex(short x, short y)
{
    return y * 8 + x;
}

static void QuerySqCenter(short x, short y, POINT *pptl)
{
    POINT aptl[4];
    QuerySqCoords(x, y, aptl);
    pptl->x = (aptl[0].x + aptl[1].x + aptl[2].x + aptl[3].x) / 4;
    pptl->y = (aptl[0].y + aptl[2].y) / 2;
}

static void PieceOriginFromCenter(POINT *pptl)
{
    pptl->x -= PIECE_XAXIS / 2;
    pptl->y -= PIECE_YAXIS / 2;
}

static void QuerySqPieceOrigin(short x, short y, POINT *pptl)
{
    QuerySqCenter(x, y, pptl);
    PieceOriginFromCenter (pptl);
}

/*
   Draw a piece in the specificed point

   Piece_bitmap is a structure with the handles for the mask,
   outline and piece.
*/
static void
ShowPiece(HDC hdc, POINT *pptl, PIECEBITMAP *Piece_bitmap, COLORREF color)
{
    HBRUSH hOldBrush = HBRUSH(::SelectObject(hdc, ::GetStockObject(BLACK_BRUSH)));
    HPEN hOldPen = HPEN(::SelectObject(hdc, ::GetStockObject(BLACK_PEN)));
    HDC hMemDC = ::CreateCompatibleDC(hdc);

    /* Write the mask to clear the space */
    ::SelectObject(hMemDC, Piece_bitmap->mask);
    ::BitBlt(hdc, pptl->x, pptl->y, PIECE_XAXIS, PIECE_YAXIS, hMemDC, 0, 0,SRCAND);

    /* Write out the piece with an OR */
    HBRUSH hBrush = ::CreateSolidBrush(color);
    ::SelectObject(hdc, hBrush);
    ::SelectObject(hMemDC, Piece_bitmap->piece);
    ::BitBlt(hdc, pptl->x, pptl->y, PIECE_XAXIS, PIECE_YAXIS, hMemDC, 0, 0, 0xB80746L);

    /* The draw the outline */
    ::SelectObject(hdc, GetStockObject(BLACK_BRUSH));
    ::SelectObject(hMemDC, Piece_bitmap->outline);
    ::BitBlt(hdc, pptl->x, pptl->y, PIECE_XAXIS, PIECE_YAXIS, hMemDC, 0, 0, 0xB80746L);
    ::SelectObject(hdc, hOldBrush);
    ::SelectObject(hdc, hOldPen);
    ::DeleteObject(hBrush);

    if (::DeleteDC(hMemDC) == 0)
        ::MessageBeep(0);
}

static void DrawOnePiece(HDC hdc, short x, short y, PIECEBITMAP *piece, COLORREF color)
{
    POINT origin;
    QuerySqPieceOrigin(x, y, &origin);
    ShowPiece(hdc, &origin, piece, color);
}

static constexpr short NETURAL = 2;

void DrawAllPieces(HDC hDC, PIECEBITMAP *pieces, int reverse, short *pbrd,
                   short *color, COLORREF clrblack, COLORREF clrwhite)
{
    for (short y = 0; y < 8; y++)
    {
        for (short x = 0; x < 8; x++)
        {
            short i = ConvertCoordToIndex(x, y);
            short *colori = color + i;

            if (*colori != NETURAL)
            {
                COLORREF colorRef = *colori == BLACK ? clrblack : clrwhite;

                if (reverse == 0)
                    DrawOnePiece(hDC, x, y, pieces + *(pbrd + i), colorRef);
                else
                    DrawOnePiece(hDC, 7 - x, 7 - y, pieces + *(pbrd + i), colorRef);
            }
        }
    }
}

