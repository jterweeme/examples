#include "sim.h"
#include "globals.h"
#include "resource.h"
#include "toolbox.h"
#include "mainwin.h"
#include "book.h"
#include "zeit.h"
#include <ctime>

Sim::Sim()
{

}

short const Sim::value[7] = {0, VALUEP, VALUEN, VALUEB, VALUER, VALUEQ, VALUEK};
short const Sim::sweep[8] = {false, false, false, true, true, true, false, false};
short const Sim::rank7[3] = {6, 1, 0};
short const Sim::kingP[3] = {4, 60, 0};
short const Sim::PassedPawn0[8] = {0, 60, 80, 120, 200, 360, 600, 800};
short const Sim::PassedPawn1[8] = {0, 30, 40, 60, 100, 180, 300, 800};
short const Sim::PassedPawn2[8] = {0, 15, 25, 35, 50, 90, 140, 800};
short const Sim::PassedPawn3[8] = {0, 5, 10, 15, 20, 30, 140, 800};
short const Sim::ISOLANI[8] = {-12, -16, -20, -24, -24, -20, -16, -12};

HGLOBAL Sim::xalloc(SIZE_T n)
{
    HGLOBAL ret = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, n);

    if (ret == NULL)
        throw UINT(IDS_ALLOCMEM);

    return ret;
}

void Sim::aWindow(short n)
{
    _awindow = n;
}

short Sim::aWindow() const
{
    return _awindow;
}

void Sim::bWindow(short n)
{
    Bwindow = n;
}

short Sim::bWindow() const
{
    return Bwindow;
}

void Sim::init_main()
{
    hHistory = xalloc(8192);
    history = LPBYTE(GlobalLock(hHistory));
    hnextdir = xalloc(64 * 64 * 8);
    nextdir = LPBYTE(GlobalLock(hnextdir));
    hnextpos = xalloc(64 * 64 * 8);
    nextpos = LPBYTE(GlobalLock(hnextpos));
    hdistdata = xalloc(64 * 64 * sizeof(short));
    distdata = (short *)(GlobalLock(hdistdata));
    htaxidata = xalloc(64 * 64 * sizeof(short));
    taxidata = (short *)(GlobalLock(htaxidata));
    hHashCode = xalloc(2 * 7 * 64 * sizeof(HashVal));
    hashcode = (HashVal *)GlobalLock(hHashCode);
    hTree = xalloc(2000 * sizeof(Leaf));
    Tree = (Leaf *)GlobalLock(hTree);
    hGameList = xalloc(512 * sizeof(GameRec));
    GameList = (GameRec *)GlobalLock(hGameList);
    hTTable = xalloc(2 * TTBLSZ * sizeof(hashentry));
    ttable = (hashentry *)GlobalLock(hTTable);
    Level = 0;
    TCflag = false;
    OperatorTime = 0;
    Initialize_dist();
    Initialize_moves();
}

void Sim::Initialize_dist()
{
    for (short a = 0; a < 64; a++)
    {
        for (short b = 0; b < 64; b++)
        {
            short d = ::abs(column(a) - column(b));
            short di = ::abs((a >> 3) - (b >> 3));
            *(taxidata + a * 64 + b) = d + di;
            *(distdata + a * 64 + b) = d > di ? d : di;
        }
    }
}

void Sim::FreeGlobals()
{
    if (hHistory)
    {
        GlobalUnlock(hHistory);
        GlobalFree(hHistory);
    }

    if (hnextdir)
    {
        GlobalUnlock(hnextdir);
        GlobalFree(hnextdir);
    }

    if (hGameList)
    {
        GlobalUnlock(hGameList);
        GlobalFree(hGameList);
    }

    if (hTTable)
    {
        GlobalUnlock(hTTable);
        GlobalFree(hTTable);
    }

    if (hTree)
    {
        GlobalUnlock(hTree);
        GlobalFree(hTree);
    }

    if (hHashCode)
    {
        GlobalUnlock(hHashCode);
        GlobalFree(hHashCode);
    }

    if (htaxidata)
    {
        GlobalUnlock(htaxidata);
        GlobalFree(htaxidata);
    }

    if (hdistdata)
    {
        GlobalUnlock(hdistdata);
        GlobalFree(hdistdata);
    }

    if (hnextpos)
    {
        GlobalUnlock(hnextpos);
        GlobalFree(hnextpos);
    }
}

short const Sim::Stboard[64] = {
    ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK,
    PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN,
    ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK};

short const Sim::Stcolor[64] = {
    white, white, white, white, white, white, white, white,
    white, white, white, white, white, white, white, white,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    black, black, black, black, black, black, black, black,
    black, black, black, black, black, black, black, black};

short const Sim::direc[8][8] = {
    {  0,  0,   0,   0,  0,   0,  0,   0},
    { 10,  9,  11,   0,  0,   0,  0,   0},
    {  8, -8,  12, -12, 19, -19, 21, -21},
    {  9, 11,  -9, -11,  0,   0,  0,   0},
    {  1, 10,  -1, -10,  0,   0,  0,   0},
    {  1, 10,  -1, -10,  9,  11, -9, -11},
    {  1, 10,  -1, -10,  9,  11, -9, -11},
    {-10, -9, -11,   0,  0,   0,  0,   0}};

short const Sim::max_steps[8] = {0, 2, 1, 7, 7, 7, 1, 2};

short const Sim::nunmap[120] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7, -1,
    -1,  8,  9, 10, 11, 12, 13, 14, 15, -1,
    -1, 16, 17, 18, 19, 20, 21, 22, 23, -1,
    -1, 24, 25, 26, 27, 28, 29, 30, 31, -1,
    -1, 32, 33, 34, 35, 36, 37, 38, 39, -1,
    -1, 40, 41, 42, 43, 44, 45, 46, 47, -1,
    -1, 48, 49, 50, 51, 52, 53, 54, 55, -1,
    -1, 56, 57, 58, 59, 60, 61, 62, 63, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

/*
  This procedure pre-calculates all moves for every piece from every square.
  This data is stored in nextpos/nextdir and used later in the move generation
  routines.
*/
void Sim::Initialize_moves()
{
    short d, di, delta;

    for (short ptyp = 0; ptyp < 8; ptyp++)
    {
        for (short po = 0; po < 64; po++)
        {
            for (short p0 = 0; p0 < 64; p0++)
            {
                *(nextpos + ptyp * 64 * 64 + po * 64 + p0) = BYTE(po);
                *(nextdir + ptyp * 64 * 64 + po * 64 + p0) = BYTE(po);
            }
        }
    }

    short dest[8][8];
    short steps[8];
    short sorted[8];
    short s;

    for (short ptyp = 1; ptyp < 8; ptyp++)
    {
        for (short po = 21; po < 99; po++)
        {
            if (nunmap[po] < 0)
                continue;

            BYTE *ppos = nextpos + ptyp * 64 * 64 + nunmap[po] * 64;
            BYTE *pdir = nextdir + ptyp * 64 * 64 + nunmap[po] * 64;

            for (d = 0; d < 8; d++)
            {
                dest[d][0] = nunmap[po];
                delta = direc[ptyp][d];

                if (delta != 0)
                {
                    short p0 = po;
                    for (s = 0; s < max_steps[ptyp]; s++)
                    {
                        p0 += delta;
                        /*
                          break if (off board) or
                          (pawns only move two steps from home square)
                        */
                        if (nunmap[p0] < 0 || ((ptyp == PAWN || ptyp == BPAWN) && s > 0 && (d > 0 || Stboard[nunmap[po]] != PAWN)))
                        {
                            break;
                        }

                        dest[d][s] = nunmap[p0];

                    }
                }
                else
                {
                    s = 0;
                }

                /*
                  sort dest in number of steps order
                  currently no sort is done due to compability with
                  the move generation order in old gnu chess
                */
                steps[d] = s;

                for (di = d; s > 0 && di > 0; di--)
                {
                    /* should be: < s */
                    if (steps[sorted[di - 1]] != 0)
                        break;

                    sorted[di] = sorted[di - 1];
                }
                sorted[di] = d;
            }

            /*
              update nextpos/nextdir,
              pawns have two threads (capture and no capture)
            */
            short p0 = nunmap[po];

            if (ptyp == PAWN || ptyp == BPAWN)
            {
                for (s = 0; s < steps[0]; s++)
                {
                    ppos[p0] = (unsigned char) dest[0][s];
                    p0 = dest[0][s];
                }

                p0 = nunmap[po];

                for (d = 1; d < 3; d++)
                {
                    pdir[p0] = (unsigned char) dest[d][0];
                    p0 = dest[d][0];
                }

                continue;
            }

            pdir[p0] = BYTE(dest[sorted[0]][0]);

            for (d = 0; d < 8; d++)
            {
                for (s = 0; s < steps[sorted[d]]; s++)
                {
                    ppos[p0] = BYTE(dest[sorted[d]][s]);
                    p0 = dest[sorted[d]][s];

                    if (d < 7)
                        pdir[p0] = BYTE(dest[sorted[d + 1]][0]);
                    /* else is already initialized */
                }
            }
        }
    }
}

void Sim::maxSearchDepth(short n)
{
    _maxSearchDepth = n;
}

short Sim::maxSearchDepth() const
{
    return _maxSearchDepth;
}

short const Sim::ptype[2][8] = {
    {NO_PIECE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NO_PIECE},
    {NO_PIECE, BPAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NO_PIECE}};

short const Sim::KBNK[64] = {
    99, 90, 80, 70, 60, 50, 40, 40,
    90, 80, 60, 50, 40, 30, 20, 40,
    80, 60, 40, 30, 20, 10, 30, 50,
    70, 50, 30, 10,  0, 20, 40, 60,
    60, 40, 20,  0, 10, 30, 50, 70,
    50, 30, 10, 20, 30, 40, 60, 80,
    40, 20, 30, 40, 50, 60, 80, 90,
    40, 40, 50, 60, 70, 80, 90, 99};

short const Sim::KingOpening[64] = {
      0,   0,  -4, -10, -10,  -4,   0,   0,
     -4,  -4,  -8, -12, -12,  -8,  -4,  -4,
    -12, -16, -20, -20, -20, -20, -16, -12,
    -16, -20, -24, -24, -24, -24, -20, -16,
    -16, -20, -24, -24, -24, -24, -20, -16,
    -12, -16, -20, -20, -20, -20, -16, -12,
     -4,  -4,  -8, -12, -12,  -8,  -4,  -4,
      0,   0,  -4, -10, -10,  -4,   0,   0};

short const Sim::KingEnding[64] = {
     0,  6, 12, 18, 18, 12,  6,  0,
     6, 12, 18, 24, 24, 18, 12,  6,
    12, 18, 24, 30, 30, 24, 18, 12,
    18, 24, 30, 36, 36, 30, 24, 18,
    18, 24, 30, 36, 36, 30, 24, 18,
    12, 18, 24, 30, 30, 24, 18, 12,
     6, 12, 18, 24, 24, 18, 12,  6,
     0,  6, 12, 18, 18, 12,  6,  0};

short const Sim::pknight[64] = {
     0,  4,  8, 10, 10,  8,  4,  0,
     4,  8, 16, 20, 20, 16,  8,  4,
     8, 16, 24, 28, 28, 24, 16,  8,
    10, 20, 28, 32, 32, 28, 20, 10,
    10, 20, 28, 32, 32, 28, 20, 10,
     8, 16, 24, 28, 28, 24, 16,  8,
     4,  8, 16, 20, 20, 16,  8,  4,
     0,  4,  8, 10, 10,  8,  4,  0};

short const Sim::pbishop[64] = {
    14, 14, 14, 14, 14, 14, 14, 14,
    14, 22, 18, 18, 18, 18, 22, 14,
    14, 18, 22, 22, 22, 22, 18, 14,
    14, 18, 22, 22, 22, 22, 18, 14,
    14, 18, 22, 22, 22, 22, 18, 14,
    14, 18, 22, 22, 22, 22, 18, 14,
    14, 22, 18, 18, 18, 18, 22, 14,
    14, 14, 14, 14, 14, 14, 14, 14};

