#ifndef GLOBALS_H
#define GLOBALS_H
#include "chess.h"

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
#endif

