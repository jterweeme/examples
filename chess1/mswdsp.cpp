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

void ShowPlayers(HWND hwnd)
{
    /* display in the status line what color the computer is playing */
    ::SetWindowText(hwnd, computer == black ? TEXT("Computer is black") : TEXT("Computer is white"));
}

void SMessageBox(HINSTANCE hInstance, HWND hWnd, int str_num, int str1_num )
{
    TCHAR str[80], str1[20];
    ::LoadString(hInstance, str_num, str, sizeof(str));
    ::LoadString(hInstance, str1_num, str1, sizeof ( str1 ) );
    ::MessageBox(hWnd, str, str1, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL );
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

static LPCTSTR ColorStr[2] = {TEXT("White"), TEXT("Black")};

void ShowSidetoMove()
{
    TCHAR tmp[30];
    ::wsprintf(tmp, TEXT("It is %s's turn"), ColorStr[player]);
    ::SetWindowText(hWhosTurn, tmp);
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

void UpdateClocks()
{
    TCHAR tmp[20];
    short m = short(et / 60);
    short s = short(et - 60 * (long)m);

    if (TCflag)
    {
        m = short((TimeControl.clock[player] - et) / 60);
        s = short(TimeControl.clock[player] - et - 60 * (long)m);
    }

    m = ::myMax(m, short(0));
    s = ::myMax(s, short(0));
    ::wsprintf(tmp, TEXT("%0d:%02d"), m, s);
    ::SetWindowText(player == white ? hClockHuman : hClockComputer, tmp);

    if (flag.post)
        ShowNodeCnt(hStats, NodeCnt, evrate);
}

void DrawPiece(HWND hWnd, short f, bool reverse)
{
    int x = reverse ? 7 - f % 8 : f % 8;
    int y = reverse ? 7 - f / 8 : f / 8;
    POINT aptl[4];
    QuerySqCoords(x, y, aptl + 0);
    HRGN hRgn = ::CreatePolygonRgn(aptl, 4, WINDING);
    ::InvalidateRgn(hWnd, hRgn, FALSE );
    ::DeleteObject(hRgn);
}

void UpdateDisplay(HWND hWnd, HWND compClr, short f, short t, short redraw, short isspec, bool reverse)
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


