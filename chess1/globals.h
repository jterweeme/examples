#ifndef GLOBALS_H
#define GLOBALS_H
#include "chess.h"
#include <cstdio>

extern BookEntry *Book;
extern DWORD hashkey, hashbd;
extern WORD hint, PrVar[MAXDEPTH];
extern BYTE *history, *nextpos, *nextdir;
extern HashVal *hashcode;
extern hashentry *ttable;
extern FILE *hashfile;
extern Leaf *Tree;
extern Flags flag;
extern GameRec *GameList;
extern TimeControlRec TimeControl;
extern long ResponseTime, ExtraTime, Level, et, et0, time0, ft;
extern long NodeCnt, ETnodes, EvalNodes, evrate;
extern short TrPnt[MAXDEPTH], player, xwndw, rehash;
extern short PieceList[2][16], PawnCnt[2][8], castld[2], Mvboard[64], svalue[64];
extern short opponent, computer, Awindow, Bwindow, dither, INCscore;
extern short Sdepth, GameCnt, Game50, epsquare, contempt;
extern short TCmoves, TCminutes, TCflag, OperatorTime;
extern short Pindex[64], PieceCnt[2], mtl[2], pmtl[2], hung[2];
extern short HasKnight[2], HasBishop[2], HasRook[2], HasQueen[2];
extern short ChkFlag[MAXDEPTH], CptrFlag[MAXDEPTH], PawnThreat[MAXDEPTH];
extern short Pscore[MAXDEPTH], Tscore[MAXDEPTH], boarddraw[64], colordraw[64];
extern short stage, stage2, Developed[2], c1, c2, sweep[8];
extern short *distdata, *taxidata, board[64], color[64];
extern const short otherside[3];
extern HWND hComputerMove, hWhosTurn, hClockComputer;
extern HWND hClockHuman, hMsgComputer, hMsgHuman, hStats;
extern GLOBALHANDLE hBook;
extern TCHAR mvstr[4][6];   //moet ANSI zijn eigenlijk
#endif

