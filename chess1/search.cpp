/*
  C source for GNU CHESS

  Revision: 1990-12-26

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

#include "protos.h"
#include "globals.h"
#include "resource.h"
#include "toolbox.h"

/*
  Perform an alpha-beta search to determine the score for the current board
  position. If depth <= 0 only capturing moves, pawn promotions and
  responses to check are generated and searched, otherwise all moves are
  processed. The search depth is modified for check evasions, certain
  re-captures and threats. Extensions may continue for up to 11 ply beyond
  the nominal search depth.
*/

static constexpr short VALUEP = 100;

static short rpthash[2][256], TOsquare;
static short rank7[3] = {6, 1, 0};
static short kingP[3] = {4, 60, 0};
static short value[7] = {0, VALUEP, VALUEN, VALUEB, VALUER, VALUEQ, VALUEK};
static short Zscore, FROMsquare, zwndw;
static long HashCol, HashCnt, FHashCnt;
static Leaf *root;
static WORD killr0[MAXDEPTH], killr1[MAXDEPTH], killr2[MAXDEPTH], killr3[MAXDEPTH];

static short ptype[2][8] = {
    {NO_PIECE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NO_PIECE},
    {NO_PIECE, BPAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NO_PIECE}};

static WORD PV, Swag0, Swag1, Swag2, Swag3, Swag4;

/* ............    MOVE GENERATION & SEARCH ROUTINES    .............. */



/*
  Find the best move in the tree between indexes p1 and p2. Swap the best
  move into the p1 element.
*/
void pick(short p1, short p2)
{
    Leaf temp;
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
        temp = Tree[p1];
        Tree[p1] = Tree[p0];
        Tree[p0] = temp;
    }
}

static constexpr BYTE TRUESCORE = 0x01, LOWERBOUND = 0x02,
    UPPERBOUND = 0x04, KINGCASTLE = 0x08, QUEENCASTLE = 0x10;

/*
  Look for the current board position in the transposition table.
*/
static int
ProbeTTable(short side, short depth, short *alpha, short *beta, short *score)
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
void PutInTTable(short side, short score, short depth, short alpha,
             short beta, WORD mv)
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

static void ShowDepth(HWND hwnd, char ch)
{
    TCHAR tmp[30];

    if (hwnd)
    {
        ::wsprintf(tmp, TEXT("%d%c"), Sdepth, ch);
        ::SetDlgItemText(hwnd, DEPTHTEXT, tmp);
    }
}

static void ShowResults(HWND hwnd, short score, PWORD bstline, char ch)
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

static void
updateSearchStatus(short &d, short pnt, short f,
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

static bool mateThreat(short ply)
{
    return ply < Sdepth + 4 && ply > 4 && ChkFlag[ply - 2] &&
            ChkFlag[ply - 4] && ChkFlag[ply - 2] != ChkFlag[ply - 4];
}

/*
  Check for draw by threefold repetition.
*/
static inline void repetition(short *cnt)
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

#define FILESZ (1 << 17)

bool xrecapture(short score, short alpha, short beta, short ply)
{
    return flag.rcptr && score > alpha && score < beta && ply > 2 && CptrFlag[ply - 1] && CptrFlag[ply - 2];
}

static constexpr WORD FREHASH = 6, EXACT = 0x0040, DRAW = 0x0400;

static constexpr short PWNTHRT = 0x0080;

static int search(HWND hWnd, short side, short ply, short depth,
        short alpha, short beta, WORD *bstline, short *rpt, HACCEL haccel)
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
        else if (score <= beta && mateThreat(ply))
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
            MSG msg;

            if (!flag.timeout && ::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if (!TranslateAccelerator(hWnd, haccel, &msg))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
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
            node->score = -search(hWnd, xside, ply + 1, depth > 0 ? depth - 1 : 0, -beta, -alpha, nxtline, &rcnt, haccel);

            if (abs (node->score) > 9000)
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
            UnmakeMove (side, node, &tempb, &tempc, &tempsf, &tempst);
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

        if (NodeCnt > ETnodes)
            ElapsedTime(0, ExtraTime, ResponseTime);

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

        killr0[ply] = best > 9000 ? mv : 0;
    }
    return best;
}

