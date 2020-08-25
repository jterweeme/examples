#ifndef SIM_H
#define SIM_H

#include "chess.h"

class Sim
{
private:
    static CONSTEXPR BYTE TRUESCORE = 0x01;
    static CONSTEXPR BYTE LOWERBOUND = 0x02;
    static CONSTEXPR BYTE UPPERBOUND = 0x04;
    static CONSTEXPR BYTE KINGCASTLE = 0x08;
    static CONSTEXPR BYTE QUEENCASTLE = 0x10;

    static CONSTEXPR short row(short a) { return a >> 3; }

    GLOBALHANDLE hGameList, hTree, hHistory, hTTable, hHashCode, hdistdata;
    GLOBALHANDLE htaxidata, hnextdir, hnextpos;

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
    short _maxSearchDepth;

    static void Initialize_dist();
    static void Initialize_moves();
    static void repetition(short *cnt);
    static void ScoreLoneKing(short int side, short int *score);
    static int BishopValue(short sq, short);
    static int RookValue(short sq, short);
    static int QueenValue(short sq, short);
    static void BRscan(short sq, short *s, short *mob);
    static int PawnValue(short sq, short side);
    static void UpdateWeights();
    static HGLOBAL xalloc(SIZE_T n);
    static void ataks(short side, short *a);
    static void ScoreThreat2(short u, short &cnt, short *s);
    static bool mateThreat(short ply);
    static short taxicab(short a, short b);
    static short xdistance(short a, short b);

    static int ScoreKPK(short side, short winner, short loser,
             short king1, short king2, short sq);
public:
    Sim();
    void init_main();
    void FreeGlobals();
    void NewGame(HWND hWnd, HWND compClr);
    short maxSearchDepth() const;
    void maxSearchDepth(short n);
    static void ExaminePosition();
    static bool patak(short c, short u);
    static void BlendBoard(const short a[64], const short b[64], short c[64]);
    static void CopyBoard(const short a[64], short b[64]);
    static void ScorePosition(short side, short *score, short *value);
    static int KingValue(short sq, short);
    static void KingScan(short sq, short *s);
    void ZeroRPT();
    void ZeroTTable();
    static int SqAtakd(short sq, short side);
    static int castle(short side, short kf, short kt, short iop);
    static int ProbeFTable(short side, short depth, short *alpha, short *beta, short *score);
    static int ProbeTTable(short side, short depth, short *alpha, short *beta, short *score);
    static void PutInTTable(short side, short score, short depth, short alpha, short beta, WORD mv);
    static void MoveList(short side, short ply);
    static void ShowResults(HWND hwnd, short score, PWORD bstline, char ch);
    static int ScoreKBNK(short winner, short king1, short king2);
    static BYTE CB(WORD i);
    static void InitializeStats();
    static void EnPassant(short xside, short f, short t, short iop);

    static void updateSearchStatus(short &d, short pnt, short f,
                       short t, short best, short alpha, short score);

    static void MakeMove(short side, Leaf *node, short *tempb,
              short *tempc, short *tempsf, short *tempst, short *INCscore);

    static void UnmakeMove(short side, Leaf *node, short *tempb,
                           short *tempc, short *tempsf, short *tempst);

    static int search(HWND hWnd, short side, short ply, short depth,
            short alpha, short beta, WORD *bstline, short *rpt);

    static int evaluate(short side, short ply, short alpha, short beta,
              short INCscore, short *slk, short *InChk, short toSquare);

    void SelectMove(HINSTANCE hInstance, HWND hWnd, HWND compClr, short side,
                           short iop, short maxSearchDepth, long xft);
};
#endif

