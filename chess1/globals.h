#ifndef GLOBALS_H
#define GLOBALS_H

#include "chess.h"
#include <cstdio>

extern struct BookEntry *Book;
extern DWORD hashkey, hashbd;
extern struct hashval *hashcode;
extern struct hashentry *ttable;
extern FILE *hashfile;
extern struct leaf *Tree, *root;
extern short TrPnt[maxdepth];
extern short PieceList[2][16], PawnCnt[2][8];
extern short castld[2], Mvboard[64];
extern short svalue[64];
extern struct flags flag;
extern short opponent, computer, Awindow, Bwindow, dither, INCscore;
extern long ResponseTime, ExtraTime, Level, et, et0, time0, ft;
extern long NodeCnt, ETnodes, EvalNodes, HashCnt, FHashCnt, HashCol;
extern short player, xwndw, rehash;
extern struct GameRec *GameList;
extern short Sdepth, GameCnt, Game50, MaxSearchDepth;
extern short epsquare, contempt;
extern struct TimeControlRec TimeControl;
extern short TCmoves, TCminutes, TCflag, OperatorTime;
extern WORD hint, PrVar[maxdepth];
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
extern short board[64], color[64];
extern BYTE *nextpos;
extern BYTE *nextdir;
extern const short otherside[3];
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

