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

#ifndef DEFS_H
#define DEFS_H

/*function prototypes
*/

#include "chess.h"
#include <cstdio>

extern void SelectMove(HINSTANCE hInstance, HWND hWnd, HWND compClr, short side,
                       short iop, short maxSearchDepth, HACCEL haccel, long xft);

extern void DrawAllPieces(HDC hdc, PIECEBITMAP *pieces, int reverse,
                short *pbrd, short *color, COLORREF xblack, COLORREF xwhite);

extern void MakeMove(short side, Leaf *node, short *tempb,
              short *tempc, short *tempsf, short *tempst, short *INCscore);

extern void UnmakeMove(short side, Leaf *node, short *tempb,
                       short *tempc, short *tempsf, short *tempst);

extern void UpdateDisplay(HWND hWnd, HWND compClr, short f, short t,
                          short flag, short iscastle, bool reverse);

extern int parse(FILE *fd, WORD *mv, short side);
extern void ZeroTTable();
extern void ZeroRPT();
extern void MoveList(short side, short ply);
extern void CaptureList(short side, short ply);
extern void InitializeStats();
extern int SqAtakd(short sq, short side);
extern void SetTimeControl(long xft);
extern void ElapsedTime(short iop, long extra, long responseTime, long xft);
extern void SearchStartStuff(short side);
extern void algbr(short f, short t, short flag);
extern void ShowNodeCnt(HWND hwnd, long NodeCnt, long evrate);
extern void ShowCurrentMove(HWND hwnd, short pnt, short f, short t);
extern void ListGame(char *fname);
extern int TimeControlDialog(HWND hWnd, HINSTANCE hInst, DWORD Param);
extern void GetGame(HWND hWnd, HWND compClr, char *fname);
extern INT_PTR CALLBACK About(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern int TestDialog(HWND hWnd, HINSTANCE hInst);
extern void StatDialog(HWND hWnd, HINSTANCE hInst);
extern void pick(short p1, short p2);
extern void ShowPlayers(HWND hwnd);
extern void algbr(short f, short t, short flag);
extern int DoGetNumberDlg(HINSTANCE hInst, HWND hwnd, LPTSTR, int);
#endif