short const Sim::PawnAdvance[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     4,  4,  4,  0,  0,  4,  4,  4,
     6,  8,  2, 10, 10,  2,  8,  6,
     6,  8, 12, 16, 16, 12,  8,  6,
     8, 12, 16, 24, 24, 16, 12,  8,
    12, 16, 24, 32, 32, 24, 16, 12,
    12, 16, 24, 32, 32, 24, 16, 12,
     0,  0,  0,  0,  0,  0,  0,  0};

/*
  Find the best move in the tree between indexes p1 and p2. Swap the best
  move into the p1 element.
*/
void pick(short p1, short p2)
{
    short s0 = Tree[p1].score;
    short p0 = p1;

    for (short p = p1 + 1; p <= p2; p++)
    {
        short s = Tree[p].score;

        if (s > s0)
        {
            s0 = s;
            p0 = p;
        }
    }

    if (p0 != p1)
    {
        Leaf temp = Tree[p1];
        Tree[p1] = Tree[p0];
        Tree[p0] = temp;
    }
}

/*
  Reset the board and other variables to start a new game.
*/
void Sim::NewGame(HWND hWnd, HWND compClr)
{
    /* the game is not yet started */
    stage = stage2 = -1;

    if (flag.post)
    {
        ::SendMessage(hStats, WM_SYSCOMMAND, SC_CLOSE, 0);
        flag.post = false;
    }

    flag.reverse = false;
    flag.mate = flag.quit = flag.bothsides = false;
    flag.force = false;
    flag.hash = flag.easy = flag.beep = flag.rcptr = true;
    NodeCnt = et0 = epsquare = 0;
    dither = 0;
    _awindow = 90;
    Bwindow = 90;
    xwndw = 90;
    _maxSearchDepth = 29;
    contempt = 0;
    GameCnt = 0;
    Game50 = 1;
    hint = 0x0C14;
    ZeroRPT();
    Developed[white] = Developed[black] = false;
    castld[white] = castld[black] = false;
    PawnThreat[0] = CptrFlag[0] = false;
    Pscore[0] = 12000;
    Tscore[0] = 12000;
    opponent = white;
    computer = black;

    for (short l = 0; l < 2000; l++)
        (Tree + l)->f = (Tree + l)->t = 0;
#if TTBLSZ
    rehash = 6;
    ZeroTTable();
    ::srand((DWORD)1);

    for (short c = white; c <= black; c++)
    {
        for (short p = 1; p <= 6; p++)
        {
            for (short l = 0; l < 64; l++)
            {
                (hashcode + c * 7*64+p*64+l)->key = DWORD(::rand());
                (hashcode + c * 7*64+p*64+l)->key += DWORD(::rand()) << 16;
                (hashcode + c * 7*64+p*64+l)->bd = DWORD(::rand());
                (hashcode + c * 7*64+p*64+l)->bd += DWORD(::rand()) << 16;
            }
        }
    }
#endif
    for (short l = 0; l < 64; l++)
    {
        ::board[l] = Stboard[l];
        ::color[l] = Stcolor[l];
        ::Mvboard[l] = 0;
    }

    if (TCflag)
    {
        SetTimeControl(::ft);
    }
    else if (Level == 0)
    {
        ::OperatorTime = 1;
        ::TCmoves = 60;
        ::TCminutes = 5;
        ::TCflag = ::TCmoves > 1 ? 1 : 0;
        SetTimeControl(::ft);
    }

    InitializeStats();
    SystemTime sysTime;
    sysTime.getTime();
    time0 = long(sysTime.unix());
    ElapsedTime(1, ExtraTime, ResponseTime, ::ft);
    UpdateDisplay(hWnd, compClr, 0, 0, 1, 0, flag.reverse);
    flag.easy = true;
    flag.hash = true;
    hashfile = NULL;
}

/*
  Look for the current board position in the transposition table.
*/
int Sim::ProbeTTable(short side, short depth, short *alpha, short *beta, short *score)
{
    struct hashentry *ptbl;
    WORD i;
    ptbl = ttable + side * 2 + (hashkey & (TTBLSZ - 1));

    /* rehash max rehash times */
    for (i = 1; ptbl->hashbd != hashbd && i <= rehash; i++)
        ptbl = ttable+side*2+((hashkey + i) & (TTBLSZ - 1));

    if ((short) ptbl->depth >= depth && ptbl->hashbd == hashbd)
    {
        HashCnt++;
#ifdef HASHTEST
        for (i = 0; i < 32; i++)
        {
            if (ptbl->bd[i] != CB(i))
            {
                HashCol++;
                ::MessageBox(0, TEXT("ttable collision detected"), TEXT("Chess"), MB_OK);
                break;
            }
        }
#endif
        PV = ptbl->mv;

        if (ptbl->flags & TRUESCORE)
        {
            *score = ptbl->score;
            *beta = -20000;
        }
#if 0 /* commented out, why? */
        else if (ptbl->flags & UPPERBOUND)
        {
            if (ptbl->score < *beta)
                *beta = ptbl->score+1;
        }
#endif
        else if (ptbl->flags & LOWERBOUND)
        {
            if (ptbl->score > *alpha)
                *alpha = ptbl->score - 1;
        }
        return true;
    }
    return false;
}

//Store the current board position in the transposition table.
void Sim::PutInTTable(short side, short score, short depth, short alpha, short beta, WORD mv)
{
    hashentry *ptbl;
    WORD i;
    ptbl = ttable + side * 2 + (hashkey & (TTBLSZ - 1));

    /* rehash max rehash times */
    for (i = 1; depth < ptbl->depth && ptbl->hashbd != hashbd && i <= rehash; i++)
        ptbl = ttable + side * 2 + ((hashkey + i) & (TTBLSZ - 1));

    if (depth >= ptbl->depth || ptbl->hashbd != hashbd)
    {
        ptbl->hashbd = hashbd;
        ptbl->depth = (unsigned char) depth;
        ptbl->score = score;
        ptbl->mv = mv;
        ptbl->flags = 0;

        if (score < alpha)
            ptbl->flags |= UPPERBOUND;
        else if (score > beta)
            ptbl->flags |= LOWERBOUND;
        else
            ptbl->flags |= TRUESCORE;
#ifdef HASHTEST
        for (i = 0; i < 32; i++)
        {
          ptbl->bd[i] = CB(i);
        }
#endif
    }
}

static void ShowScore(HWND hwnd, short score)
{
    TCHAR tmp[30];

    if (hwnd)
    {
        ::wsprintf(tmp, TEXT("%d"), score);
        ::SetDlgItemText(hwnd, SCORETEXT, tmp);
    }
}

void Sim::ShowDepth(HWND hwnd, char ch)
{
    TCHAR tmp[30];

    if (hwnd)
    {
        ::wsprintf(tmp, TEXT("%d%c"), Sdepth, ch);
        ::SetDlgItemText(hwnd, DEPTHTEXT, tmp);
    }
}

void Sim::ShowResults(HWND hwnd, short score, PWORD bstline, char ch)
{
    if (!flag.post)
        return;

    ShowDepth(hStats, ch);
    ShowScore(hwnd, score);
    int s = 0;
    TCHAR str[300];

    for (BYTE ply = 1; bstline[ply] > 0; ply++)
    {
        algbr(short(bstline[ply]) >> 8, short(bstline[ply]) & 0xFF, false);

        if (ply == 5 || ply == 9 || ply == 13 || ply == 17)
            s += wsprintf(str + s, TEXT("\n"));

        s += ::wsprintf(str + s, TEXT("%-5s "), LPTSTR(mvstr[0]));
    }
    ::SetDlgItemText(hwnd, BSTLINETEXT, LPTSTR(str));
}

void Sim::updateSearchStatus(short &d, short pnt, short f,
                   short t, short best, short alpha, short score)
{
    if (flag.post)
        ShowCurrentMove(hStats, pnt, f, t);

    if (pnt <= TrPnt[1])
        return;

    d = best - Zscore;
    short e = best - score;

    if (best < alpha)
        ExtraTime = 10 * ResponseTime;
    else if (d > -zwndw && e > 4 * zwndw)
        ExtraTime = -ResponseTime / 3;
    else if (d > -zwndw)
        ExtraTime = 0;
    else if (d > -3 * zwndw)
        ExtraTime = ResponseTime;
    else if (d > -9 * zwndw)
        ExtraTime = 3 * ResponseTime;
    else
        ExtraTime = 5 * ResponseTime;
}

bool Sim::_mateThreat(short ply)
{
    return ply < Sdepth + 4 && ply > 4 && ChkFlag[ply - 2] &&
            ChkFlag[ply - 4] && ChkFlag[ply - 2] != ChkFlag[ply - 4];
}

/*
  Check for draw by threefold repetition.
*/
void Sim::repetition(short *cnt)
{
    short c = 0;
    short b[64];
    *cnt = 0;

    if (GameCnt > Game50 + 3)
    {
        ::memset((char *)b, 0, sizeof (b));

        for (short i = GameCnt; i > Game50; i--)
        {
            WORD m = GameList[i].gmove;
            short f = m >> 8;
            short t = m & 0xFF;

            if (++b[f] == 0)
                c--;
            else
                c++;

            if (--b[t] == 0)
                c--;
            else
                c++;

            if (c == 0)
                (*cnt)++;
        }
    }
}

bool Sim::xrecapture(short score, short alpha, short beta, short ply)
{
    return flag.rcptr && score > alpha && score < beta && ply > 2 && CptrFlag[ply - 1] && CptrFlag[ply - 2];
}

short Sim::xdistance(short a, short b)
{
    return *(distdata + a * 64 + b);
}

short Sim::taxicab(short a, short b)
{
    return *(taxidata + a * 64 + b);
}

/*
  Score King and Pawns versus King endings.
*/
int Sim::ScoreKPK(short side, short winner, short loser,
    short king1, short king2, short sq)
{
    short s = PieceCnt[winner] == 1 ? 50 : 120;

    if (winner == white)
    {
        short r = side == loser ? row(sq) - 1 : row(sq);

        if (row(king2) >= r && xdistance(sq, king2) < 8 - r)
            s += 10 * row(sq);
        else
            s = 500 + 50 * row(sq);

        if (row(sq) < 6)
            sq += 16;
        else if (row(sq) == 6)
            sq += 8;
    }
    else
    {
        short r = side == loser ? row(sq) + 1 : row(sq);

        if (row (king2) <= r && xdistance(sq, king2) < r + 1)
            s += 10 * (7 - row(sq));
        else
            s = 500 + 50 * (7 - row(sq));

        if (row(sq) > 1)
            sq -= 16;
        else if (row(sq) == 1)
            sq -= 8;
    }

    s += 8 * (taxicab(king2, sq) - taxicab(king1, sq));
    return s;
}

/*
  Score King+Bishop+Knight versus King endings.
  This doesn't work all that well but it's better than nothing.
*/
int Sim::_scoreKBNK(short winner, short king1, short king2)
{
    short KBNKsq = 0;

    for (short sq = 0; sq < 64; sq++)
        if (board[sq] == BISHOP)
            KBNKsq = row(sq) % 2 == column(sq) % 2 ? 0 : 7;

    short s = emtl[winner] - 300;

    if (KBNKsq == 0)
        s += KBNK[king2];
    else
        s += KBNK[locn(row(king2), 7 - column(king2))];

    s -= taxicab(king1, king2);
    s -= xdistance(PieceList[winner][1], king2);
    s -= xdistance(PieceList[winner][2], king2);
    return s;
}

