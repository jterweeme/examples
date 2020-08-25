#ifndef GLOBALS_H
#define GLOBALS_H

#include "chess.h"
#include <cstdio>

extern BookEntry *Book;
extern DWORD hashkey, hashbd;
extern WORD hint, PrVar[MAXDEPTH];
extern Leaf *Tree;
extern Flags flag;
extern GameRec *GameList;
extern TimeControlRec TimeControl;
extern HWND hMsgComputer, hStats;
extern GLOBALHANDLE hBook;
extern TCHAR mvstr[4][6];   //moet ANSI zijn eigenlijk
extern long ResponseTime, ExtraTime, Level, et, et0, time0, ft;
extern long NodeCnt, ETnodes, evrate;
extern short TrPnt[MAXDEPTH], player;
extern short PieceList[2][16], castld[2], Mvboard[64];
extern short opponent, computer, Awindow, Bwindow, dither, INCscore;
extern short Sdepth, GameCnt, Game50, epsquare, contempt;
extern short TCmoves, TCminutes, TCflag, OperatorTime;
extern short Pindex[64], PieceCnt[2];
extern short Tscore[MAXDEPTH], boarddraw[64], colordraw[64];
extern short board[64], color[64];
extern const short otherside[3];
extern HWND hWhosTurn;
extern HWND hComputerMove;
extern HWND hClockHuman;
extern HWND hClockComputer;

extern void DrawAllPieces(HDC hdc, PIECEBITMAP *pieces, int reverse,
                short *pbrd, short *color, COLORREF xblack, COLORREF xwhite);

extern void UpdateDisplay(HWND hWnd, HWND compClr, short f, short t,
                          short flag, short iscastle, bool reverse);

extern int parse(FILE *fd, WORD *mv, short side);
extern void CaptureList(short side, short ply);
extern void SetTimeControl(long xft);
extern void ElapsedTime(short iop, long extra, long responseTime, long xft);
extern void SearchStartStuff(short side);
extern void algbr(short f, short t, short flag);
extern void ShowNodeCnt(HWND hwnd, long NodeCnt, long evrate);
extern void ShowCurrentMove(HWND hwnd, short pnt, short f, short t);
extern void ListGame(char *fname);
extern void GetGame(HWND hWnd, HWND compClr, char *fname);
extern int TestDialog(HWND hWnd, HINSTANCE hInst);
extern void StatDialog(HWND hWnd, HINSTANCE hInst);
extern void pick(short p1, short p2);
extern void ShowPlayers(HWND hwnd);
extern void algbr(short f, short t, short flag);
extern int DoGetNumberDlg(HINSTANCE hInst, HWND hwnd, LPTSTR, int);
extern void ShowSidetoMove();
extern void UpdateClocks();
#endif

