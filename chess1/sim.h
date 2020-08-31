#ifndef SIM_H
#define SIM_H

#include "chess.h"
#include <stdio.h>

class Sim
{
private:
    static CONSTEXPR BYTE TRUESCORE = 0x01;
    static CONSTEXPR BYTE LOWERBOUND = 0x02;
    static CONSTEXPR BYTE UPPERBOUND = 0x04;
    static CONSTEXPR BYTE KINGCASTLE = 0x08;
    static CONSTEXPR BYTE QUEENCASTLE = 0x10;
    static CONSTEXPR short CTLR = 0x0400;
    static CONSTEXPR short CTLQ = 0x0200, CTLK = 0x0100;
    static CONSTEXPR short CTLP = 0x4000, CTLB = 0x1800, CTLN = 0x2800, CTLRQ = 0x0600;
    static CONSTEXPR short CTLBN = 0x0800, CTLBQ = 0x1200, CTLNN = 0x2000;
    static CONSTEXPR short row(short a) { return a >> 3; }
    static CONSTEXPR WORD FREHASH = 6, EXACT = 0x0040, DRAW = 0x0400;
    static CONSTEXPR short PWNTHRT = 0x0080;
    static CONSTEXPR short VALUEP = 100;
    static CONSTEXPR short VALUER = 550, VALUEB = 355, VALUEN = 350;
    static CONSTEXPR short VALUEQ = 1100, VALUEK = 1200, BPAWN = 7;

    static short const ISOLANI[8];
    static short const PassedPawn0[8];
    static short const PassedPawn1[8];
    static short const PassedPawn2[8];
    static short const PassedPawn3[8];
    static short const kingP[3];
    static short const rank7[3];
    static short const sweep[8];
    static short const value[7];
    static short const control[7];
    static short const max_steps[8];
    static short const direc[8][8];
    static short const nunmap[120];
    static short const Stcolor[64];
    static short const Stboard[64];
    static short const PawnAdvance[64];
    static short const DyingKing[64];
    static short const pbishop[64];
    static short const pknight[64];
    static short const KingEnding[64];
    static short const KingOpening[64];
    static short const ptype[2][8];
    static short const KBNK[64];
    static short const BACKWARD[16];
    static short const BMBLTY[14];
    static short const RMBLTY[15];
    static short const KTHRT[36];

    GLOBALHANDLE hGameList, hTree, hHistory, hTTable, hHashCode, hdistdata;
    GLOBALHANDLE htaxidata, hnextdir, hnextpos;
    short KNIGHTPOST, KNIGHTSTRONG, BISHOPSTRONG, KATAK;
    short RHOPN, RHOPNX, KHOPN, KHOPNX, KSFTY;
    short ATAKD, HUNGP, HUNGX, KCASTLD, KMOVD, XRAY, PINVAL;
    short PEDRNK2B, PWEAKH, PADVNCM, PADVNCI, PAWNSHIELD, PDOUBLED, PBLOK;
    short _maxSearchDepth;
    short _awindow, Bwindow;
    short *distdata;
    short *taxidata;
    short *PC1, *PC2, *atk1, *atk2;
    short atak[2][64];
    short Mwpawn[64], Mbpawn[64], Mknight[2][64], Mbishop[2][64];
    short Mking[2][64], Kfield[2][64], hung[2];
    short svalue[64];
    short Pindex[64];
    short castld[2];
    short HasKnight[2], HasBishop[2], mtl[2], PawnCnt[2][8];
    short PawnBonus, BishopBonus, RookBonus;
    short ChkFlag[MAXDEPTH], CptrFlag[MAXDEPTH], PawnThreat[MAXDEPTH];
    short emtl[2], HasQueen[2], pmtl[2], Pscore[MAXDEPTH];
    long EvalNodes, HashCol, HashCnt, FHashCnt;
    short rpthash[2][256];
    short PieceCnt[2];
    short Tscore[MAXDEPTH];
    WORD PrVar[MAXDEPTH];
    short TOsquare;
    short Zscore, FROMsquare, zwndw, stage, stage2, Developed[2];
    short c1, c2, xwndw, rehash, HasRook[2];
    WORD Swag0, Swag1, Swag2, Swag3, Swag4;
    BYTE *history, *nextpos, *nextdir;
    FILE *hashfile;
    HashVal *hashcode;
    hashentry *ttable;
    Leaf *root;
    WORD _killr0[MAXDEPTH], killr1[MAXDEPTH], killr2[MAXDEPTH], killr3[MAXDEPTH];
    WORD PV;