/*
  If material balance has changed, determine the values for the positional
  evaluation terms.
*/
void Sim::UpdateWeights()
{
    emtl[white] = mtl[white] - pmtl[white] - VALUEK;
    emtl[black] = mtl[black] - pmtl[black] - VALUEK;
    short tmtl = emtl[white] + emtl[black];
    short s1 = tmtl > 6600 ? 0 : ((tmtl < 1400) ? 10 : (6600 - tmtl) / 520);

    if (s1 != stage)
    {
        stage = s1;
        stage2 = tmtl > 3600 ? 0 : (tmtl < 1400 ? 10 : (3600 - tmtl) / 220);
        PEDRNK2B = -15;   /* centre pawn on 2nd rank & blocked */
        PBLOK = -4;               /* blocked backward pawn */
        PDOUBLED = -14;   /* doubled pawn */
        PWEAKH = -4;              /* weak pawn on half open file */
        PAWNSHIELD = 10 - stage;  /* pawn near friendly king */
        PADVNCM = 10;             /* advanced pawn multiplier */
        PADVNCI = 7;              /* muliplier for isolated pawn */
        PawnBonus = stage;
        KNIGHTPOST = (stage + 2) / 3;     /* knight near enemy pieces */
        KNIGHTSTRONG = (stage + 6) / 2;   /* occupies pawn hole */
        BISHOPSTRONG = (stage + 6) / 2;   /* occupies pawn hole */
        BishopBonus = 2 * stage;
        RHOPN = 10;               /* rook on half open file */
        RHOPNX = 4;
        RookBonus = 6 * stage;
        XRAY = 8;         /* Xray attack on piece */
        PINVAL = 10;              /* Pin */
        KHOPN = (3 * stage - 30) / 2;     /* king on half open file */
        KHOPNX = KHOPN / 2;
        KCASTLD = 10 - stage;
        KMOVD = -40 / (stage + 1);        /* king moved before castling */
        KATAK = (10 - stage) / 2; /* B,R attacks near enemy king */
        KSFTY = stage < 8 ? 16 - 2 * stage : 0;
        ATAKD = -6;               /* defender > attacker */
        HUNGP = -8;               /* each hung piece */
        HUNGX = -12;              /* extra for >1 hung piece */
    }
}

short const Sim::DyingKing[64] = {
     0,  8, 16, 24, 24, 16,  8,  0,
     8, 32, 40, 48, 48, 40, 32,  8,
    16, 40, 56, 64, 64, 56, 40, 16,
    24, 48, 64, 72, 72, 64, 48, 24,
    24, 48, 64, 72, 72, 64, 48, 24,
    16, 40, 56, 64, 64, 56, 40, 16,
     8, 32, 40, 48, 48, 40, 32,  8,
     0,  8, 16, 24, 24, 16,  8,  0};

/*
  Static evaluation when loser has only a king and winner has no pawns or no
  pieces.
*/
void Sim::ScoreLoneKing(short int side, short int *score)
{
    UpdateWeights();
    short winner = mtl[white] > mtl[black] ? white : black;
    short loser = otherside[winner];
    short king1 = PieceList[winner][0];
    short king2 = PieceList[loser][0];
    short s = 0;

    if (pmtl[winner] > 0)
    {
        for (short i = 1; i <= PieceCnt[winner]; i++)
            s += ScoreKPK(side, winner, loser, king1, king2, PieceList[winner][i]);
    }
    else if (emtl[winner] == VALUEB + VALUEN)
    {
        s = _scoreKBNK(winner, king1, king2);
    }
    else if (emtl[winner] > VALUEB)
    {
        s = 500 + emtl[winner] - DyingKing[king2] - 2 * xdistance(king1, king2);
    }

    *score = side == winner ? s : -s;
}

bool Sim::anyatak(short c, short u)
{
    return atak[c][u] > 0;
}

/*
  Fill array atak[][] with info about ataks to a square.  Bits 8-15 are set
  if the piece (king..pawn) ataks the square.  Bits 0-7 contain a count of
  total ataks to the square.
*/
void Sim::_ataks(short side, short *a)
{
    BYTE *ppos, *pdir;
    memset((char *)a, 0, 64 * sizeof(a[0]));
    short *PL = PieceList[side];

    for (short i = PieceCnt[side]; i >= 0; i--)
    {
        short sq = PL[i];
        short piece = board[sq];
        short c = control[piece];

        if (sweep[piece])
        {
            ppos = nextpos + piece * 64 * 64 + sq * 64;
            pdir = nextdir + piece * 64 * 64 + sq * 64;
            short u = ppos[sq];

            do
            {
                a[u] = (a[u] + 1) | c;
                u = color[u] == NEUTRAL ? ppos[u] : pdir[u];
            }
            while (u != sq);
        }
        else
        {
            pdir = nextdir + ptype[side][piece]*64*64+sq*64;
            short u = pdir[sq]; /* follow captures thread for pawns */

            do
            {
                a[u] = (a[u] + 1) | c;
                u = pdir[u];
            }
            while (u != sq);
        }
    }
}

short const Sim::BACKWARD[16] = {
     -6, -10, -15, -21, -28, -28, -28, -28,
    -28, -28, -28, -28, -28, -28, -28, -28};

short const Sim::BMBLTY[14] = {
    -2, 0, 2, 4, 6, 8, 10, 12, 13, 14, 15, 16, 16, 16};

short const Sim::RMBLTY[15] = {
    0, 2, 4, 6, 8, 10, 11, 12, 13, 14, 14, 14, 14, 14, 14};

short const Sim::KTHRT[36] = {
      0,  -8, -20, -36, -52, -68, -80, -80, -80, -80, -80, -80,
    -80, -80, -80, -80, -80, -80, -80, -80, -80, -80, -80, -80,
    -80, -80, -80, -80, -80, -80, -80, -80, -80, -80, -80, -80
};

#if 0
static short qrook[3] = {0, 56, 0};
static short krook[3] = {7, 63, 0};
#endif

short Sim::_wking()
{
    return ::PieceList[white][0];
}

short Sim::_bking()
{
    return ::PieceList[black][0];
}

short Sim::_enemyKing()
{
    return ::PieceList[c2][0];
}

/*
  See if the attacked piece has unattacked squares to move to.
  The following must be true:
  c1 == color[sq]
  c2 == otherside[c1]
*/
int Sim::trapped(short sq)
{
    short piece = board[sq];
    BYTE *ppos = nextpos + (ptype[c1][piece] * 64 * 64) + sq * 64;
    BYTE *pdir = nextdir + (ptype[c1][piece] * 64 * 64) + sq * 64;

    if (piece == PAWN)
    {
        /* follow no captures thread */
        short u = ppos[sq];

        if (color[u] == NEUTRAL)
        {
            if (atk1[u] >= atk2[u])
                return false;

            if (atk2[u] < CTLP)
            {
                u = ppos[u];

                if (color[u] == NEUTRAL && atk1[u] >= atk2[u])
                    return false;
            }
        }
        u = pdir[sq];     /* follow captures thread */

        if (color[u] == c2)
            return false;

        u = pdir[u];

        return color[u] == c2 ? false : true;
    }

    short u = ppos[sq];

    do
    {
        if (color[u] != c1)
            if (atk2[u] == 0 || board[u] >= piece)
                return false;

        u = color[u] == NEUTRAL ? ppos[u] : pdir[u];
    }
    while (u != sq);

    return true;
}

/*
  Calculate the positional value for a pawn on 'sq'.
*/
int Sim::PawnValue(short sq, short side)
{
    short j, in_square, r, e;
    short a1 = atk1[sq] & 0x4FFF;
    short a2 = atk2[sq] & 0x4FFF;
    short rank = row(sq);
    short fyle = column(sq);
    short s = 0;

    if (c1 == white)
    {
        s = Mwpawn[sq];

        if ((sq == 11 && color[19] != NEUTRAL) || (sq == 12 && color[20] != NEUTRAL))
            s += PEDRNK2B;

        if ((fyle == 0 || PC1[fyle - 1] == 0) && (fyle == 7 || PC1[fyle + 1] == 0))
            s += ISOLANI[fyle];
        else if (PC1[fyle] > 1)
            s += PDOUBLED;

        if (a1 < CTLP && atk1[sq + 8] < CTLP)
        {
            s += BACKWARD[a2 & 0xFF];

            if (PC2[fyle] == 0)
                s += PWEAKH;

            if (color[sq + 8] != NEUTRAL)
                s += PBLOK;
        }

        if (PC2[fyle] == 0)
        {
            r = side == black ? rank - 1 : rank;
            in_square = row(_bking()) >= r && xdistance(sq, _bking()) < 8 - r;
            e = a2 == 0 || side == white ? 0 : 1;

            for (j = sq + 8; j < 64; j += 8)
            {
                if (atk2[j] >= CTLP)
                {
                    e = 2;
                    break;
                }
                else if (atk2[j] > 0 || color[j] != NEUTRAL)
                {
                    e = 1;
                }
            }

            if (e == 2)
                s += (stage * PassedPawn3[rank]) / 10;
            else if (in_square || e == 1)
                s += (stage * PassedPawn2[rank]) / 10;
            else if (emtl[black] > 0)
                s += (stage * PassedPawn1[rank]) / 10;
            else
                s += PassedPawn0[rank];
        }
    }
    else if (c1 == black)
    {
        s = Mbpawn[sq];

        if ((sq == 51 && color[43] != NEUTRAL) || (sq == 52 && color[44] != NEUTRAL))
            s += PEDRNK2B;

        if ((fyle == 0 || PC1[fyle - 1] == 0) && (fyle == 7 || PC1[fyle + 1] == 0))
            s += ISOLANI[fyle];
        else if (PC1[fyle] > 1)
            s += PDOUBLED;

        if (a1 < CTLP && atk1[sq - 8] < CTLP)
        {
            s += BACKWARD[a2 & 0xFF];

            if (PC2[fyle] == 0)
                s += PWEAKH;

            if (color[sq - 8] != NEUTRAL)
                s += PBLOK;
        }

        if (PC2[fyle] == 0)
        {
            r = side == white ? rank + 1 : rank;
            in_square = (row(_wking()) <= r && xdistance(sq, _wking()) < r + 1);
            e = a2 == 0 || side == black ? 0 : 1;

            for (j = sq - 8; j >= 0; j -= 8)
            {
                if (atk2[j] >= CTLP)
                {
                    e = 2;
                    break;
                }
                else if (atk2[j] > 0 || color[j] != NEUTRAL)
                {
                    e = 1;
                }
            }

            if (e == 2)
                s += (stage * PassedPawn3[7 - rank]) / 10;
            else if (in_square || e == 1)
                s += (stage * PassedPawn2[7 - rank]) / 10;
            else if (emtl[white] > 0)
                s += (stage * PassedPawn1[7 - rank]) / 10;
            else
                s += PassedPawn0[7 - rank];
        }
    }

    if (a2 > 0)
    {
        if (a1 == 0 || a2 > CTLP + 1)
        {
            s += HUNGP;
            ++hung[c1];

            if (trapped(sq))
                ++hung[c1];
        }
        else if (a2 > a1)
        {
            s += ATAKD;
        }
    }
    return s;
}

/*
  Calculate the positional value for a knight on 'sq'.
*/
int Sim::KnightValue(short sq, short)
{
    short s = Mknight[c1][sq];
    short a2 = atk2[sq] & 0x4FFF;

    if (a2 > 0)
    {
        short a1 = atk1[sq] & 0x4FFF;

        if (a1 == 0 || a2 > CTLBN + 1)
        {
            s += HUNGP;
            ++hung[c1];

            if (trapped(sq))
                ++hung[c1];
        }
        else if (a2 >= CTLBN || a1 < CTLP)
        {
            s += ATAKD;
        }
    }
    return s;
}

