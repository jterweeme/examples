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

extern int parse(FILE *fd, WORD *mv, short side);
extern void GetOpenings(HINSTANCE hInstance);
extern void OpeningBook(unsigned short int *hint);
extern void repetition(short int *cnt);
extern void SelectMove(HWND hWnd, short side, short iop);
extern void ZeroTTable();
extern void ZeroRPT();
extern void MoveList(short int side, short int ply);
extern void CaptureList(short int side, short int ply);
extern int castle(short int side, short int kf, short int kt, short int iop);

extern void MakeMove(short side, struct leaf *node, short *tempb,
              short *tempc, short *tempsf, short *tempst, short *INCscore);

extern void UnmakeMove(short side, struct leaf *node, short *tempb,
                       short *tempc, short *tempsf, short *tempst);

extern void InitializeStats();
extern int SqAtakd(short sq, short side);

extern int evaluate(short side, short ply, short alpha, short beta,
             short INCscore, short *slk, short *InChk);

extern void ScoreLoneKing(short side, short *score);
extern void ScorePosition(short side, short *score);
extern void ExaminePosition(void);
extern void UpdateWeights(void);
extern void Initialize();
extern void InputCommand();
extern void ClrScreen();
extern void SetTimeControl();
extern void SelectLevel();
extern void UpdateDisplay(HWND hWnd, short f, short t, short flag, short iscastle);
extern void ElapsedTime(short int iop);
extern void ShowSidetoMove(void);
extern void SearchStartStuff(short int side);
extern void ShowDepth(char ch);
extern void ShowResults(short score, WORD *bstline, char ch);
extern void algbr(short int f, short int t, short int flag);
extern void OutputMove(HWND hWnd);
extern void ShowCurrentMove(short int pnt, short int f, short int t);
extern void ClrScreen();
extern void gotoXY(short x, short y);
extern void ClrEoln();
extern void DrawPiece(HWND hWnd, short int sq);
extern void UpdateClocks();
extern void ataks(short side, short *a);
extern void ListGame(char *fname);
extern void QuerySqSize(POINT *pptl);
extern void QueryBoardSize(POINT *pptl);
extern void QuerySqOrigin(short x, short y, POINT *pptl);
extern void QuerySqCoords(short x, short y, POINT aptl[]);
extern void Draw_Board(HDC hDC, int reverse, COLORREF DarkColor, COLORREF LightColor);
extern void DrawAllPieces(HDC hDC, int reverse, short *pbrd, short *color, DWORD xblack, DWORD xwhite);
extern void DrawWindowBackGround(HDC hDC, HWND hWnd, DWORD bkcolor);
extern void InitHitTest();
extern int HitTest(int x, int y);
extern void HiliteSquare(HWND hWnd, int Square);
extern void UnHiliteSquare(HWND hWnd, int Square);
extern void Hittest_Destructor();
extern int ColorDialog(HWND hWnd, HINSTANCE hInst, WPARAM wParam);
extern int TimeControlDialog(HWND hWnd, HINSTANCE hInst, DWORD Param);
extern int ReviewDialog(HWND hWnd, HINSTANCE hInst);
extern void SaveGame(HWND hWnd, char *fname);
extern void GetGame(HWND hWnd, char *fname);
extern void Undo(HWND hWnd);
extern int VerifyMove(HWND hWnd, TCHAR *s, short iop, short unsigned int *mv);
extern void ShowSidetoMove();
extern INT_PTR CALLBACK About(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern void FreeBook();
extern void DrawCoords(HDC hDC, int reverse, DWORD clrBackGround, DWORD clrText);
extern int TestDialog(HWND hWnd, HINSTANCE hInst);
extern int StatDialog(HWND hWnd, HINSTANCE hInst);
extern int PromoteDialog(HWND hWnd, HINSTANCE hInst);
extern void pick(short p1, short p2);
extern void SMessageBox(HINSTANCE hInstance, HWND hWnd, int str_num, int str1_num );
extern void ShowPlayers();
extern void algbr(short int f, short int t, short int flag);
extern int DoGetNumberDlg(HINSTANCE hInst, HWND hwnd, LPTSTR, int);
extern int DoManualMoveDlg(HINSTANCE hInst, HWND hWnd, TCHAR *szPrompt);
#endif
