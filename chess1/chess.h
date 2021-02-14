/*
  C source for GNU CHESS

  Revision: 1990-09-30
  Modified by Daryl Baker for use in MS WINDOWS environment

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

#ifndef CHESS_H
#define CHESS_H
#include "toolbox.h"

#define EXE_NAME_MAX_SIZE  128
#define BLACK 1
#define WHITE 2
#define MAXDEPTH 30
#define STDDEPTH 1
#define TTBLSZ (1 << 16)
#define white 0
#define black 1

static CONSTEXPR short PROMOTE = 0x0008, NEUTRAL = 2;
static CONSTEXPR short NO_PIECE = 0, PAWN = 1, KNIGHT = 2, BISHOP = 3, ROOK = 4, QUEEN = 5, KING = 6;
static CONSTEXPR short PMASK = 0x0007;

static CONSTEXPR WORD CSTLMASK = 0x0010, EPMASK = 0x0020, CAPTURE = 0x0200;

MAKRO int locn(int a, int b)
{
    return a << 3 | b;
}

MAKRO short column(short a)
{
    return a & 7;
}

struct Flags
{
    bool mate;         /* the game is over */
    bool post;         /* show principle variation */
    bool quit;         /* quit/exit gnuchess */
    bool reverse;      /* reverse board display */
    bool bothsides;    /* computer plays both sides */
    bool hash;         /* enable/disable transposition table */
    bool force;        /* enter moves */
    bool easy;         /* disable thinking on opponents time */
    bool beep;         /* enable/disable beep */
    bool timeout;      /* time to make a move */
    bool rcptr;        /* enable/disable recapture heuristics */
};

struct PIECEBITMAP
{
    HBITMAP piece;
    HBITMAP mask;
    HBITMAP outline;
};

struct TimeControlRec
{
    short moves[2];
    long clock[2];
};

struct Leaf
{
    short f, t, score, reply;
    WORD flags;
};

struct GameRec
{
    WORD gmove;
    short score, depth, time, piece, color;
    long nodes;
};

struct BookEntry
{
    BookEntry *next;
    WORD *mv;
};

struct HashVal
{
    DWORD key;
    DWORD bd;
};

struct hashentry
{
    DWORD hashbd;
    WORD mv;
    BYTE flags, depth;   /* char saves some space */
    short score;
#ifdef HASHTEST
    unsigned char bd[32];
#endif
};

struct fileentry
{
    BYTE bd[32];
    BYTE f, t, flags, depth, sh, sl;
};

#endif