/*
  Find Bishop and Rook mobility, XRAY attacks, and pins. Increment the
  hung[] array if a pin is found.
*/
void Sim::BRscan(short sq, short *s, short *mob)
{
    short *Kf = Kfield[c1];
    *mob = 0;
    short piece = board[sq];
    BYTE *ppos = nextpos + piece * 64 * 64 + sq * 64;
    BYTE *pdir = nextdir + piece * 64 * 64 + sq * 64;
    short u = ppos[sq];
    short pin = -1;                     /* start new direction */

    do
    {
        *s += Kf[u];

        if (color[u] == NEUTRAL)
        {
            (*mob)++;

            /* oops new direction */
            if (ppos[u] == pdir[u])
                pin = -1;

            u = ppos[u];
        }
        else if (pin < 0)
        {
            if (board[u] == PAWN || board[u] == KING)
            {
                u = pdir[u];
            }
            else
            {
                /* not on the edge and on to find a pin */
                if (ppos[u] != pdir[u])
                    pin = u;

                u = ppos[u];
            }
        }
        else
        {
            if (color[u] == c2 && (board[u] > piece || atk2[u] == 0))
            {
                if (color[pin] == c2)
                {
                    *s += PINVAL;

                    if (atk2[pin] == 0 || atk1[pin] > control[board[pin]] + 1)
                        ++hung[c2];
                }
                else
                {
                    *s += XRAY;
                }
            }

            /* new direction */
            pin = -1;
            u = pdir[u];
        }
    }
    while (u != sq);
}

/*
  Calculate the positional value for a bishop on 'sq'.
*/
int Sim::BishopValue(short sq, short)
{
    short mob;
    short s = Mbishop[c1][sq];
    BRscan(sq, &s, &mob);
    s += BMBLTY[mob];
    short a2 = atk2[sq] & 0x4FFF;

    if (a2 > 0)
    {
        short a1 = atk1[sq] & 0x4FFF;

        if (a1 == 0 || a2 > CTLBN + 1)
        {
            s += HUNGP;
            ++hung[c1];

            if (trapped(sq))
                ++hung[c1];
        }
        else if (a2 >= CTLBN || a1 < CTLP)
        {
            s += ATAKD;
        }
    }
    return s;
}

/*
  Calculate the positional value for a rook on 'sq'.
*/
int Sim::_rookValue(short sq, short)
{
    short mob;
    short s = RookBonus;
    BRscan(sq, &s, &mob);
    s += RMBLTY[mob];
    short fyle = column(sq);

    if (PC1[fyle] == 0)
        s += RHOPN;

    if (PC2[fyle] == 0)
        s += RHOPNX;

    if (pmtl[c2] > 100 && row(sq) == rank7[c1])
        s += 10;

    if (stage > 2)
        s += 14 - taxicab(sq, _enemyKing());

    short a2 = atk2[sq] & 0x4FFF;

    if (a2 > 0)
    {
        short a1 = atk1[sq] & 0x4FFF;

        if (a1 == 0 || a2 > CTLR + 1)
        {
            s += HUNGP;
            ++hung[c1];

            if (trapped(sq))
                ++hung[c1];
        }
        else if (a2 >= CTLR || a1 < CTLP)
        {
            s += ATAKD;
        }
    }
    return s;
}

//Calculate the positional value for a queen on 'sq'.
int Sim::QueenValue(short sq, short)
{
    short s = xdistance(sq, _enemyKing()) < 3 ? 12 : 0;

    if (stage > 2)
        s += 14 - taxicab(sq, _enemyKing());

    short a2 = atk2[sq] & 0x4FFF;

    if (a2 > 0)
    {
        short a1 = atk1[sq] & 0x4FFF;

        if (a1 == 0 || a2 > CTLQ + 1)
        {
            s += HUNGP;
            ++hung[c1];

            if (trapped(sq))
                ++hung[c1];
        }
        else if (a2 >= CTLQ || a1 < CTLP)
        {
            s += ATAKD;
        }
    }
    return s;
}

/*
  Assign penalties if king can be threatened by checks, if squares
  near the king are controlled by the enemy (especially the queen),
  or if there are no pawns near the king.
  The following must be true:
  board[sq] == king
  c1 == color[sq]
  c2 == otherside[c1]
*/
void Sim::ScoreThreat2(short u, short &cnt, short *s)
{
    if (::color[u] == c2)
        return;

    if (atk1[u] == 0 || (atk2[u] & 0xff) > 1)
        ++cnt;
    else
        *s -= 3;
}

void Sim::KingScan(short sq, short *s)
{
    BYTE *ppos, *pdir;
    short cnt = 0;

    if (HasBishop[c2] || HasQueen[c2])
    {
        ppos = nextpos + BISHOP * 64 * 64 + sq * 64;
        pdir = nextdir + BISHOP * 64 * 64 + sq * 64;
        short u = ppos[sq];

        do
        {
            if (atk2[u] & CTLBQ)
                ScoreThreat2(u, cnt, s);

            u = color[u] == NEUTRAL ? ppos[u] : pdir[u];
        }
        while (u != sq);
    }

    if (HasRook[c2] || HasQueen[c2])
    {
        ppos = nextpos + ROOK * 64 * 64 + sq * 64;
        pdir = nextdir + ROOK * 64 * 64 + sq * 64;
        short u = ppos[sq];

        do
        {
            if (atk2[u] & CTLRQ)
                ScoreThreat2(u, cnt, s);

            u = color[u] == NEUTRAL ? ppos[u] : pdir[u];
        }
        while (u != sq);
    }

    if (HasKnight[c2])
    {
        pdir = nextdir + KNIGHT * 64 * 64 + sq * 64;
        short u = pdir[sq];

        do
        {
            if (atk2[u] & CTLNN)
                ScoreThreat2(u, cnt, s);

            u = pdir[u];
        }
        while (u != sq);
    }

    *s += (KSFTY * KTHRT[cnt]) / 16;
    cnt = 0;
    short ok = false;
    pdir = nextpos + KING * 64 * 64 + sq * 64;
    short u = pdir[sq];

    do
    {
        if (board[u] == PAWN)
            ok = true;

        if (atk2[u] > atk1[u])
        {
            ++cnt;
            if (atk2[u] & CTLQ)
                if (atk2[u] > CTLQ + 1 && atk1[u] < CTLQ)
                    *s -= 4 * KSFTY;
        }
        u = pdir[u];
    }
    while (u != sq);

    if (!ok)
        *s -= KSFTY;

    if (cnt > 1)
        *s -= KSFTY;
}

//Calculate the positional value for a king on 'sq'.
int Sim::KingValue(short sq, short)
{
    short s = Mking[c1][sq];

    if (KSFTY > 0)
        if (Developed[c2] || stage > 0)
            KingScan(sq, &s);

    if (castld[c1])
        s += KCASTLD;
    else if (Mvboard[kingP[c1]])
        s += KMOVD;

    short fyle = column(sq);

    if (PC1[fyle] == 0)
        s += KHOPN;

    if (PC2[fyle] == 0)
        s += KHOPNX;

    switch (fyle)
    {
    case 5:
        if (PC1[7] == 0)
            s += KHOPN;

        if (PC2[7] == 0)
            s += KHOPNX;
        /* Fall through */
    case 4:
    case 6:
    case 0:
        if (PC1[fyle + 1] == 0)
            s += KHOPN;

        if (PC2[fyle + 1] == 0)
            s += KHOPNX;
        break;
    case 2:
        if (PC1[0] == 0)
            s += KHOPN;
        if (PC2[0] == 0)
            s += KHOPNX;
        /* Fall through */
    case 3:
    case 1:
    case 7:
        if (PC1[fyle - 1] == 0)
            s += KHOPN;

        if (PC2[fyle - 1] == 0)
            s += KHOPNX;
        break;
    default:
        /* Impossible! */
        break;
    }

    short a2 = atk2[sq] & 0x4FFF;

    if (a2 > 0)
    {
        short a1 = atk1[sq] & 0x4FFF;

        if (a1 == 0 || a2 > CTLK + 1)
        {
            s += HUNGP;
            ++hung[c1];
        }
        else
        {
            s += ATAKD;
        }
    }
    return s;
}

/*
  Perform normal static evaluation of board position. A score is generated
  for each piece and these are summed to get a score for each side.
*/
void Sim::ScorePosition(short side, short *score, short *value)
{
    short pscore[2];
    UpdateWeights();
    short xside = otherside[side];
    pscore[white] = pscore[black] = 0;

    for (c1 = white; c1 <= black; c1++)
    {
        c2 = otherside[c1];
        atk1 = atak[c1];
        atk2 = atak[c2];
        PC1 = PawnCnt[c1];
        PC2 = PawnCnt[c2];

        for (short i = PieceCnt[c1]; i >= 0; i--)
        {
            short sq = PieceList[c1][i];
            short s = 0;

            switch (board[sq])
            {
            case PAWN:
                s = PawnValue(sq, side);
                break;
            case KNIGHT:
                s = KnightValue(sq, side);
                break;
            case BISHOP:
                s = BishopValue(sq, side);
                break;
            case ROOK:
                s = _rookValue(sq, side);
                break;
            case QUEEN:
                s = QueenValue(sq, side);
                break;
            case KING:
                s = KingValue(sq, side);
                break;
            }
            pscore[c1] += s;
            value[sq] = s;
        }
    }

    if (hung[side] > 1)
        pscore[side] += HUNGX;

    if (hung[xside] > 1)
        pscore[xside] += HUNGX;

    *score = mtl[side] - mtl[xside] + pscore[side] - pscore[xside] + 10;

    if (dither)
        *score += ::rand() % dither;

    if (*score > 0 && pmtl[side] == 0)
    {
        if (emtl[side] < VALUER)
            *score = 0;
        else if (*score < VALUER)
            *score /= 2;
    }

    if (*score < 0 && pmtl[xside] == 0)
    {
        if (emtl[xside] < VALUER)
            *score = 0;
        else if (-*score < VALUER)
            *score /= 2;
    }

    if (mtl[xside] == VALUEK && emtl[side] > VALUEB)
        *score += 200;

    if (mtl[side] == VALUEK && emtl[xside] > VALUEB)
        *score -= 200;
}


/*
  Compute an estimate of the score by adding the positional score from the
  previous ply to the material difference. If this score falls inside a
  window which is 180 points wider than the alpha-beta window (or within a
  50 point window during quiescence search) call ScorePosition() to
  determine a score, otherwise return the estimated score. If one side has
  only a king and the other either has no pawns or no pieces then the
  function ScoreLoneKing() is called.
*/
int Sim::evaluate(short side, short ply, short alpha, short beta,
          short INCscore, short *slk, short *InChk, short toSquare)
{
    short evflag;
    short xside = otherside[side];
    short s = -Pscore[ply - 1] + mtl[side] - mtl[xside] - INCscore;
    hung[white] = hung[black] = 0;

    *slk = ((mtl[white] == VALUEK && (pmtl[black] == 0 || emtl[black] == 0)) ||
            (mtl[black] == VALUEK && (pmtl[white] == 0 || emtl[white] == 0)));

    if (*slk)
    {
        evflag = false;
    }
    else
    {
        evflag = (ply == 1 || ply < Sdepth ||
            ((ply == Sdepth + 1 || ply == Sdepth + 2) &&
             (s > alpha - xwndw && s < beta + xwndw)) ||
             (ply > Sdepth + 2 && s >= alpha - 25 && s <= beta + 25));
    }

    if (evflag)
    {
        EvalNodes++;
        _ataks(side, atak[side]);

        if (anyatak(side, PieceList[xside][0]))
            return 10001 - ply;

        _ataks(xside, atak[xside]);
        *InChk = anyatak(xside, PieceList[side][0]);
        ScorePosition(side, &s, svalue);
    }
    else
    {
        if (SqAtakd(PieceList[xside][0], side))
            return 10001 - ply;

        *InChk = SqAtakd(PieceList[side][0], xside);

        if (*slk)
            ScoreLoneKing(side, &s);
    }

    Pscore[ply] = s - mtl[side] + mtl[xside];
    ChkFlag[ply - 1] = *InChk ? Pindex[toSquare] : 0;
    return s;
}

