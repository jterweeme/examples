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
#define maxdepth 30

struct flags
{
    short mate;         /* the game is over */
    short post;         /* show principle variation */
    short quit;         /* quit/exit gnuchess */
    short reverse;      /* reverse board display */
    short bothsides;    /* computer plays both sides */
    short hash;         /* enable/disable transposition table */
    short force;        /* enter moves */
    short easy;         /* disable thinking on opponents time */
    short beep;         /* enable/disable beep */
    short timeout;      /* time to make a move */
    short rcptr;        /* enable/disable recapture heuristics */
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

struct leaf
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
    struct BookEntry *next;
    WORD *mv;
};

struct hashval
{
    DWORD key,bd;
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

