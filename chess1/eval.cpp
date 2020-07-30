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
#include "protos.h"
#include "gnuchess.h"

#define taxicab(a,b) *(taxidata+a*64+b)

#define wking PieceList[white][0]
#define bking PieceList[black][0]
#define EnemyKing PieceList[c2][0]

static short KingOpening[64] = {
      0,   0,  -4, -10, -10,  -4,   0,   0,
     -4,  -4,  -8, -12, -12,  -8,  -4,  -4,
    -12, -16, -20, -20, -20, -20, -16, -12,
    -16, -20, -24, -24, -24, -24, -20, -16,
    -16, -20, -24, -24, -24, -24, -20, -16,
    -12, -16, -20, -20, -20, -20, -16, -12,
     -4,  -4,  -8, -12, -12,  -8,  -4,  -4,
      0,   0,  -4, -10, -10,  -4,   0,   0};

static short KingEnding[64] = {
     0, 6, 12, 18, 18, 12, 6, 0,
     6, 12, 18, 24, 24, 18, 12, 6,
    12, 18, 24, 30, 30, 24, 18, 12,
    18, 24, 30, 36, 36, 30, 24, 18,
    18, 24, 30, 36, 36, 30, 24, 18,
    12, 18, 24, 30, 30, 24, 18, 12,
     6, 12, 18, 24, 24, 18, 12, 6,
     0, 6, 12, 18, 18, 12, 6, 0};

static short DyingKing[64] = {
     0,  8, 16, 24, 24, 16,  8,  0,
     8, 32, 40, 48, 48, 40, 32,  8,
    16, 40, 56, 64, 64, 56, 40, 16,
    24, 48, 64, 72, 72, 64, 48, 24,
    24, 48, 64, 72, 72, 64, 48, 24,
    16, 40, 56, 64, 64, 56, 40, 16,
     8, 32, 40, 48, 48, 40, 32,  8,
     0,  8, 16, 24, 24, 16,  8,  0};

static short KBNK[64] = {
    99, 90, 80, 70, 60, 50, 40, 40,
    90, 80, 60, 50, 40, 30, 20, 40,
    80, 60, 40, 30, 20, 10, 30, 50,
    70, 50, 30, 10,  0, 20, 40, 60,
    60, 40, 20,  0, 10, 30, 50, 70,
    50, 30, 10, 20, 30, 40, 60, 80,
    40, 20, 30, 40, 50, 60, 80, 90,
    40, 40, 50, 60, 70, 80, 90, 99};

static short pknight[64] =
{0, 4, 8, 10, 10, 8, 4, 0,
 4, 8, 16, 20, 20, 16, 8, 4,
 8, 16, 24, 28, 28, 24, 16, 8,
 10, 20, 28, 32, 32, 28, 20, 10,
 10, 20, 28, 32, 32, 28, 20, 10,
 8, 16, 24, 28, 28, 24, 16, 8,
 4, 8, 16, 20, 20, 16, 8, 4,
 0, 4, 8, 10, 10, 8, 4, 0};

static short pbishop[64] = {
    14, 14, 14, 14, 14, 14, 14, 14,
    14, 22, 18, 18, 18, 18, 22, 14,
    14, 18, 22, 22, 22, 22, 18, 14,
    14, 18, 22, 22, 22, 22, 18, 14,
    14, 18, 22, 22, 22, 22, 18, 14,
    14, 18, 22, 22, 22, 22, 18, 14,
    14, 22, 18, 18, 18, 18, 22, 14,
    14, 14, 14, 14, 14, 14, 14, 14};

static short PawnAdvance[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     4,  4,  4,  0,  0,  4,  4,  4,
     6,  8,  2, 10, 10,  2,  8,  6,
     6,  8, 12, 16, 16, 12,  8,  6,
     8, 12, 16, 24, 24, 16, 12,  8,
    12, 16, 24, 32, 32, 24, 16, 12,
    12, 16, 24, 32, 32, 24, 16, 12,
     0,  0,  0,  0,  0,  0,  0,  0};