static void OutputMove(HINSTANCE hInstance, HWND hwnd, HWND compClr, Leaf *node)
{
    TCHAR tmp[30];
    UpdateDisplay(hwnd, compClr, node->f, node->t, 0, short(node->flags), flag.reverse);
    ::wsprintf(tmp, TEXT("My move is %s"), mvstr[0]);
    ::SetWindowText(compClr, tmp);
    Toolbox t;

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

/*
  Select a move by calling function search() at progressively deeper ply
  until time is up or a mate or draw is reached. An alpha-beta window of -90
  to +90 points is set around the score returned from the previous
  iteration. If Sdepth != 0 then the program has correctly predicted the
  opponents move and the search will start at a depth of Sdepth+1 rather
  than a depth of 1.
*/
void SelectMove(HINSTANCE hInstance, HWND hWnd, HWND compClr, short side,
                short iop, short maxSearchDepth, HACCEL haccel)
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

        ResponseTime += (ResponseTime * TimeControl.moves[side]) / (2 * TCmoves + 1);
    }
    else
    {
        ResponseTime = Level;
    }

    if (iop == 2)
        ResponseTime = 99999;

    if (Sdepth > 0 && root->score > Zscore - zwndw)
        ResponseTime -= ft;
    else if (ResponseTime < 1)
        ResponseTime = 1;

    ExtraTime = 0;
    ExaminePosition();
    ScorePosition(side, &score, svalue);
    ShowSidetoMove();

    if (Sdepth == 0)
    {
        SearchStartStuff(side);
        ::memset(history, 0, 8192 * sizeof(char));
        FROMsquare = TOsquare = -1;
        PV = 0;

        if (iop != 2)
            hint = 0;

        for (i = 0; i < MAXDEPTH; i++)
            PrVar[i] = killr0[i] = killr1[i] = killr2[i] = killr3[i] = 0;

        alpha = score - 90;
        beta = score + 90;
        rpt = 0;
        TrPnt[1] = 0;
        root = &Tree[0];
        MoveList(side, 1);

        for (i = TrPnt[1]; i < TrPnt[2]; i++)
            pick(i, TrPnt[2] - 1);

        if (Book != NULL)
            OpeningBook (&hint);

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
        score = search(hWnd, side, 1, Sdepth, alpha, beta, PrVar, &rpt, haccel);

        for (i = 1; i <= Sdepth; i++)
            killr0[i] = PrVar[i];

        if (score < alpha)
        {
            ShowDepth(hStats, '\xbb' /*'-'*/);
            ExtraTime = 10 * ResponseTime;
            /* ZeroTTable (); */
            score = search(hWnd, side, 1, Sdepth, -9000, score, PrVar, &rpt, haccel);
        }

        if (score > beta && !(root->flags & EXACT))
        {
            ShowDepth(hStats, '\xab' /*'+'*/);
            ExtraTime = 0;
            /* ZeroTTable (); */
            score = search(hWnd, side, 1, Sdepth, score, 9000, PrVar, &rpt, haccel);
        }

        score = root->score;

        if (!flag.timeout)
            for (i = TrPnt[1] + 1; i < TrPnt[2]; i++)
                pick (i, TrPnt[2] - 1);

        ShowResults(hStats, score, PrVar, '\xb7' /*'.'*/);

        for (i = 1; i <= Sdepth; i++)
            killr0[i] = PrVar[i];

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
            alpha = Zscore - Awindow - zwndw;
        else
            alpha = score - Awindow - zwndw;
    }

    score = root->score;

    if (rpt >= 2 || score < -12000)
        root->flags |= DRAW;

    if (iop == 2)
        return;

    if (Book == NULL)
        hint = PrVar[2];

    ElapsedTime(1, ExtraTime, ResponseTime);

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
            SetTimeControl();
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
void UpdateHashbd(short side, short piece, short f, short t)
{
    if (f >= 0)
    {
        ::hashbd ^= (::hashcode + side * 7 * 64 + piece * 64 + f)->bd;
        ::hashkey ^= (::hashcode + side * 7 * 64 + piece * 64 + f)->key;
    }

    if (t >= 0)
    {
        ::hashbd ^= (::hashcode + side * 7 * 64 + piece * 64 + t)->bd;
        ::hashkey ^= (::hashcode + side * 7 * 64 + piece * 64 + t)->key;
    }
}

