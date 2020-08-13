#include "globals.h"

BookEntry *Book;
DWORD hashkey, hashbd;
WORD hint, PrVar[MAXDEPTH];
Leaf *Tree;
Flags flag;
GameRec *GameList;
TimeControlRec TimeControl;
HWND hMsgComputer, hStats;
GLOBALHANDLE hBook = 0;
TCHAR mvstr[4][6];
long ResponseTime, ExtraTime, Level, et, et0, time0, ft, NodeCnt, ETnodes, evrate;
short TrPnt[MAXDEPTH], PieceList[2][16], castld[2], Mvboard[64];
short opponent, computer, Awindow, Bwindow, dither, INCscore;
short Sdepth, GameCnt, Game50, epsquare, contempt;
short TCflag, TCmoves, TCminutes, OperatorTime, player;
short Pindex[64], PieceCnt[2], c1, c2;
short Tscore[MAXDEPTH], boarddraw[64], colordraw[64];
short board[64], color[64];
const short otherside[3] = {1, 0, 2};