static short control[7] = {0, CTLP, CTLN, CTLB, ctlR, ctlQ, ctlK};
static short PassedPawn0[8] = {0, 60, 80, 120, 200, 360, 600, 800};
static short PassedPawn1[8] = {0, 30, 40, 60, 100, 180, 300, 800};
static short PassedPawn2[8] = {0, 15, 25, 35, 50, 90, 140, 800};
static short PassedPawn3[8] = {0, 5, 10, 15, 20, 30, 140, 800};
static short ISOLANI[8] = {-12, -16, -20, -24, -24, -20, -16, -12};

static short BACKWARD[16] = {
     -6, -10, -15, -21, -28, -28, -28, -28,
    -28, -28, -28, -28, -28, -28, -28, -28};

static short BMBLTY[14] = {
    -2, 0, 2, 4, 6, 8, 10, 12, 13, 14, 15, 16, 16, 16};

static short RMBLTY[15] = {
    0, 2, 4, 6, 8, 10, 11, 12, 13, 14, 14, 14, 14, 14, 14};

static short KTHRT[36] = {
      0,  -8, -20, -36, -52, -68, -80, -80, -80, -80, -80, -80,
    -80, -80, -80, -80, -80, -80, -80, -80, -80, -80, -80, -80,
    -80, -80, -80, -80, -80, -80, -80, -80, -80, -80, -80, -80
};

/*
  ptype is used to separate white and black pawns, like this;
  ptyp = ptype[side][piece]
  piece can be used directly in nextpos/nextdir when generating moves
  for pieces that are not black pawns.
*/
static short ptype[2][8] = {
    {no_piece, pawn, knight, bishop, rook, queen, king, no_piece},
    {no_piece, bpawn, knight, bishop, rook, queen, king, no_piece}};

#if 0
static short qrook[3] = {0, 56, 0};
static short krook[3] = {7, 63, 0};
#endif


/* ............    POSITIONAL EVALUATION ROUTINES    ............ */

static short RHOPN, RHOPNX, KHOPN, KHOPNX, KSFTY;
static short ATAKD, HUNGP, HUNGX, KCASTLD, KMOVD, XRAY, PINVAL;
static short PEDRNK2B, PWEAKH, PADVNCM, PADVNCI, PAWNSHIELD, PDOUBLED, PBLOK;
static short KNIGHTPOST, KNIGHTSTRONG, BISHOPSTRONG, KATAK;
static short Mwpawn[64], Mbpawn[64], Mknight[2][64], Mbishop[2][64];
static short Mking[2][64], Kfield[2][64];
static short kingP[3] = {4, 60, 0};
static short rank7[3] = {6, 1, 0};
static short PawnBonus, BishopBonus, RookBonus;
static short *PC1, *PC2, *atk1, *atk2, atak[2][64];

#define Patak(c, u) (atak[c][u] > CTLP)
#define Anyatak(c, u) (atak[c][u] > 0)

/*
  Compute an estimate of the score by adding the positional score from the
  previous ply to the material difference. If this score falls inside a
  window which is 180 points wider than the alpha-beta window (or within a
  50 point window during quiescence search) call ScorePosition() to
  determine a score, otherwise return the estimated score. If one side has
  only a king and the other either has no pawns or no pieces then the
  function ScoreLoneKing() is called.
*/
int evaluate(short side, short ply, short alpha, short beta,
          short INCscore, short *slk, short *InChk)
{
    short evflag;
    short xside = otherside[side];
    short s = -Pscore[ply - 1] + mtl[side] - mtl[xside] - INCscore;
    hung[white] = hung[black] = 0;

    *slk = ((mtl[white] == valueK && (pmtl[black] == 0 || emtl[black] == 0)) ||
            (mtl[black] == valueK && (pmtl[white] == 0 || emtl[white] == 0)));

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
        ataks(side, atak[side]);

        if (Anyatak(side, PieceList[xside][0]))
            return 10001 - ply;

        ataks(xside, atak[xside]);
        *InChk = Anyatak(xside, PieceList[side][0]);
        ScorePosition(side, &s);
    }
    else
    {
        if (SqAtakd(PieceList[xside][0], side))
            return 10001 - ply;

        *InChk = SqAtakd (PieceList[side][0], xside);

        if (*slk)
            ScoreLoneKing(side, &s);
    }

    Pscore[ply] = s - mtl[side] + mtl[xside];

    if (*InChk)
        ChkFlag[ply - 1] = Pindex[TOsquare];
    else
        ChkFlag[ply - 1] = 0;

    return s;
}

