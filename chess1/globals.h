#ifndef GLOBALS_H
#define GLOBALS_H

#include "gnuchess.h"
#include <cstdio>

struct PIECEBITMAP
{
    HBITMAP piece;
    HBITMAP mask;
    HBITMAP outline;
};

#if ttblsz
extern DWORD hashkey, hashbd;
extern struct hashval far *hashcode;
extern struct hashentry far *ttable;
#endif

extern FILE *hashfile;
extern short PieceList[2][16], PawnCnt[2][8];
extern long NodeCnt, ETnodes, EvalNodes, HashCnt, FHashCnt, HashCol;
extern short player, xwndw, rehash;
extern short TCmoves, TCminutes, TCflag;
extern short Pindex[64];
extern short PieceCnt[2];
extern short c1, c2, *atk1, *atk2, *PC1, *PC2, atak[2][64];
extern short mtl[2], pmtl[2], emtl[2], hung[2];
extern short FROMsquare, TOsquare, Zscore, zwndw;
extern short HasKnight[2], HasBishop[2], HasRook[2], HasQueen[2];
extern short ChkFlag[maxdepth], CptrFlag[maxdepth], PawnThreat[maxdepth];
extern short Pscore[maxdepth], Tscore[maxdepth];
extern WORD killr0[maxdepth], killr1[maxdepth], killr2[maxdepth], killr3[maxdepth];
extern WORD PV, Swag0, Swag1, Swag2, Swag3, Swag4;
extern BYTE *history;

extern short rpthash[2][256];
extern short Mwpawn[64], Mbpawn[64], Mknight[2][64], Mbishop[2][64];
extern short Mking[2][64], Kfield[2][64];
extern short KNIGHTPOST, KNIGHTSTRONG, BISHOPSTRONG, KATAK;
extern short PEDRNK2B, PWEAKH, PADVNCM, PADVNCI, PAWNSHIELD, PDOUBLED, PBLOK;
extern short RHOPN, RHOPNX, KHOPN, KHOPNX, KSFTY;
extern short ATAKD, HUNGP, HUNGX, KCASTLD, KMOVD, XRAY, PINVAL;
extern short stage, stage2, Developed[2];
extern short PawnBonus, BishopBonus, RookBonus;
extern short *distdata, *taxidata;
extern BYTE *nextpos;
extern BYTE *nextdir;
extern HWND hComputerColor;
extern HWND hComputerMove;
extern HWND hWhosTurn;
extern HWND hClockComputer;
extern HWND hClockHuman;
extern HWND hMsgComputer;
extern HWND hMsgHuman;
extern HWND hStats;
extern GLOBALHANDLE hBook;
extern DWORD clrBackGround;
extern DWORD clrBlackSquare;
extern DWORD clrWhiteSquare;
extern DWORD clrBlackPiece;
extern DWORD clrWhitePiece;
extern DWORD clrText;
extern HINSTANCE hInst;
extern HACCEL hAccel;
extern TCHAR mvstr[4][6];
extern long evrate;
extern int coords;
extern struct PIECEBITMAP pieces[7];
extern short boarddraw[64];
extern short colordraw[64];

#endif

