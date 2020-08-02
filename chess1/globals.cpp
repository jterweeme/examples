#include "globals.h"

BookEntry *Book;
DWORD hashkey, hashbd;
WORD hint, PrVar[MAXDEPTH];
HashVal *hashcode;
hashentry *ttable;
FILE *hashfile;
Leaf *Tree;
Flags flag;
GameRec *GameList;
TimeControlRec TimeControl;
long ResponseTime, ExtraTime, Level, et, et0, time0, ft;
long NodeCnt, ETnodes, EvalNodes, evrate;
short TrPnt[MAXDEPTH], PieceList[2][16], PawnCnt[2][8];
short castld[2], Mvboard[64], svalue[64];
short opponent, computer, Awindow, Bwindow, dither, INCscore;
short Sdepth, GameCnt, Game50, epsquare, contempt;
short TCflag, TCmoves, TCminutes, OperatorTime, player, xwndw, rehash;
short Pindex[64], PieceCnt[2], c1, c2, mtl[2], pmtl[2], hung[2];
short HasKnight[2], HasBishop[2], HasRook[2], HasQueen[2];
short ChkFlag[MAXDEPTH], CptrFlag[MAXDEPTH], PawnThreat[MAXDEPTH];
short Pscore[MAXDEPTH], Tscore[MAXDEPTH], boarddraw[64], colordraw[64];
short stage, stage2, Developed[2], *distdata, *taxidata, board[64], color[64];
BYTE *history, *nextpos, *nextdir;
const short otherside[3] = {1, 0, 2};
HWND hComputerMove, hWhosTurn;
HWND hClockComputer, hClockHuman, hMsgComputer, hMsgHuman, hStats;
GLOBALHANDLE hBook = 0;
TCHAR mvstr[4][6];
short sweep[8] = {false, false, false, true, true, true, false, false};

