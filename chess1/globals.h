#ifndef GLOBALS_H
#define GLOBALS_H

#include "chess.h"
#include <cstdio>

extern BookEntry *Book;
extern DWORD hashkey, hashbd;
extern HashVal *hashcode;
extern hashentry *ttable;
extern FILE *hashfile;
extern Leaf *Tree;
extern short TrPnt[MAXDEPTH], player, xwndw, rehash;
extern short PieceList[2][16], PawnCnt[2][8], castld[2], Mvboard[64], svalue[64];
extern Flags flag;
extern short opponent, computer, Awindow, Bwindow, dither, INCscore;
extern long ResponseTime, ExtraTime, Level, et, et0, time0, ft;
extern long NodeCnt, ETnodes, EvalNodes, HashCnt, FHashCnt;
extern GameRec *GameList;
extern short Sdepth, GameCnt, Game50, MaxSearchDepth, epsquare, contempt;
extern TimeControlRec TimeControl;
extern short TCmoves, TCminutes, TCflag, OperatorTime;
extern WORD hint, PrVar[MAXDEPTH];
extern short Pindex[64], PieceCnt[2], mtl[2], pmtl[2], emtl[2], hung[2];
extern short TOsquare, HasKnight[2], HasBishop[2], HasRook[2], HasQueen[2];
extern short ChkFlag[MAXDEPTH], CptrFlag[MAXDEPTH], PawnThreat[MAXDEPTH];
extern short Pscore[MAXDEPTH], Tscore[MAXDEPTH];
extern short stage, stage2, Developed[2], c1, c2;
extern short *distdata, *taxidata, board[64], color[64];
extern BYTE *history, *nextpos, *nextdir;
extern const short otherside[3];
extern HWND hComputerColor, hComputerMove, hWhosTurn, hClockComputer;
extern HWND hClockHuman, hMsgComputer, hMsgHuman, hStats;
extern GLOBALHANDLE hBook;
extern COLORREF clrBackGround, clrBlackSquare, clrWhiteSquare, clrBlackPiece, clrWhitePiece, clrText;
extern HINSTANCE hInst;
extern HACCEL hAccel;
extern TCHAR mvstr[4][6];   //moet ANSI zijn eigenlijk
extern long evrate;
extern PIECEBITMAP pieces[7];
extern short boarddraw[64], colordraw[64];
#endif

