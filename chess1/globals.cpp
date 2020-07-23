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

#include "globals.h"
#include <stdio.h>

struct BookEntry *Book;

#if ttblsz
DWORD hashkey, hashbd;
struct hashval far *hashcode;
struct hashentry far *ttable;
#endif

FILE *hashfile;
struct leaf *Tree, *root;

short TrPnt[maxdepth];
short PieceList[2][16], PawnCnt[2][8];
short castld[2], Mvboard[64];
short svalue[64];
struct flags flag;
short opponent, computer, Awindow, Bwindow, dither, INCscore;
long ResponseTime, ExtraTime, Level, et, et0, time0, ft;
long NodeCnt, ETnodes, EvalNodes, HashCnt, FHashCnt, HashCol;
short player, xwndw, rehash;
struct GameRec *GameList;
short Sdepth, GameCnt, Game50, MaxSearchDepth;
short epsquare, contempt;
struct TimeControlRec TimeControl;
short TCflag, TCmoves, TCminutes, OperatorTime;
WORD hint, PrVar[maxdepth];
short Pindex[64];
short PieceCnt[2];
short c1, c2, *atk1, *atk2, *PC1, *PC2, atak[2][64];
short mtl[2], pmtl[2], emtl[2], hung[2];
short FROMsquare, TOsquare, Zscore, zwndw;
short HasKnight[2], HasBishop[2], HasRook[2], HasQueen[2];
short ChkFlag[maxdepth], CptrFlag[maxdepth], PawnThreat[maxdepth];
short Pscore[maxdepth], Tscore[maxdepth];
WORD killr0[maxdepth], killr1[maxdepth], killr2[maxdepth], killr3[maxdepth];
WORD PV, Swag0, Swag1, Swag2, Swag3, Swag4;
BYTE *history;

short rpthash[2][256];
short Mwpawn[64], Mbpawn[64], Mknight[2][64], Mbishop[2][64];
short Mking[2][64], Kfield[2][64];
short KNIGHTPOST, KNIGHTSTRONG, BISHOPSTRONG, KATAK;
short PEDRNK2B, PWEAKH, PADVNCM, PADVNCI, PAWNSHIELD, PDOUBLED, PBLOK;
short RHOPN, RHOPNX, KHOPN, KHOPNX, KSFTY;
short ATAKD, HUNGP, HUNGX, KCASTLD, KMOVD, XRAY, PINVAL;
short stage, stage2, Developed[2];
short PawnBonus, BishopBonus, RookBonus;
short *distdata, *taxidata;

short board[64], color[64];
BYTE *nextpos;
BYTE *nextdir;


const short otherside[3] = {1, 0, 2};

#if ttblsz
#if HASHFILE
/*
  In a networked enviroment gnuchess might be compiled on different
  hosts with different random number generators, that is not acceptable
  if they are going to share the same transposition table.
*/
static unsigned long int next = 1;

unsigned int urand (void)
{
  next *= 1103515245;
  next += 12345;
  return ((unsigned int) (next >> 16) & 0xFFFF);
}

void srand (unsigned int seed)
{
  next = seed;
}
#endif /*HASHFILE*/
#endif /*ttblsz*/

HWND hComputerColor;
HWND hComputerMove;
HWND hWhosTurn;
HWND hClockComputer;
HWND hClockHuman;
HWND hMsgComputer;
HWND hMsgHuman;
HWND hStats;

GLOBALHANDLE hBook = 0;
COLORREF clrBackGround;
DWORD clrBlackSquare;
DWORD clrWhiteSquare;
DWORD clrBlackPiece;
DWORD clrWhitePiece;
DWORD clrText;
HINSTANCE hInst;
HACCEL hAccel;

TCHAR mvstr[4][6];
long evrate;
int coords = 1;

struct PIECEBITMAP pieces[7];

short boarddraw[64];
short colordraw[64];