static BYTE CB(WORD i)
{
    return BYTE((color[2 * i] ? 0x80 : 0) | (board[2 * i] << 4) | (color[2 * i + 1] ? 0x8 : 0) | (board[2 * i + 1]));
}

void ZeroTTable()
{
    if (!flag.hash)
        return;

    for (int side = white; side <= black; side++)
        for (int i = 0; i < TTBLSZ; i++)
            (ttable + side * 2 + i)->depth = 0;
}

/*
  Look for the current board position in the persistent transposition table.
*/
int ProbeFTable(short side, short depth, short *alpha, short *beta, short *score)
{
    WORD j;
    DWORD hashix;
    short s;
    struct fileentry xnew, t;

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

void ZeroRPT()
{
    for (int side = white; side <= black; side++)
        for (int i = 0; i < 256; i++)
            rpthash[side][i] = 0;
}

static void
xlink(Leaf *node, short from, short to, short flag, short s, short ply)
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
static inline void
LinkMove(short ply, short f, short t, short flag, short xside)
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

    s += *(history+z);

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
static inline void
GenMoves(short ply, short sq, short side, short xside)
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
    }
    else
    {
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
}

/*
  Fill the array Tree[] with all available moves for side to play. Array
  TrPnt[ply] contains the index into Tree[] of the first move at a ply.
*/
void MoveList(short side, short ply)
{
    short xside = otherside[side];
    TrPnt[ply + 1] = TrPnt[ply];
    Swag0 = PV == 0 ? killr0[ply] : PV;
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
void CaptureList(short side, short ply)
{
    short u;
    BYTE *ppos;
    short xside = otherside[side];
    TrPnt[ply + 1] = TrPnt[ply];
    Leaf *node = &Tree[TrPnt[ply]];
    short r7 = rank7[side];
    short *PL = PieceList[side];

    for (short i = 0; i <= PieceCnt[side]; i++)
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
                        xlink(node++, sq, u, CAPTURE,
                              value[board[u]] + svalue[board[u]] - piece,
                              ply);
                    }

                    u = pdir[u];
                }
            }
            while (u != sq);
        }
        else
        {
            BYTE *pdir = nextdir + ptype[side][piece] * 64 * 64 + sq * 64;

            if (piece == PAWN && sq >> 3 == r7)
            {
                u = pdir[sq];

                if (color[u] == xside)
                    xlink(node++, sq, u, CAPTURE | PROMOTE | QUEEN, VALUEQ, ply);

                u = pdir[u];

                if (color[u] == xside)
                    xlink(node++, sq, u, CAPTURE | PROMOTE | QUEEN, VALUEQ, ply);

                ppos = nextpos+ptype[side][piece] * 64 * 64 + sq * 64;
                u = ppos[sq]; /* also generate non capture promote */

                if (color[u] == NEUTRAL)
                    xlink(node++, sq, u, PROMOTE | QUEEN, VALUEQ, ply);
            }
            else
            {
                u = pdir[sq];

                do
                {
                    if (color[u] == xside)
                    {
                        xlink(node++, sq, u, CAPTURE,
                              value[board[u]] + svalue[board[u]] - piece,
                              ply);
                    }

                    u = pdir[u];
                }
                while (u != sq);
            }
        }
    }
}

//Make or Unmake a castling move.
int castle(short side, short kf, short kt, short iop)
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

//Make or unmake an en passant move.
static inline void EnPassant(short xside, short f, short t, short iop)
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
static inline void UpdatePieceList(short side, short sq, short iop)
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
void MakeMove(short side, Leaf *node, short *tempb,
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
        (void)castle(side, f, t, 1);
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
            UpdatePieceList (*tempc, t, 1);

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
void UnmakeMove(short side, Leaf *node, short *tempb,
            short *tempc, short *tempsf, short *tempst)
{
    short xside = otherside[side];
    short f = node->f;
    short t = node->t;
    epsquare = -1;
    GameCnt--;

    if (node->flags & CSTLMASK)
    {
        (void)castle(side, f, t, 2);
    }
    else
    {
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
            UpdatePieceList (*tempc, t, 2);

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
void InitializeStats()
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

/*
  See if any piece with color 'side' ataks sq.  First check pawns then Queen,
  Bishop, Rook and King and last Knight.
*/
int SqAtakd(short sq, short side)
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

