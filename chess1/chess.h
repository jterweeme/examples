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
#include <windows.h>

#define EXE_NAME_MAX_SIZE  128

#define BLACK 1
#define WHITE 2

#define NETURAL 2
#define MAXDEPTH 30

#define TTBLSZ (1 << 16)
#define white 0
#define black 1
#define neutral 2
#define no_piece 0
#define pawn 1
#define knight 2
#define bishop 3
#define rook 4
#define queen 5
#define king 6
#define bpawn 7
#define pmask 0x0007
#define PROMOTE 0x0008
#define CSTLMASK 0x0010
#define EPMASK 0x0020
#define capture 0x0200
#define valueP 100
#define valueN 350
#define valueB 355
#define valueR 550
#define valueQ 1100
#define valueK 1200
#define CTLP 0x4000
#define CTLN 0x2800
#define CTLB 0x1800
#define ctlR 0x0400
#define ctlQ 0x0200
#define ctlK 0x0100
#define ctlBQ 0x1200
#define CTLNN 0x2000
#define urand rand
#define column(a) ((a) & 7)
#define locn(a,b) (((a) << 3) | (b))

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