int Sim::_search(HWND hWnd, short side, short ply, short depth,
        short alpha, short beta, WORD *bstline, short *rpt)
{
    NodeCnt++;
    short xside = otherside[side];

    if ((ply <= Sdepth + 3) && rpthash[side][hashkey & 0xFF] > 0)
        repetition(rpt);
    else
        *rpt = 0;

    /* Detect repetitions a bit earlier. SMC. 12/89 */
    if (*rpt == 1 && ply > 1)
        return 0;

    short slk, InChk;
    short score = evaluate(side, ply, alpha, beta, INCscore, &slk, &InChk, TOsquare);

    if (score > 9000)
    {
        bstline[ply] = 0;
        return score;
    }

    /* This test has been modified in 3.1 from 1.55 code. I think the mods
     have introduced an error in the search which causes the programme
     to run away from checking moves - hence the failure to find mates
     and to play through mate situations.
     The fixes introduced solve the problem reported by:
     cambell@rnd.GBA.NYU.EDU
     in the mate example.
                             J.Birmingham.
    */

    if (depth > 0)
    {
        /* Allow opponent a chance to check again */
        if (InChk)
            depth = depth < 2 ? 2 : depth;
        else if (PawnThreat[ply - 1] || xrecapture(score, alpha, beta, ply))
            ++depth;
    }
    else
    {
        if (score >= alpha && (InChk || PawnThreat[ply - 1] || (hung[side] > 1 && ply == Sdepth+1)))
            ++depth;           /* this was depth=1 in original ?bug? */
        else if (score <= beta && _mateThreat(ply))
            ++depth;           /* this was also set to depth=1  ?bug? */
    }

    /* end of changed section      J.Birmingham.          */

#if TTBLSZ
    if (depth > 0 && flag.hash && ply > 1)
    {
        if (ProbeTTable(side, depth, &alpha, &beta, &score) == false)
        {
#ifdef HASHFILE
            if (hashfile && (depth > 5) && (GameCnt < 12))
                ProbeFTable (side, depth, &alpha, &beta, &score);
#endif
            bstline[ply] = PV;
        }

        bstline[ply + 1] = 0;

        if (beta == -20000)
            return score;

        if (alpha > beta)
            return alpha;
    }
#endif
    short d = Sdepth == 1 ? 7 : 11;

    if (ply > Sdepth + d || (depth < 1 && score > beta))
        return score;

    if (ply > 1)
    {
        depth > 0 ? MoveList(side, ply) : CaptureList(side, ply);
    }

    if (TrPnt[ply] == TrPnt[ply + 1])
        return (score);

    short cf = depth < 1 && ply > Sdepth + 1 && !ChkFlag[ply - 2] && !slk;
    short best = depth > 0 ? -12000 : score;

    if (best > alpha)
        alpha = best;

    short pnt, pbst;
    for (pnt = pbst = TrPnt[ply]; pnt < TrPnt[ply + 1] && best <= beta; pnt++)
    {
        /* Little code segment to allow cooperative multitasking */
        {
#if 0
            MSG msg;

            if (!flag.timeout && ::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if (!TranslateAccelerator(hWnd, haccel, &msg))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
#endif
        }
        /* End of segment */

        if (ply > 1)
            pick(pnt, TrPnt[ply + 1] - 1);

        Leaf *node = &Tree[pnt];
        WORD mv = (node->f << 8) | node->t;
        WORD nxtline[MAXDEPTH];
        nxtline[ply + 1] = 0;

        if (cf && score+node->score < alpha)
            break;

        if (ply == 1)
            updateSearchStatus(d, pnt, node->f, node->t, best, alpha, node->score);

        if (!(node->flags & EXACT))
        {
            short tempst, tempsf, tempc, tempb;
            MakeMove(side, node, &tempb, &tempc, &tempsf, &tempst, &INCscore);
            CptrFlag[ply] = node->flags & CAPTURE;
            PawnThreat[ply] = node->flags & PWNTHRT;
            Tscore[ply] = node->score;
            PV = node->reply;

            short rcnt;
            node->score = -_search(hWnd, xside, ply + 1, depth > 0 ? depth - 1 : 0, -beta, -alpha, nxtline, &rcnt);

            if (abs(node->score) > 9000)
                node->flags |= EXACT;
            else if (rcnt == 1)
                node->score /= 2;

            if (rcnt >= 2 || GameCnt - Game50 > 99 ||
                (node->score == 9999 - ply && !ChkFlag[ply]))
            {
                node->flags |= DRAW;
                node->flags |= EXACT;

                if (side == computer)
                    node->score = contempt;
                else
                    node->score = -contempt;
            }

            node->reply = nxtline[ply + 1];
            UnmakeMove(side, node, &tempb, &tempc, &tempsf, &tempst);
        }

        if (node->score > best && !flag.timeout)
        {
            if (depth > 0)
                if (node->score > alpha && !(node->flags & EXACT))
                    node->score += depth;

            best = node->score;
            pbst = pnt;

            if (best > alpha)
                alpha = best;

            short j;
            for (j = ply + 1; nxtline[j] > 0; j++)
                bstline[j] = nxtline[j];

            bstline[j] = 0;
            bstline[ply] = mv;

            if (ply == 1)
            {
                if (best > root->score)
                {
                    Leaf tmp = Tree[pnt];

                    for (j = pnt - 1; j >= 0; j--)
                        Tree[j + 1] = Tree[j];

                    Tree[0] = tmp;
                    pbst = 0;
                }

                if (Sdepth > 2)
                {
                    if (best > beta)
                        ShowResults(hStats, best, bstline, '\xab' /*'+'*/);
                    else if (best < alpha)
                        ShowResults(hStats, best, bstline, '\xbb' /* '-'*/);
                    else
                        ShowResults(hStats, best, bstline, '\xb1' /*'&'*/);
                }
            }
        }

        if (::NodeCnt > ::ETnodes)
            ElapsedTime(0, ::ExtraTime, ::ResponseTime, ::ft);

        if (flag.timeout)
            return -Tscore[ply - 1];
    }

    Leaf *node = &Tree[pbst];
    WORD mv = (node->f << 8) | node->t;
#if TTBLSZ
    if (flag.hash && ply <= Sdepth && *rpt == 0 && best == alpha)
    {
        PutInTTable(side, best, depth, alpha, beta, mv);
#ifdef HASHFILE
        if (hashfile && (depth > 5) && (GameCnt < 12))
            PutInFTable(side, best, depth, alpha, beta, node->f, node->t);
#endif
    }
#endif
    if (depth > 0)
    {
        short j = node->f << 6 | node->t;

        if (side == black)
            j |= 0x1000;

        if (*(history + j) < 150)
            *(history + j) += BYTE(2) * depth;

        if (node->t != (GameList[GameCnt].gmove & 0xFF))
        {
            if (best <= beta)
            {
                killr3[ply] = mv;
            }
            else if (mv != killr1[ply])
            {
                killr2[ply] = killr1[ply];
                killr1[ply] = mv;
            }
        }

        _killr0[ply] = best > 9000 ? mv : 0;
    }
    return best;
}

//moet weg uit sim.cpp
void Sim::OutputMove(HINSTANCE hInstance, HWND hwnd, HWND compClr, Leaf *node)
{
    TCHAR tmp[30];
    UpdateDisplay(hwnd, compClr, node->f, node->t, 0, short(node->flags), flag.reverse);
    ::wsprintf(tmp, TEXT("My move is %s"), mvstr[0]);
    ::SetWindowText(compClr, tmp);
    Toolbox t;
    (void)t;

    if (node->flags & DRAW)
        t.messageBox(hInstance, hwnd, IDS_DRAWGAME, IDS_CHESS);
    else if (node->score == -9999)
        t.messageBox(hInstance, hwnd, IDS_YOUWIN, IDS_CHESS);
    else if (node->score == 9998)
        t.messageBox(hInstance, hwnd, IDS_COMPUTERWIN, IDS_CHESS);
    else if (node->score < -9000)
        t.messageBox(hInstance, hwnd, IDS_MATESOON, IDS_CHESS);
    else if (node->score > 9000)
        t.messageBox(hInstance, hwnd, IDS_COMPMATE, IDS_CHESS);

    if (flag.post)
        ShowNodeCnt(hStats, NodeCnt, evrate);
}

void Sim::BlendBoard(const short a[64], const short b[64], short c[64])
{
    for (int sq = 0; sq < 64; sq++)
        c[sq] = (a[sq] * (10 - stage) + b[sq] * stage) / 10;
}

void Sim::CopyBoard(const short a[64], short b[64])
{
    for (int sq = 0; sq < 64; sq++)
        b[sq] = a[sq];
}

bool Sim::patak(short c, short u)
{
    return atak[c][u] > CTLP;
}

/*
  This is done one time before the search is started. Set up arrays
  Mwpawn, Mbpawn, Mknight, Mbishop, Mking which are used in the
  SqValue() function to determine the positional value of each piece.
*/
void Sim::ExaminePosition()
{
    short wpadv, bpadv, wstrong, bstrong, z, pp, j, k, val, Pd, fyle, rank;
    static short PawnStorm = false;

    _ataks(white, atak[white]);
    _ataks(black, atak[black]);
    UpdateWeights();
    HasKnight[white] = HasKnight[black] = 0;
    HasBishop[white] = HasBishop[black] = 0;
    HasRook[white] = HasRook[black] = 0;
    HasQueen[white] = HasQueen[black] = 0;

    for (short side = white; side <= black; ++side)
    {
        for (short i = PieceCnt[side]; i >= 0; --i)
        {
            switch (board[PieceList[side][i]])
            {
            case KNIGHT:
                ++HasKnight[side];
                break;
            case BISHOP:
                ++HasBishop[side];
                break;
            case ROOK:
                ++HasRook[side];
                break;
            case QUEEN:
                ++HasQueen[side];
                break;
            }
        }
    }

    if (!Developed[white])
    {
        Developed[white] = board[1] != KNIGHT && board[2] != BISHOP &&
                            board[5] != BISHOP && board[6] != KNIGHT;
    }

    if (!Developed[black])
    {
        Developed[black] = board[57] != KNIGHT && board[58] != BISHOP &&
                            board[61] != BISHOP && board[62] != KNIGHT;
    }

    if (!PawnStorm && stage < 5)
    {
        PawnStorm = (column(_wking()) < 3 && column(_bking()) > 4) ||
                     (column(_wking()) > 4 && column(_bking()) < 3);
    }

    CopyBoard(pknight, Mknight[white]);
    CopyBoard(pknight, Mknight[black]);
    CopyBoard(pbishop, Mbishop[white]);
    CopyBoard(pbishop, Mbishop[black]);
    BlendBoard(KingOpening, KingEnding, Mking[white]);
    BlendBoard(KingOpening, KingEnding, Mking[black]);

    for (short sq = 0; sq < 64; ++sq)
    {
        fyle = column(sq);
        rank = row(sq);
        wstrong = bstrong = true;

        for (short i = sq; i < 64; i += 8)
        {
            if (patak(black, i))
            {
                wstrong = false;
                break;
            }
        }

        for (short i = sq; i >= 0; i -= 8)
        {
            if (patak(white, i))
            {
                bstrong = false;
                break;
            }
        }

        wpadv = bpadv = PADVNCM;

        if ((fyle == 0 || PawnCnt[white][fyle - 1] == 0) &&
            (fyle == 7 || PawnCnt[white][fyle + 1] == 0))
        {
            wpadv = PADVNCI;
        }

        if ((fyle == 0 || PawnCnt[black][fyle - 1] == 0) &&
            (fyle == 7 || PawnCnt[black][fyle + 1] == 0))
        {
            bpadv = PADVNCI;
        }

        Mwpawn[sq] = (wpadv * PawnAdvance[sq]) / 10;
        Mbpawn[sq] = (bpadv * PawnAdvance[63 - sq]) / 10;
        Mwpawn[sq] += PawnBonus;
        Mbpawn[sq] += PawnBonus;

        if (Mvboard[kingP[white]])
        {
            if ((fyle < 3 || fyle > 4) && xdistance(sq, _wking()) < 3)
                Mwpawn[sq] += PAWNSHIELD;
        }
        else if (rank < 3 && (fyle < 2 || fyle > 5))
        {
            Mwpawn[sq] += PAWNSHIELD / 2;
        }

        if (Mvboard[kingP[black]])
        {
            if ((fyle < 3 || fyle > 4) && xdistance(sq, _bking()) < 3)
                Mbpawn[sq] += PAWNSHIELD;
        }
        else if (rank > 4 && (fyle < 2 || fyle > 5))
        {
            Mbpawn[sq] += PAWNSHIELD / 2;
        }

        if (PawnStorm)
        {
            if ((column(_wking()) < 4 && fyle > 4) || (column(_wking()) > 3 && fyle < 3))
                Mwpawn[sq] += 3 * rank - 21;

            if ((column(_bking()) < 4 && fyle > 4) || (column(_bking()) > 3 && fyle < 3))
                Mbpawn[sq] -= 3 * rank;
        }

        Mknight[white][sq] += 5 - xdistance(sq, _bking());
        Mknight[white][sq] += 5 - xdistance(sq, _wking());
        Mknight[black][sq] += 5 - xdistance(sq, _wking());
        Mknight[black][sq] += 5 - xdistance(sq, _bking());
        Mbishop[white][sq] += BishopBonus;
        Mbishop[black][sq] += BishopBonus;

        for (short i = PieceCnt[black]; i >= 0; --i)
        {
            int xxxtmp = PieceList[black][i];

            if (xdistance(sq, xxxtmp) < 3)
                Mknight[white][sq] += KNIGHTPOST;
        }

        for (short i = PieceCnt[white]; i >= 0; --i)
        {
            int xxxtmp = PieceList[white][i];

            if (xdistance(sq, xxxtmp) < 3)
                Mknight[black][sq] += KNIGHTPOST;
        }

        if (wstrong)
            Mknight[white][sq] += KNIGHTSTRONG;

        if (bstrong)
            Mknight[black][sq] += KNIGHTSTRONG;

        if (wstrong)
            Mbishop[white][sq] += BISHOPSTRONG;

        if (bstrong)
            Mbishop[black][sq] += BISHOPSTRONG;

        if (HasBishop[white] == 2)
            Mbishop[white][sq] += 8;

        if (HasBishop[black] == 2)
            Mbishop[black][sq] += 8;

        if (HasKnight[white] == 2)
            Mknight[white][sq] += 5;

        if (HasKnight[black] == 2)
            Mknight[black][sq] += 5;

        Kfield[white][sq] = Kfield[black][sq] = 0;

        if (xdistance(sq, _wking()) == 1)
            Kfield[black][sq] = KATAK;

        if (xdistance(sq, _bking()) == 1)
            Kfield[white][sq] = KATAK;

        Pd = 0;
        for (k = 0; k <= PieceCnt[white]; ++k)
        {
            short i = PieceList[white][k];

            if (board[i] == PAWN)
            {
                pp = true;

                if (row(i) == 6)
                    z = i + 8;
                else
                    z = i + 16;

                for (j = i + 8; j < 64; j += 8)
                {
                    if (patak(black, j) || board[j] == PAWN)
                    {
                        pp = false;
                        break;
                    }
                }

                if (pp)
                    Pd += 5 * taxicab(sq, z);
                else
                    Pd += taxicab(sq, z);
            }
        }

        for (k = 0; k <= PieceCnt[black]; ++k)
        {
            short i = PieceList[black][k];

            if (board[i] == PAWN)
            {
                pp = true;
                z = row(i) == 1 ? i - 8 : i - 16;

                for (j = i - 8; j >= 0; j -= 8)
                {
                    if (patak(white, j) || board[j] == PAWN)
                    {
                        pp = false;
                        break;
                    }
                }

                if (pp)
                    Pd += 5 * taxicab(sq, z);
                else
                    Pd += taxicab(sq, z);
            }
        }

        if (Pd != 0)
        {
            val = (Pd * stage2) / 10;
            Mking[white][sq] -= val;
            Mking[black][sq] -= val;
        }
    }
}

/*
  Select a move by calling function search() at progressively deeper ply
  until time is up or a mate or draw is reached. An alpha-beta window of -90
  to +90 points is set around the score returned from the previous
  iteration. If Sdepth != 0 then the program has correctly predicted the
  opponents move and the search will start at a depth of Sdepth+1 rather
  than a depth of 1.
*/
void Sim::SelectMove(HINSTANCE hInstance, HWND hWnd, HWND compClr, short side,
                short iop, short maxSearchDepth, long xft)
{
    static short i, tempb, tempc, tempsf, tempst, rpt;
    static short alpha, beta, score;

    flag.timeout = false;
    short xside = otherside[side];

    if (iop != 2)
        player = side;

    if (TCflag)
    {
        if ((TimeControl.moves[side] + 3) != 0)
        {
            ResponseTime = TimeControl.clock[side] / (TimeControl.moves[side] + 3) - OperatorTime;
        }
        else
        {
            ResponseTime = 0;
        }

        ::ResponseTime += (ResponseTime * TimeControl.moves[side]) / (2 * TCmoves + 1);
    }
    else
    {
        ::ResponseTime = Level;
    }

    if (iop == 2)
        ::ResponseTime = 99999;

    if (::Sdepth > 0 && root->score > Zscore - zwndw)
        ::ResponseTime -= xft;
    else if (::ResponseTime < 1)
        ::ResponseTime = 1;

    ExtraTime = 0;
    ExaminePosition();
    ScorePosition(side, &score, svalue);
    ShowSidetoMove();

    if (::Sdepth == 0)
    {
        SearchStartStuff(side);
        ::memset(history, 0, 8192 * sizeof(char));
        FROMsquare = TOsquare = -1;
        PV = 0;

        if (iop != 2)
            hint = 0;

        for (i = 0; i < MAXDEPTH; i++)
            PrVar[i] = _killr0[i] = killr1[i] = killr2[i] = killr3[i] = 0;

        alpha = score - 90;
        beta = score + 90;
        rpt = 0;
        TrPnt[1] = 0;
        root = &Tree[0];
        MoveList(side, 1);

        for (i = TrPnt[1]; i < TrPnt[2]; i++)
            pick(i, TrPnt[2] - 1);

        if (Book != NULL)
            OpeningBook(&hint);

        if (Book != NULL)
            flag.timeout = true;

        NodeCnt = ETnodes = EvalNodes = HashCnt = FHashCnt = HashCol = 0;
        Zscore = 0;
        zwndw = 20;
    }

    while (!flag.timeout && Sdepth < maxSearchDepth)
    {
        Sdepth++;
        ShowDepth(hStats, ' ');
        score = _search(hWnd, side, 1, Sdepth, alpha, beta, PrVar, &rpt);

        for (i = 1; i <= Sdepth; i++)
            _killr0[i] = PrVar[i];

        if (score < alpha)
        {
            ShowDepth(hStats, '\xbb' /*'-'*/);
            ExtraTime = 10 * ResponseTime;
            /* ZeroTTable (); */
            score = _search(hWnd, side, 1, Sdepth, -9000, score, PrVar, &rpt);
        }

        if (score > beta && !(root->flags & EXACT))
        {
            ShowDepth(hStats, '\xab' /*'+'*/);
            ExtraTime = 0;
            /* ZeroTTable (); */
            score = _search(hWnd, side, 1, Sdepth, score, 9000, PrVar, &rpt);
        }

        score = root->score;

        if (!flag.timeout)
            for (i = TrPnt[1] + 1; i < TrPnt[2]; i++)
                pick (i, TrPnt[2] - 1);

        ShowResults(hStats, score, PrVar, '\xb7' /*'.'*/);

        for (i = 1; i <= Sdepth; i++)
            _killr0[i] = PrVar[i];

        if (score > Zscore - zwndw && score > Tree[1].score + 250)
            ExtraTime = 0;
        else if (score > Zscore - 3 * zwndw)
            ExtraTime = ResponseTime;
        else
            ExtraTime = 3 * ResponseTime;

        if (root->flags & EXACT)
            flag.timeout = true;

        if (Tree[1].score < -9000)
            flag.timeout = true;

        if (4 * et > 2 * ResponseTime + ExtraTime)
            flag.timeout = true;

        if (!flag.timeout)
        {
            Tscore[0] = score;
            Zscore = Zscore == 0 ? score : (Zscore + score) / 2;
        }

        zwndw = 20 + abs (Zscore / 12);
        beta = score + Bwindow;

        if (Zscore < score)
            alpha = Zscore - _awindow - zwndw;
        else
            alpha = score - _awindow - zwndw;
    }

    score = root->score;

    if (rpt >= 2 || score < -12000)
        root->flags |= DRAW;

    if (iop == 2)
        return;

    if (Book == NULL)
        hint = PrVar[2];

    ElapsedTime(1, ExtraTime, ResponseTime, ::ft);

    if (score > -9999 && rpt <= 2)
    {
        MakeMove(side, root, &tempb, &tempc, &tempsf, &tempst, &INCscore);
        algbr(root->f, root->t, (short) root->flags);
    }
    else
    {
        algbr(0, 0, 0);
    }

    OutputMove(hInstance, hWnd, compClr, root);

    if (score == -9999 || score == 9998)
        flag.mate = true;

    if (flag.mate)
        hint = 0;

    if (board[root->t] == PAWN || root->flags & CAPTURE || root->flags & CSTLMASK)
    {
        Game50 = GameCnt;
        ZeroRPT();
    }

    GameList[GameCnt].score = score;
    GameList[GameCnt].nodes = NodeCnt;
    GameList[GameCnt].time = (short) et;
    GameList[GameCnt].depth = Sdepth;

    if (TCflag)
    {
        TimeControl.clock[side] -= (et + OperatorTime);

        if (--TimeControl.moves[side] == 0)
            SetTimeControl(::ft);
    }

    if (root->flags & DRAW && flag.bothsides)
        flag.mate = true;

    if (GameCnt > 470)
        flag.mate = true; /* out of move store, you loose */

    player = xside;
    Sdepth = 0;
}

int parse(FILE *fd, WORD *mv, short side)
{
    int c;
    char s[100];
    while ((c = getc(fd)) == ' ');
    int i = 0;
    s[0] = char(c);

    while (c != ' ' && c != '\n' && c != EOF)
        s[++i] = (char)(c = getc(fd));

    s[++i] = '\0';

    if (c == EOF)
        return -1;

    if (s[0] == '!' || s[0] == ';' || i < 3)
    {
        while (c != '\n' && c != EOF)
            c = getc(fd);

        return 0;
    }

    if (s[4] == 'o')
    {
        *mv = side == black ? 0x3c3a : 0x0402;
    }
    else if (s[0] == 'o')
    {
        *mv = side == black ? 0x3c3e : 0x0406;
    }
    else
    {
        int c1 = s[0] - 'a';
        int r1 = s[1] - '1';
        int c2 = s[2] - 'a';
        int r2 = s[3] - '1';
        *mv = locn(r1, c1) << 8 | locn(r2, c2);
    }

    return 1;
}

/*
  hashbd contains a 32 bit "signature" of the board position. hashkey
  contains a 16 bit code used to address the hash table. When a move is
  made, XOR'ing the hashcode of moved piece on the from and to squares with
  the hashbd and hashkey values keeps things current.
*/
#if TTBLSZ
void Sim::UpdateHashbd(short side, short piece, short f, short t)
{
    if (f >= 0)
    {
        ::hashbd ^= (hashcode + side * 7 * 64 + piece * 64 + f)->bd;
        ::hashkey ^= (hashcode + side * 7 * 64 + piece * 64 + f)->key;
    }

    if (t >= 0)
    {
        ::hashbd ^= (hashcode + side * 7 * 64 + piece * 64 + t)->bd;
        ::hashkey ^= (hashcode + side * 7 * 64 + piece * 64 + t)->key;
    }
}

BYTE Sim::CB(WORD i)
{
    return BYTE((color[2 * i] ? 0x80 : 0) | (board[2 * i] << 4) | (color[2 * i + 1] ? 0x8 : 0) | (board[2 * i + 1]));
}

void Sim::ZeroTTable()
{
    if (!flag.hash)
        return;

    for (int side = white; side <= black; side++)
        for (int i = 0; i < TTBLSZ; i++)
            (ttable + side * 2 + i)->depth = 0;
}

#define FILESZ (1 << 17)

/*
  Look for the current board position in the persistent transposition table.
*/
int Sim::ProbeFTable(short side, short depth, short *alpha, short *beta, short *score)
{
    WORD j;
    DWORD hashix;
    short s;
    fileentry xnew, t;

    if (side == white)
        hashix = hashkey & 0xFFFFFFFE & (FILESZ - 1);
    else
        hashix = hashkey | (1 & (FILESZ - 1));

    for (WORD i = 0; i < 32; i++)
        xnew.bd[i] = CB(i);

    xnew.flags = 0;

    //wat is qrook en krook?
#if 0
    if ((Mvboard[kingP[side]] == 0) && (Mvboard[qrook[side]] == 0))
        xnew.flags |= QUEENCASTLE;

    if ((Mvboard[kingP[side]] == 0) && (Mvboard[krook[side]] == 0))
        xnew.flags |= KINGCASTLE;
#endif

    for (WORD i = 0; i < FREHASH; i++)
    {
        fseek(hashfile, sizeof(fileentry) * ((hashix + 2 * i) & (FILESZ - 1)), SEEK_SET);
        fread(&t, sizeof(fileentry), 1, hashfile);

        for (j = 0; j < 32; j++)
            if (t.bd[j] != xnew.bd[j])
                break;

        if ((short(t.depth) >= depth) && (j >= 32) && (xnew.flags == (t.flags & (KINGCASTLE | QUEENCASTLE))))
        {
            FHashCnt++;
            PV = (t.f << 8) | t.t;
            s = (t.sh << 8) | t.sl;

            if (t.flags & TRUESCORE)
            {
                *score = s;
                *beta = -20000;
            }
            else if (t.flags & LOWERBOUND)
            {
                if (s > *alpha)
                    *alpha = s - 1;
            }

            return true;
        }
    }
    return false;
}

/*
  Store the current board position in the persistent transposition table.
*/
#ifdef HASHFILE
static void PutInFTable(short side, short score, short depth, short alpha,
    short beta, WORD f, WORD t)
{
    WORD i;
    DWORD hashix;
    struct fileentry xnew, tmp;

    if (side == white)
        hashix = hashkey & 0xFFFFFFFE & (filesz - 1);
    else
        hashix = hashkey | 1 & (filesz - 1);

    for (i = 0; i < 32; i++)
        xnew.bd[i] = CB(i);

    xnew.f = BYTE(f);
    xnew.t = BYTE(t);
    xnew.flags = 0;

    if (score < alpha)
        xnew.flags |= upperbound;
    else if (score > beta)
        xnew.flags |= LOWERBOUND;
    else
        xnew.flags |= TRUESCORE;

    //wat is qrook en krook?
#if 0
    if ((Mvboard[kingP[side]] == 0) && (Mvboard[qrook[side]] == 0))
        xnew.flags |= queencastle;

    if ((Mvboard[kingP[side]] == 0) && (Mvboard[krook[side]] == 0))
        xnew.flags |= KINGCASTLE;
#endif
    xnew.depth = BYTE(depth);
    xnew.sh = BYTE(score >> 8);
    xnew.sl = BYTE(score & 0xFF);

    for (i = 0; i < FREHASH; i++)
    {
        fseek(hashfile, sizeof(struct fileentry) * ((hashix + 2 * i) & (filesz - 1)), SEEK_SET);
        fread(&tmp, sizeof(struct fileentry), 1, hashfile);

        if ((short)tmp.depth <= depth)
        {
            fseek(hashfile, sizeof(struct fileentry) * ((hashix + 2 * i) & (filesz - 1)), SEEK_SET);
            fwrite(&xnew, sizeof(struct fileentry), 1, hashfile);
            break;
        }
    }
}
#endif
#endif

void Sim::ZeroRPT()
{
    for (int side = white; side <= black; side++)
        for (int i = 0; i < 256; i++)
            rpthash[side][i] = 0;
}

void Sim::xlink(Leaf *node, short from, short to, short flag, short s, short ply)
{
    node->f = from;
    node->t = to;
    node->reply = 0;
    node->flags = flag;
    node->score = s;
    ++TrPnt[ply + 1];
}

/*
  Add a move to the tree.  Assign a bonus to order the moves
  as follows:
  1. Principle variation
  2. Capture of last moved piece
  3. Other captures (major pieces first)
  4. Killer moves
  5. "history" killers
*/
void Sim::LinkMove(short ply, short f, short t, short flag, short xside)
{
    Leaf *node = &Tree[TrPnt[ply + 1]];
    WORD mv = (f << 8) | t;
    short s = 0;

    if (mv == Swag0)
        s = 2000;
    else if (mv == Swag1)
        s = 60;
    else if (mv == Swag2)
        s = 50;
    else if (mv == Swag3)
        s = 40;
    else if (mv == Swag4)
        s = 30;

    short z = (f << 6) | t;

    if (xside == white)
        z |= 0x1000;

    s += *(history + z);

    if (color[t] != NEUTRAL)
    {
        if (t == TOsquare)
            s += 500;

        s += value[board[t]] - board[f];
    }

    if (board[f] == PAWN)
    {
        if ((t >> 3) == 0 || (t >> 3) == 7)
        {
            flag |= PROMOTE;
            s += 800;
            xlink(node++, f, t, flag | QUEEN, s - 20000, ply);
            //Link(f, t, flag | QUEEN, s - 20000);
            s -= 200;
            xlink(node++, f, t, flag | KNIGHT, s - 20000, ply);
            //Link(f, t, flag | KNIGHT, s - 20000);
            s -= 50;
            xlink(node++, f, t, flag | ROOK, s - 20000, ply);
            //Link(f, t, flag | ROOK, s - 20000);
            flag |= BISHOP;
            s -= 50;
        }
        else if ((t >> 3) == 1 || (t >> 3) == 6)
        {
            flag |= PWNTHRT;
            s += 600;
        }
    }
    xlink(node++, f, t, flag, s - 20000, ply);
}

/*
  Generate moves for a piece. The moves are taken from the precalulated
  array nextpos/nextdir. If the board is free, next move is choosen from
  nextpos else from nextdir.
*/
void Sim::GenMoves(short ply, short sq, short side, short xside)
{
    short piece = board[sq];
    BYTE *ppos = nextpos + ptype[side][piece] * 64 * 64 + sq * 64;
    BYTE *pdir = nextdir + ptype[side][piece] * 64 * 64 + sq * 64;

    if (piece == PAWN)
    {
        short u = ppos[sq];     /* follow no captures thread */

        if (color[u] == NEUTRAL)
        {
            LinkMove(ply, sq, u, 0, xside);
            u = ppos[u];

            if (color[u] == NEUTRAL)
                LinkMove(ply, sq, u, 0, xside);
        }

        u = pdir[sq];     /* follow captures thread */

        if (color[u] == xside)
            LinkMove(ply, sq, u, CAPTURE, xside);
        else if (u == epsquare)
            LinkMove(ply, sq, u, CAPTURE | EPMASK, xside);

        u = pdir[u];

        if (color[u] == xside)
            LinkMove(ply, sq, u, CAPTURE, xside);
        else if (u == epsquare)
            LinkMove(ply, sq, u, CAPTURE | EPMASK, xside);

        return;
    }

    short u = ppos[sq];

    do
    {
        if (color[u] == NEUTRAL)
        {
            LinkMove(ply, sq, u, 0, xside);
            u = ppos[u];
        }
        else
        {
            if (color[u] == xside)
                LinkMove(ply, sq, u, CAPTURE, xside);

            u = pdir[u];
        }
    }
    while (u != sq);
}

//Make or Unmake a castling move.
int Sim::castle(short side, short kf, short kt, short iop)
{
    short rf, rt, t0;
    short xside = otherside[side];

    if (kt > kf)
    {
        rf = kf + 3;
        rt = kt - 1;
    }
    else
    {
        rf = kf - 4;
        rt = kt + 1;
    }

    if (iop == 0)
    {
        if (kf != kingP[side] || board[kf] != KING || board[rf] != ROOK ||
            Mvboard[kf] != 0 || Mvboard[rf] != 0 || color[kt] != NEUTRAL ||
            color[rt] != NEUTRAL || color[kt - 1] != NEUTRAL || SqAtakd(kf, xside) ||
            SqAtakd(kt, xside) || SqAtakd(rt, xside))
        {
            return (false);
        }
    }
    else
    {
        if (iop == 1)
        {
            castld[side] = true;
            Mvboard[kf]++;
            Mvboard[rf]++;
        }
        else
        {
            castld[side] = false;
            Mvboard[kf]--;
            Mvboard[rf]--;
            t0 = kt;
            kt = kf;
            kf = t0;
            t0 = rt;
            rt = rf;
            rf = t0;
        }

        board[kt] = KING;
        color[kt] = side;
        Pindex[kt] = 0;
        board[kf] = NO_PIECE;
        color[kf] = NEUTRAL;
        board[rt] = ROOK;
        color[rt] = side;
        Pindex[rt] = Pindex[rf];
        board[rf] = NO_PIECE;
        color[rf] = NEUTRAL;
        PieceList[side][Pindex[kt]] = kt;
        PieceList[side][Pindex[rt]] = rt;
#if TTBLSZ
        UpdateHashbd(side, KING, kf, kt);
        UpdateHashbd(side, ROOK, rf, rt);
#endif
    }
    return true;
}

/*
  Fill the array Tree[] with all available moves for side to play. Array
  TrPnt[ply] contains the index into Tree[] of the first move at a ply.
*/
void Sim::MoveList(short side, short ply)
{
    short xside = otherside[side];
    TrPnt[ply + 1] = TrPnt[ply];
    Swag0 = PV == 0 ? _killr0[ply] : PV;
    Swag1 = killr1[ply];
    Swag2 = killr2[ply];
    Swag3 = killr3[ply];
    Swag4 = ply > 2 ? killr1[ply - 2] : 0;

    for (short i = PieceCnt[side]; i >= 0; i--)
        GenMoves(ply, PieceList[side][i], side, xside);

    if (!castld[side])
    {
        short f = PieceList[side][0];

        if (castle(side, f, f + 2, 0))
            LinkMove(ply, f, f + 2, CSTLMASK, xside);

        if (castle(side, f, f - 2, 0))
            LinkMove(ply, f, f - 2, CSTLMASK, xside);
    }
}

/*
  Fill the array Tree[] with all available cature and promote moves for
  side to play. Array TrPnt[ply] contains the index into Tree[]
  of the first move at a ply.
*/
void Sim::CaptureList(short side, short ply)
{
    short u;
    BYTE *ppos;
    short xside = otherside[side];
    TrPnt[ply + 1] = TrPnt[ply];
    Leaf *node = &Tree[TrPnt[ply]];
    short r7 = rank7[side];
    short *PL = PieceList[side];

    for (short i = 0; i <= PieceCnt[side]; ++i)
    {
        short sq = PL[i];
        short piece = board[sq];

        if (sweep[piece])
        {
            ppos = nextpos + piece * 64 * 64 + sq * 64;
            BYTE *pdir = nextdir + piece * 64 * 64 + sq * 64;
            u = ppos[sq];

            do
            {
                if (color[u] == NEUTRAL)
                {
                    u = ppos[u];
                }
                else
                {
                    if (color[u] == xside)
                    {
                        short s = value[board[u]] + svalue[board[u]] - piece;
                        xlink(node, sq, u, CAPTURE, s, ply);
                        node++;
                    }

                    u = pdir[u];
                }
            }
            while (u != sq);

            continue;
        }

        BYTE *pdir = nextdir + ptype[side][piece] * 64 * 64 + sq * 64;

        if (piece == PAWN && sq >> 3 == r7)
        {
            u = pdir[sq];

            if (color[u] == xside)
                xlink(node++, sq, u, CAPTURE | PROMOTE | QUEEN, VALUEQ, ply);

            u = pdir[u];

            if (color[u] == xside)
                xlink(node++, sq, u, CAPTURE | PROMOTE | QUEEN, VALUEQ, ply);

            ppos = nextpos + ptype[side][piece] * 64 * 64 + sq * 64;
            u = ppos[sq]; /* also generate non capture promote */

            if (color[u] == NEUTRAL)
                xlink(node++, sq, u, PROMOTE | QUEEN, VALUEQ, ply);

            continue;
        }

        u = pdir[sq];

        do
        {
            if (color[u] == xside)
            {
                short s = value[board[u]] + svalue[board[u]] - piece;
                xlink(node, sq, u, CAPTURE, s, ply);
                node++;
            }

            u = pdir[u];
        }
        while (u != sq);
    }
}

//Make or unmake an en passant move.
void Sim::EnPassant(short xside, short f, short t, short iop)
{
    short l = t > f ? t - 8 : t + 8;

    if (iop == 1)
    {
        board[l] = NO_PIECE;
        color[l] = NEUTRAL;
    }
    else
    {
        board[l] = PAWN;
        color[l] = xside;
    }
    InitializeStats();
}

/*
  Update the PieceList and Pindex arrays when a piece is captured or when a
  capture is unmade.
*/
void Sim::_updatePieceList(short side, short sq, short iop)
{
    if (iop == 1)
    {
        PieceCnt[side]--;
        for (short i = Pindex[sq]; i <= PieceCnt[side]; i++)
        {
            PieceList[side][i] = PieceList[side][i + 1];
            Pindex[PieceList[side][i]] = i;
        }
    }
    else
    {
        PieceCnt[side]++;
        PieceList[side][PieceCnt[side]] = sq;
        Pindex[sq] = PieceCnt[side];
    }
}

/*
  Update Arrays board[], color[], and Pindex[] to reflect the new board
  position obtained after making the move pointed to by node. Also update
  miscellaneous stuff that changes when a move is made.
*/
void Sim::MakeMove(short side, Leaf *node, short *tempb,
          short *tempc, short *tempsf, short *tempst, short *INCscore)
{
    short ct, cf;
    short xside = otherside[side];
    GameCnt++;
    short f = node->f;
    short t = node->t;
    epsquare = -1;
    FROMsquare = f;
    TOsquare = t;
    *INCscore = 0;
    GameList[GameCnt].gmove = (f << 8) | t;

    if (node->flags & CSTLMASK)
    {
        GameList[GameCnt].piece = NO_PIECE;
        GameList[GameCnt].color = side;
        Sim::castle(side, f, t, 1);
    }
    else
    {
        if (!(node->flags & CAPTURE) && board[f] != PAWN)
            rpthash[side][hashkey & 0xFF]++;

        *tempc = color[t];
        *tempb = board[t];
        *tempsf = svalue[f];
        *tempst = svalue[t];
        GameList[GameCnt].piece = *tempb;
        GameList[GameCnt].color = *tempc;

        if (*tempc != NEUTRAL)
        {
            _updatePieceList (*tempc, t, 1);

            if (*tempb == PAWN)
                --PawnCnt[*tempc][column(t)];

            if (board[f] == PAWN)
            {
                --PawnCnt[side][column (f)];
                ++PawnCnt[side][column (t)];
                cf = column(f);
                ct = column(t);

                if (PawnCnt[side][ct] > 1 + PawnCnt[side][cf])
                    *INCscore -= 15;
                else if (PawnCnt[side][ct] < 1 + PawnCnt[side][cf])
                    *INCscore += 15;
                else if (ct == 0 || ct == 7 || PawnCnt[side][ct + ct - cf] == 0)
                    *INCscore -= 15;
            }

            mtl[xside] -= value[*tempb];

            if (*tempb == PAWN)
                pmtl[xside] -= VALUEP;
#if TTBLSZ
            UpdateHashbd(xside, *tempb, -1, t);
#endif
            *INCscore += *tempst;
            Mvboard[t]++;
        }
        color[t] = color[f];
        board[t] = board[f];
        svalue[t] = svalue[f];
        Pindex[t] = Pindex[f];
        PieceList[side][Pindex[t]] = t;
        color[f] = NEUTRAL;
        board[f] = NO_PIECE;

        if (board[t] == PAWN)
        {
            if (t - f == 16)
                epsquare = f + 8;
            else if (f - t == 16)
                epsquare = f - 8;
        }

        if (node->flags & PROMOTE)
        {
            board[t] = node->flags & PMASK;

            if (board[t] == QUEEN)
                HasQueen[side]++;
            else if (board[t] == ROOK)
                HasRook[side]++;
            else if (board[t] == BISHOP)
                HasBishop[side]++;
            else if (board[t] == KNIGHT)
                HasKnight[side]++;

            --PawnCnt[side][column (t)];
            mtl[side] += value[board[t]] - VALUEP;
            pmtl[side] -= VALUEP;
#if TTBLSZ
            UpdateHashbd(side, PAWN, f, -1);
            UpdateHashbd(side, board[t], f, -1);
#endif
            *INCscore -= *tempsf;
        }

        if (node->flags & EPMASK)
            EnPassant(xside, f, t, 1);
        else
#if TTBLSZ
            UpdateHashbd(side, board[t], f, t);
#else
        /* NOOP */;
#endif
        Mvboard[f]++;
    }
}

/*
  Take back a move.
*/
void Sim::UnmakeMove(short side, Leaf *node, short *tempb,
            short *tempc, short *tempsf, short *tempst)
{
    short xside = otherside[side];
    short f = node->f;
    short t = node->t;
    epsquare = -1;
    GameCnt--;

    if (node->flags & CSTLMASK)
    {
        Sim::castle(side, f, t, 2);
        return;
    }

    color[f] = color[t];
    board[f] = board[t];
    svalue[f] = *tempsf;
    Pindex[f] = Pindex[t];
    PieceList[side][Pindex[f]] = f;
    color[t] = *tempc;
    board[t] = *tempb;
    svalue[t] = *tempst;

    if (node->flags & PROMOTE)
    {
        board[f] = PAWN;
        ++PawnCnt[side][column (t)];
        mtl[side] += VALUEP - value[node->flags & PMASK];
        pmtl[side] += VALUEP;
#if TTBLSZ
        UpdateHashbd(side, short(node->flags) & PMASK, -1, t);
        UpdateHashbd(side, PAWN, -1, t);
#endif
    }

    if (*tempc != NEUTRAL)
    {
        _updatePieceList (*tempc, t, 2);

        if (*tempb == PAWN)
            ++PawnCnt[*tempc][column (t)];

        if (board[f] == PAWN)
        {
            --PawnCnt[side][column (t)];
            ++PawnCnt[side][column (f)];
        }

        mtl[xside] += value[*tempb];

        if (*tempb == PAWN)
            pmtl[xside] += VALUEP;
#if TTBLSZ
        UpdateHashbd (xside, *tempb, -1, t);
#endif
        Mvboard[t]--;
    }
    if (node->flags & EPMASK)
        EnPassant(xside, f, t, 2);
    else
#if TTBLSZ
    UpdateHashbd(side, board[f], f, t);
#else
      /* NOOP */;
#endif
    Mvboard[f]--;

    if (!(node->flags & CAPTURE) && (board[f] != PAWN))
        rpthash[side][hashkey & 0xFF]--;
}

/*
  Scan thru the board seeing what's on each square. If a piece is found,
  update the variables PieceCnt, PawnCnt, Pindex and PieceList. Also
  determine the material for each side and set the hashkey and hashbd
  variables to represent the current board position. Array
  PieceList[side][indx] contains the location of all the pieces of either
  side. Array Pindex[sq] contains the indx into PieceList for a given
  square.
*/
void Sim::InitializeStats()
{
    epsquare = -1;

    for (short i = 0; i < 8; i++)
        PawnCnt[white][i] = PawnCnt[black][i] = 0;

    mtl[white] = mtl[black] = pmtl[white] = pmtl[black] = 0;
    PieceCnt[white] = PieceCnt[black] = 0;
#if TTBLSZ
    hashbd = hashkey = 0;
#endif
    for (short sq = 0; sq < 64; sq++)
    {
        if (color[sq] != NEUTRAL)
        {
            mtl[color[sq]] += value[board[sq]];

            if (board[sq] == PAWN)
            {
                pmtl[color[sq]] += VALUEP;
                ++PawnCnt[color[sq]][column (sq)];
            }

            if (board[sq] == KING)
                Pindex[sq] = 0;
            else
                Pindex[sq] = ++PieceCnt[color[sq]];

            PieceList[color[sq]][Pindex[sq]] = sq;
#if TTBLSZ
            hashbd ^= (hashcode + color[sq] * 7 * 64 + board[sq] * 64 + sq)->bd;
            hashkey ^= (hashcode + color[sq] * 7 * 64 + board[sq] * 64 + sq)->key;
#endif
        }
    }
}

short const Sim::control[7] = {0, CTLP, CTLN, CTLB, CTLR, CTLQ, CTLK};

/*
  See if any piece with color 'side' ataks sq.  First check pawns then Queen,
  Bishop, Rook and King and last Knight.
*/
int Sim::SqAtakd(short sq, short side)
{
    BYTE *ppos;
    short xside = otherside[side];
    BYTE *pdir = nextdir + ptype[xside][PAWN] * 64 * 64 + sq * 64;
    short u = pdir[sq];         /* follow captures thread */

    if (u != sq)
    {
        if (board[u] == PAWN && color[u] == side)
            return true;

        u = pdir[u];

        if (u != sq && board[u] == PAWN && color[u] == side)
            return true;
    }

    /* king capture */
    if (*(distdata + sq * 64 + PieceList[side][0]) == 1)
        return (true);

    /* try a queen bishop capture */
    ppos = nextpos + BISHOP * 64 * 64 + sq * 64;
    pdir = nextdir + BISHOP * 64 * 64 + sq * 64;
    u = ppos[sq];

    do
    {
        if (color[u] == NEUTRAL)
        {
            u = ppos[u];
        }
        else
        {
            if (color[u] == side && (board[u] == QUEEN || board[u] == BISHOP))
                return true;

            u = pdir[u];
        }
    }
    while (u != sq);

    /* try a queen rook capture */
    ppos = nextpos + ROOK * 64 * 64 + sq * 64;
    pdir = nextdir + ROOK * 64 * 64 + sq * 64;
    u = ppos[sq];

    do
    {
        if (color[u] == NEUTRAL)
        {
            u = ppos[u];
        }
        else
        {
            if (color[u] == side && (board[u] == QUEEN || board[u] == ROOK))
                return true;

            u = pdir[u];
        }
    }
    while (u != sq);

    /* try a knight capture */
    pdir = nextdir + KNIGHT * 64*64+sq*64;
    u = pdir[sq];

    do
    {
        if (color[u] == side && board[u] == KNIGHT)
            return true;

        u = pdir[u];
    }
    while (u != sq);

    return false;
}

