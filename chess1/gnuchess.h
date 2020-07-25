/*
  gnuchess.h - Header file for GNU CHESS

  Revision: 1990-04-18

  Copyright (C) 1986, 1987, 1988, 1989, 1990 Free Software Foundation, Inc.

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

#ifndef GNUCHESS_H
#define GNUCHESS_H

#include <windows.h>

/*
  TTBLSZ must be a power of 2.
  Setting TTBLSZ 0 removes the transposition tables.
*/
#define TTBLSZ (1 << 16)
#define huge
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
#define promote 0x0008
#define cstlmask 0x0010
#define epmask 0x0020
#define exact 0x0040
#define pwnthrt 0x0080
#define check 0x0100
#define capture 0x0200
#define draw 0x0400
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
#define ctlBN 0x0800
#define ctlRQ 0x0600
#define CTLNN 0x2000

#define TRUESCORE 0x0001
#define LOWERBOUND 0x0002
#define UPPERBOUND 0x0004
#define KINGCASTLE 0x0008
#define QUEENCASTLE 0x0010

/*
  persistent transposition table.
  The size must be a power of 2. If you change the size,
  be sure to run gnuchess -t before anything else.
*/
#define frehash 6
#define filesz (1 << 17)

/*
  In a networked enviroment gnuchess might be compiled on different
  hosts with different random number generators, that is not acceptable
  if they are going to share the same transposition table.
*/
#define urand rand

#define distance(a, b) *(distdata + a * 64 + b)
#define row(a) ((a) >> 3)
#define column(a) ((a) & 7)
#define locn(a,b) (((a) << 3) | (b))
#endif