/*
  Score King and Pawns versus King endings.
*/
static inline int
ScoreKPK(short side, short winner, short loser,
         short king1, short king2, short sq)
{
    short s, r;

    if (PieceCnt[winner] == 1)
        s = 50;
    else
        s = 120;

    if (winner == white)
    {
        if (side == loser)
            r = row(sq) - 1;
        else
            r = row(sq);

        if (row(king2) >= r && distance(sq, king2) < 8 - r)
            s += 10 * row (sq);
        else
            s = 500 + 50 * row (sq);

        if (row(sq) < 6)
            sq += 16;
        else if (row(sq) == 6)
            sq += 8;
    }
    else
    {
        if (side == loser)
            r = row (sq) + 1;
        else
            r = row (sq);

        if (row (king2) <= r && distance (sq, king2) < r + 1)
            s += 10 * (7 - row (sq));
        else
            s = 500 + 50 * (7 - row (sq));

        if (row (sq) > 1)
            sq -= 16;
        else if (row(sq) == 1)
            sq -= 8;
    }

    s += 8 * (taxicab (king2, sq) - taxicab (king1, sq));
    return s;
}

/*
  Score King+Bishop+Knight versus King endings.
  This doesn't work all that well but it's better than nothing.
*/
static inline int ScoreKBNK(short winner, short king1, short king2)
{
    short KBNKsq = 0;

    for (short sq = 0; sq < 64; sq++)
    {
        if (board[sq] == bishop)
        {
            if (row (sq) % 2 == column (sq) % 2)
                KBNKsq = 0;
            else
                KBNKsq = 7;
        }
    }

    short s = emtl[winner] - 300;

    if (KBNKsq == 0)
        s += KBNK[king2];
    else
        s += KBNK[locn (row (king2), 7 - column (king2))];

    s -= taxicab(king1, king2);
    s -= distance(PieceList[winner][1], king2);
    s -= distance(PieceList[winner][2], king2);
    return s;
}

/*
  Static evaluation when loser has only a king and winner has no pawns or no
  pieces.
*/
void ScoreLoneKing(short int side, short int *score)
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
    else if (emtl[winner] == valueB + valueN)
    {
        s = ScoreKBNK(winner, king1, king2);
    }
    else if (emtl[winner] > valueB)
    {
        s = 500 + emtl[winner] - DyingKing[king2] - 2 * distance(king1, king2);
    }

    *score = side == winner ? s : -s;
}

/*
  Find Bishop and Rook mobility, XRAY attacks, and pins. Increment the
  hung[] array if a pin is found.
*/
static inline void BRscan(short sq, short *s, short *mob)
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

        if (color[u] == neutral)
        {
            (*mob)++;

            if (ppos[u] == pdir[u])
                pin = -1;           /* oops new direction */

            u = ppos[u];
        }
        else if (pin < 0)
        {
            if (board[u] == pawn || board[u] == king)
            {
                u = pdir[u];
            }
            else
            {
                if (ppos[u] != pdir[u])
                    pin = u;        /* not on the edge and on to find a pin */

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

            pin = -1;             /* new direction */
            u = pdir[u];
        }
    }
    while (u != sq);
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
void ScoreThreat2(short u, short &cnt, short *s)
{
    if (::color[u] == ::c2)
        return;

    if (::atk1[u] == 0 || (::atk2[u] & 0xff) > 1)
        ++cnt;
    else
        *s -= 3;
}

