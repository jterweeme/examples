/*
  C source for GNU CHESS

  Revision: 1990-09-30

  Modified by Daryl Baker for use in MS WINDOWS environment

  Copyright (C) 1986, 1987, 1988, 1989, 1990 Free Software Foundation, Inc.
  Copyright (c) 1988, 1989, 1990  John Stanback

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
#include "resource.h"
#include "globals.h"
#include "toolbox.h"
#include "board.h"

void ShowPlayers(HWND hwnd)
{
    /* display in the status line what color the computer is playing */
    ::SetWindowText(hwnd, computer == black ? TEXT("Computer is black") : TEXT("Computer is white"));
}

void ShowCurrentMove(HWND hwnd, short pnt, short f, short t)
{
    TCHAR tmp[30];

    if (hwnd)
    {
        algbr(f, t, false);
        ::wsprintf(tmp, TEXT("(%2d) %4s"), pnt, LPCTSTR(mvstr[0]));
        ::SetDlgItemText(hwnd, POSITIONTEXT, tmp);
    }
}

void ShowNodeCnt(HWND hwnd, long NodeCnt, long evrate)
{
    TCHAR tmp[40];

    if (hwnd)
    {
        ::wsprintf(tmp, TEXT("%-8ld"), NodeCnt);
        ::SetDlgItemText(hwnd, NODETEXT, tmp);
        ::wsprintf(tmp, TEXT("%-5ld"), evrate);
        ::SetDlgItemText(hwnd, NODESECTEXT, tmp);
    }
}

void SearchStartStuff(short int)
{
}

static void DrawPiece(HWND hWnd, short f, bool reverse)
{
    int x = reverse ? 7 - f % 8 : f % 8;
    int y = reverse ? 7 - f / 8 : f / 8;
    POINT aptl[4];
    QuerySqCoords(x, y, aptl + 0);
#ifndef WINCE
    HRGN hRgn = ::CreatePolygonRgn(aptl, 4, WINDING);
    ::InvalidateRgn(hWnd, hRgn, FALSE );
    ::DeleteObject(hRgn);
#endif
}

void UpdateDisplay(HWND hWnd, HWND compClr, short f, short t,
                   short redraw, short isspec, bool reverse)
{
    for (short sq = 0; sq < 64; sq++)
    {
        boarddraw[sq] = board[sq];
        colordraw[sq] = color[sq];
    }

    if (redraw)
    {
        ::InvalidateRect(hWnd, NULL, TRUE);
        ShowPlayers(compClr);
        ::UpdateWindow(hWnd);
    }
    else
    {
        DrawPiece(hWnd, f, reverse);
        DrawPiece(hWnd, t, reverse);

        if (isspec & CSTLMASK)
        {
            if (t > f)
            {
                DrawPiece(hWnd, f + 3, reverse);
                DrawPiece(hWnd, t - 1, reverse);
            }
            else
            {
                DrawPiece(hWnd, f - 4, reverse);
                DrawPiece(hWnd, t + 1, reverse);
            }
        }
        else if (isspec & EPMASK)
        {
            DrawPiece(hWnd, t - 8, reverse);
            DrawPiece(hWnd, t + 8, reverse);
        }

        UpdateWindow(hWnd);
    }
}