    void Initialize_dist();
    void Initialize_moves();
    static void repetition(short *cnt);
    void ScoreLoneKing(short int side, short int *score);
    int BishopValue(short sq, short);
    int _rookValue(short sq, short);
    int QueenValue(short sq, short);
    void BRscan(short sq, short *s, short *mob);
    int PawnValue(short sq, short side);
    void UpdateWeights();
    static HGLOBAL xalloc(SIZE_T n);
    void _ataks(short side, short *a);
    void ScoreThreat2(short u, short &cnt, short *s);
    bool _mateThreat(short ply);
    short taxicab(short a, short b);
    short xdistance(short a, short b);
    static short _wking();
    static short _bking();
    short _enemyKing();
    bool patak(short c, short u);
    int trapped(short sq);
    int KnightValue(short sq, short);
    int ScoreKPK(short side, short winner, short loser, short king1, short king2, short sq);
    static void xlink(Leaf *node, short from, short to, short flag, short s, short ply);
    void LinkMove(short ply, short f, short t, short flag, short xside);
    void GenMoves(short ply, short sq, short side, short xside);
    bool xrecapture(short score, short alpha, short beta, short ply);
    void UpdateHashbd(short side, short piece, short f, short t);
    int _search(HWND hWnd, short side, short ply, short depth, short alpha, short beta, WORD *bstline, short *rpt);
    int evaluate(short side, short ply, short alpha, short beta, short INCscore, short *slk, short *InChk, short toSquare);
    void updateSearchStatus(short &d, short pnt, short f, short t, short best, short alpha, short score);
    bool anyatak(short c, short u);
    static void OutputMove(HINSTANCE hInstance, HWND hwnd, HWND compClr, Leaf *node);
    void CaptureList(short side, short ply);
    int castle(short side, short kf, short kt, short iop);
    void ScorePosition(short side, short *score, short *value);
    int KingValue(short sq, short);
    void KingScan(short sq, short *s);
    void EnPassant(short xside, short f, short t, short iop);
    static BYTE CB(WORD i);
    int _scoreKBNK(short winner, short king1, short king2);
    static void CopyBoard(const short a[64], short b[64]);
    void _updatePieceList(short side, short sq, short iop);
    static void ShowDepth(HWND hwnd, char ch);
public:
    Sim();
    void init_main();
    void FreeGlobals();
    void NewGame(HWND hWnd, HWND compClr);
    short maxSearchDepth() const;
    void maxSearchDepth(short n);
    void ExaminePosition();
    void BlendBoard(const short a[64], const short b[64], short c[64]);
    void ZeroRPT();
    void ZeroTTable();
    int SqAtakd(short sq, short side);
    int ProbeFTable(short side, short depth, short *alpha, short *beta, short *score);
    int ProbeTTable(short side, short depth, short *alpha, short *beta, short *score);
    void PutInTTable(short side, short score, short depth, short alpha, short beta, WORD mv);
    void MoveList(short side, short ply);
    static void ShowResults(HWND hwnd, short score, PWORD bstline, char ch);
    void InitializeStats();
    void aWindow(short n);
    short aWindow() const;
    void bWindow(short n);
    short bWindow() const;
    void MakeMove(short side, Leaf *node, short *tempb, short *tempc, short *tempsf, short *tempst, short *INCscore);
    void UnmakeMove(short side, Leaf *node, short *tempb, short *tempc, short *tempsf, short *tempst);
    void SelectMove(HINSTANCE hInstance, HWND hWnd, HWND compClr, short side, short iop, short maxSearchDepth, long xft);
};
#endif