static inline void KingScan(short sq, short *s)
{
    BYTE *ppos, *pdir;
    short cnt = 0;

    if (HasBishop[c2] || HasQueen[c2])
    {
        ppos = nextpos + bishop * 64 * 64 + sq * 64;
        pdir = nextdir + bishop * 64 * 64 + sq * 64;
        short u = ppos[sq];

        do
        {
            if (atk2[u] & ctlBQ)
                ScoreThreat2(u, cnt, s);

            u = color[u] == neutral ? ppos[u] : pdir[u];
        }
        while (u != sq);
    }

    if (HasRook[c2] || HasQueen[c2])
    {
        ppos = nextpos + rook * 64 * 64 + sq * 64;
        pdir = nextdir + rook * 64 * 64 + sq * 64;
        short u = ppos[sq];

        do
        {
            if (atk2[u] & ctlRQ)
                ScoreThreat2(u, cnt, s);

            u = (color[u] == neutral) ? ppos[u] : pdir[u];
        }
        while (u != sq);
    }

    if (HasKnight[c2])
    {
        pdir = nextdir + knight * 64 * 64 + sq * 64;
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
    pdir = nextpos + king * 64 * 64 + sq * 64;
    short u = pdir[sq];

    do
    {
        if (board[u] == pawn)
            ok = true;

        if (atk2[u] > atk1[u])
        {
            ++cnt;
            if (atk2[u] & ctlQ)
                if (atk2[u] > ctlQ + 1 && atk1[u] < ctlQ)
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

/*
  See if the attacked piece has unattacked squares to move to.
  The following must be true:
  c1 == color[sq]
  c2 == otherside[c1]
*/
static inline int trapped(short sq)
{
    short piece = board[sq];
    BYTE *ppos = nextpos + (ptype[c1][piece] * 64 * 64) + sq * 64;
    BYTE *pdir = nextdir + (ptype[c1][piece] * 64 * 64) + sq * 64;

    if (piece == pawn)
    {
        short u = ppos[sq];     /* follow no captures thread */

        if (color[u] == neutral)
        {
            if (atk1[u] >= atk2[u])
                return false;

            if (atk2[u] < CTLP)
            {
                u = ppos[u];

                if (color[u] == neutral && atk1[u] >= atk2[u])
                    return false;
            }
        }
        u = pdir[sq];     /* follow captures thread */

        if (color[u] == c2)
            return false;

        u = pdir[u];

        if (color[u] == c2)
            return false;
    }
    else
    {
        short u = ppos[sq];

        do
        {
            if (color[u] != c1)
                if (atk2[u] == 0 || board[u] >= piece)
                    return false;

            u = color[u] == neutral ? ppos[u] : pdir[u];
        }
        while (u != sq);
    }
    return true;
}

/*
  Calculate the positional value for a pawn on 'sq'.
*/
static inline int PawnValue(short sq, short side)
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

        if ((sq == 11 && color[19] != neutral) || (sq == 12 && color[20] != neutral))
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

            if (color[sq + 8] != neutral)
                s += PBLOK;
        }

        if (PC2[fyle] == 0)
        {
            r = side == black ? rank - 1 : rank;
            in_square = row(bking) >= r && distance(sq, bking) < 8 - r;
            e = a2 == 0 || side == white ? 0 : 1;

            for (j = sq + 8; j < 64; j += 8)
            {
                if (atk2[j] >= CTLP)
                {
                    e = 2;
                    break;
                }
                else if (atk2[j] > 0 || color[j] != neutral)
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

        if ((sq == 51 && color[43] != neutral) || (sq == 52 && color[44] != neutral))
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

            if (color[sq - 8] != neutral)
                s += PBLOK;
        }

        if (PC2[fyle] == 0)
        {
          if (side == white)
            r = rank + 1;
          else
            r = rank;
          in_square = (row (wking) <= r && distance (sq, wking) < r + 1);
          if (a2 == 0 || side == black)
            e = 0;
          else
            e = 1;
          for (j = sq - 8; j >= 0; j -= 8)
            if (atk2[j] >= CTLP)
              {
                e = 2;
                break;
              }
            else if (atk2[j] > 0 || color[j] != neutral)
              e = 1;
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
          if (trapped (sq))
            ++hung[c1];
        }
      else
        if (a2 > a1)
          s += ATAKD;
    }
    return s;
}

/*
  Calculate the positional value for a knight on 'sq'.
*/
static inline int KnightValue(short sq, short)
{
    short s = Mknight[c1][sq];
    short a2 = atk2[sq] & 0x4FFF;

    if (a2 > 0)
    {
        short a1 = atk1[sq] & 0x4FFF;

        if (a1 == 0 || a2 > ctlBN + 1)
        {
            s += HUNGP;
            ++hung[c1];

            if (trapped(sq))
                ++hung[c1];
        }
        else if (a2 >= ctlBN || a1 < CTLP)
        {
            s += ATAKD;
        }
    }
    return s;
}

/*
  Calculate the positional value for a bishop on 'sq'.
*/
static inline int BishopValue(short sq, short)
{
  short a2, a1;
  short s, mob;

  s = Mbishop[c1][sq];
  BRscan (sq, &s, &mob);
  s += BMBLTY[mob];
  a2 = (atk2[sq] & 0x4FFF);
  if (a2 > 0)
    {
      a1 = (atk1[sq] & 0x4FFF);
      if (a1 == 0 || a2 > ctlBN + 1)
        {
          s += HUNGP;
          ++hung[c1];
          if (trapped (sq))
            ++hung[c1];
        }
      else
        if (a2 >= ctlBN || a1 < CTLP)
          s += ATAKD;
    }
  return (s);
}

/*
  Calculate the positional value for a rook on 'sq'.
*/
static inline int RookValue(short sq, short)
{
    short a2, a1;
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
        s += 14 - taxicab(sq, EnemyKing);

    a2 = (atk2[sq] & 0x4FFF);

    if (a2 > 0)
    {
      a1 = (atk1[sq] & 0x4FFF);
      if (a1 == 0 || a2 > ctlR + 1)
        {
          s += HUNGP;
          ++hung[c1];

          if (trapped (sq))
            ++hung[c1];
        }
      else
        if (a2 >= ctlR || a1 < CTLP)
          s += ATAKD;
    }
    return s;
}

//Calculate the positional value for a queen on 'sq'.
static inline int QueenValue(short sq, short)
{
    short a1;
    short s = distance(sq, EnemyKing) < 3 ? 12 : 0;

    if (stage > 2)
        s += 14 - taxicab(sq, EnemyKing);

    short a2 = atk2[sq] & 0x4FFF;

    if (a2 > 0)
    {
        a1 = atk1[sq] & 0x4FFF;

        if (a1 == 0 || a2 > ctlQ + 1)
        {
            s += HUNGP;
            ++hung[c1];

            if (trapped (sq))
                ++hung[c1];
        }
        else if (a2 >= ctlQ || a1 < CTLP)
            s += ATAKD;
    }
    return s;
}

//Calculate the positional value for a king on 'sq'.
static inline int KingValue(short sq, short)
{
    short fyle, a2, a1;
    short s;

    s = Mking[c1][sq];
    if (KSFTY > 0)
        if (Developed[c2] || stage > 0)
            KingScan (sq, &s);

    if (castld[c1])
        s += KCASTLD;
    else if (Mvboard[kingP[c1]])
        s += KMOVD;

    fyle = column (sq);

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

  a2 = (atk2[sq] & 0x4FFF);
  if (a2 > 0)
    {
      a1 = (atk1[sq] & 0x4FFF);
      if (a1 == 0 || a2 > ctlK + 1)
        {
          s += HUNGP;
          ++hung[c1];
        }
      else
        s += ATAKD;
    }
  return (s);
}

/*
  Perform normal static evaluation of board position. A score is generated
  for each piece and these are summed to get a score for each side.
*/
void ScorePosition(short side, short *score)
{
    short sq, s, i;
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
      for (i = PieceCnt[c1]; i >= 0; i--)
        {
          sq = PieceList[c1][i];
          switch (board[sq])
            {
            case pawn:
              s = PawnValue(sq, side);
              break;
            case knight:
              s = KnightValue(sq, side);
              break;
            case bishop:
              s = BishopValue(sq, side);
              break;
            case rook:
              s = RookValue(sq, side);
              break;
            case queen:
              s = QueenValue(sq, side);
              break;
            case king:
              s = KingValue(sq, side);
              break;
            default:
              s = 0;
              break;
            }
          pscore[c1] += s;
          svalue[sq] = s;
        }
    }
  if (hung[side] > 1)
    pscore[side] += HUNGX;
  if (hung[xside] > 1)
    pscore[xside] += HUNGX;

  *score = mtl[side] - mtl[xside] + pscore[side] - pscore[xside] + 10;
  if (dither)
    *score += urand () % dither;

  if (*score > 0 && pmtl[side] == 0)
  {
    if (emtl[side] < valueR)
      *score = 0;
    else if (*score < valueR)
      *score /= 2;
  }

  if (*score < 0 && pmtl[xside] == 0)
  {
    if (emtl[xside] < valueR)
      *score = 0;
    else if (-*score < valueR)
      *score /= 2;
  }

  if (mtl[xside] == valueK && emtl[side] > valueB)
    *score += 200;
  if (mtl[side] == valueK && emtl[xside] > valueB)
    *score -= 200;
}


static inline void
BlendBoard(const short a[64], const short b[64], short c[64])
{
    for (int sq = 0; sq < 64; sq++)
        c[sq] = (a[sq] * (10 - stage) + b[sq] * stage) / 10;
}

static inline void CopyBoard(const short a[64], short b[64])
{
    for (int sq = 0; sq < 64; sq++)
        b[sq] = a[sq];
}

/*
  This is done one time before the search is started. Set up arrays
  Mwpawn, Mbpawn, Mknight, Mbishop, Mking which are used in the
  SqValue() function to determine the positional value of each piece.
*/
void ExaminePosition()
{
    short i, sq;
    short wpadv, bpadv, wstrong, bstrong, z, side, pp, j, k, val, Pd, fyle, rank;
    static short PawnStorm = false;

    ataks(white, atak[white]);
    ataks(black, atak[black]);
    UpdateWeights();
    HasKnight[white] = HasKnight[black] = 0;
    HasBishop[white] = HasBishop[black] = 0;
    HasRook[white] = HasRook[black] = 0;
    HasQueen[white] = HasQueen[black] = 0;

    for (side = white; side <= black; side++)
    {
        for (i = PieceCnt[side]; i >= 0; i--)
        {
            switch (board[PieceList[side][i]])
            {
            case knight:
                ++HasKnight[side];
                break;
            case bishop:
                ++HasBishop[side];
                break;
            case rook:
                ++HasRook[side];
                break;
            case queen:
                ++HasQueen[side];
                break;
            }
        }
    }

    if (!Developed[white])
    {
        Developed[white] = (board[1] != knight && board[2] != bishop &&
                            board[5] != bishop && board[6] != knight);
    }

    if (!Developed[black])
    {
        Developed[black] = (board[57] != knight && board[58] != bishop &&
                            board[61] != bishop && board[62] != knight);
    }

    if (!PawnStorm && stage < 5)
    {
        PawnStorm = ((column (wking) < 3 && column (bking) > 4) ||
                     (column (wking) > 4 && column (bking) < 3));
    }

    CopyBoard(pknight, Mknight[white]);
    CopyBoard(pknight, Mknight[black]);
    CopyBoard(pbishop, Mbishop[white]);
    CopyBoard(pbishop, Mbishop[black]);
    BlendBoard(KingOpening, KingEnding, Mking[white]);
    BlendBoard(KingOpening, KingEnding, Mking[black]);

    for (sq = 0; sq < 64; sq++)
    {
      fyle = column (sq);
      rank = row (sq);
      wstrong = bstrong = true;
      for (i = sq; i < 64; i += 8)
        if (Patak (black, i))
          {
            wstrong = false;
            break;
          }
      for (i = sq; i >= 0; i -= 8)
        if (Patak (white, i))
          {
            bstrong = false;
            break;
          }
      wpadv = bpadv = PADVNCM;
      if ((fyle == 0 || PawnCnt[white][fyle - 1] == 0) &&
          (fyle == 7 || PawnCnt[white][fyle + 1] == 0))
        wpadv = PADVNCI;
      if ((fyle == 0 || PawnCnt[black][fyle - 1] == 0) &&
          (fyle == 7 || PawnCnt[black][fyle + 1] == 0))
        bpadv = PADVNCI;
      Mwpawn[sq] = (wpadv * PawnAdvance[sq]) / 10;
      Mbpawn[sq] = (bpadv * PawnAdvance[63 - sq]) / 10;
      Mwpawn[sq] += PawnBonus;
      Mbpawn[sq] += PawnBonus;
      if (Mvboard[kingP[white]])
        {
          if ((fyle < 3 || fyle > 4) && distance (sq, wking) < 3)
            Mwpawn[sq] += PAWNSHIELD;
        }
      else if (rank < 3 && (fyle < 2 || fyle > 5))
        Mwpawn[sq] += PAWNSHIELD / 2;
      if (Mvboard[kingP[black]])
        {
          if ((fyle < 3 || fyle > 4) && distance (sq, bking) < 3)
            Mbpawn[sq] += PAWNSHIELD;
        }
      else if (rank > 4 && (fyle < 2 || fyle > 5))
        Mbpawn[sq] += PAWNSHIELD / 2;
      if (PawnStorm)
        {
          if ((column (wking) < 4 && fyle > 4) ||
              (column (wking) > 3 && fyle < 3))
            Mwpawn[sq] += 3 * rank - 21;
          if ((column (bking) < 4 && fyle > 4) ||
              (column (bking) > 3 && fyle < 3))
            Mbpawn[sq] -= 3 * rank;
        }
      Mknight[white][sq] += 5 - distance (sq, bking);
      Mknight[white][sq] += 5 - distance (sq, wking);
      Mknight[black][sq] += 5 - distance (sq, wking);
      Mknight[black][sq] += 5 - distance (sq, bking);
      Mbishop[white][sq] += BishopBonus;
      Mbishop[black][sq] += BishopBonus;
   {
      int xxxtmp;

      for (i = PieceCnt[black]; i >= 0; i--) {
         xxxtmp = PieceList[black][i];
        if (distance (sq, xxxtmp) < 3)
          Mknight[white][sq] += KNIGHTPOST;
      }
      for (i = PieceCnt[white]; i >= 0; i--) {
         xxxtmp = PieceList[white][i];
        if (distance (sq, xxxtmp) < 3)
          Mknight[black][sq] += KNIGHTPOST;
      }
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
      if (distance (sq, wking) == 1)
        Kfield[black][sq] = KATAK;
      if (distance (sq, bking) == 1)
        Kfield[white][sq] = KATAK;

      Pd = 0;
      for (k = 0; k <= PieceCnt[white]; k++)
        {
          i = PieceList[white][k];
          if (board[i] == pawn)
            {
              pp = true;
              if (row (i) == 6)
                z = i + 8;
              else
                z = i + 16;
              for (j = i + 8; j < 64; j += 8)
                if (Patak (black, j) || board[j] == pawn)
                  {
                    pp = false;
                    break;
                  }
              if (pp)
                Pd += 5 * taxicab (sq, z);
              else
                Pd += taxicab (sq, z);
            }
        }
      for (k = 0; k <= PieceCnt[black]; k++)
        {
          i = PieceList[black][k];
          if (board[i] == pawn)
            {
              pp = true;
              if (row (i) == 1)
                z = i - 8;
              else
                z = i - 16;
              for (j = i - 8; j >= 0; j -= 8)
                if (Patak (white, j) || board[j] == pawn)
                  {
                    pp = false;
                    break;
                  }
              if (pp)
                Pd += 5 * taxicab (sq, z);
              else
                Pd += taxicab (sq, z);
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
  If material balance has changed, determine the values for the positional
  evaluation terms.
*/
void UpdateWeights()
{
    emtl[white] = mtl[white] - pmtl[white] - valueK;
    emtl[black] = mtl[black] - pmtl[black] - valueK;
    short tmtl = emtl[white] + emtl[black];
    short s1 = tmtl > 6600 ? 0 : ((tmtl < 1400) ? 10 : (6600 - tmtl) / 520);

    if (s1 != stage)
    {
        stage = s1;
        stage2 = (tmtl > 3600) ? 0 : ((tmtl < 1400) ? 10 : (3600 - tmtl) / 220);
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

        if (stage < 8)
            KSFTY = 16 - 2 * stage;
        else
            KSFTY = 0;
      
        ATAKD = -6;               /* defender > attacker */
        HUNGP = -8;               /* each hung piece */
        HUNGX = -12;              /* extra for >1 hung piece */
    }
}

