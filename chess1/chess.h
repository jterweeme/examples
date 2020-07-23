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

#define BLACK 1
#define WHITE 2

#define NETURAL 2

#define PAWNBASE 0
#define PAWN   1
#define KNIGHT 2
#define BISHOP 3
#define ROOK   4
#define QUEEN  5
#define KING   6

#define PAWNM   7
#define KNIGHTM 8
#define BISHOPM 9
#define ROOKM   10
#define QUEENM  11
#define KINGM   12

#define PAWNO   13
#define KNIGHTO 14
#define BISHOPO 15
#define ROOKO   16
#define QUEENO  17
#define KINGO   18

#define ID_ABOUT  (WM_USER+1)

#define MSG_CHESS_NEW     (WM_USER+2)
#define MSG_CHESS_LIST    (WM_USER+3)
#define MSG_CHESS_GET     (WM_USER+4)
#define MSG_CHESS_SAVE    (WM_USER+5)
#define MSG_CHESS_QUIT    (WM_USER+6)
#define MSG_CHESS_HASH    (WM_USER+7)
#define MSG_CHESS_BEEP    (WM_USER+8)
#define MSG_CHESS_BOTH    (WM_USER+9)
#define MSG_CHESS_POST    (WM_USER+10)
#define MSG_CHESS_AWIN    (WM_USER+11)
#define MSG_CHESS_BWIN    (WM_USER+12)
#define MSG_CHESS_CONTEMP (WM_USER+13)
#define MSG_CHESS_UNDO    (WM_USER+14)
#define MSG_CHESS_ABOUT   (WM_USER+15)

#define MSG_CHESS_COORD   (WM_USER+16)
#define MSG_CHESS_REVIEW  (WM_USER+17)
#define MSG_CHESS_TEST    (WM_USER+18)
#define MSG_CHESS_BOOK    (WM_USER+19)

#define MSG_CHESS_RANDOM  (WM_USER+25)
#define MSG_CHESS_EASY    (WM_USER+26)
#define MSG_CHESS_DEPTH   (WM_USER+27)
#define MSG_CHESS_REVERSE (WM_USER+28)
#define MSG_CHESS_SWITCH  (WM_USER+29)
#define MSG_CHESS_BLACK   (WM_USER+30)
#define MSG_CHESS_WHITE   (WM_USER+31)
                                    
#define MSG_EDITBOARD     (WM_USER+32)
#define ID_EDITDONE       (WM_USER+33)

#define MSG_CHESS_EDIT    (WM_USER+34)
#define MSG_CHESS_EDITDONE (WM_USER+24)

#define MSG_USER_MOVE      (WM_USER+35)
#define MSG_USER_ENTERED_MOVE (WM_USER+36)
#define MSG_COMPUTER_MOVE  (WM_USER+37)

#define MSG_CHESS_HINT     (WM_USER+38)
#define MSG_CHESS_REMOVE   (WM_USER+39)
#define MSG_CHESS_FORCE    (WM_USER+40)
#define MSG_MANUAL_ENTRY_POINT    (WM_USER+41)

#define MSG_HELP_INDEX     (WM_USER+50)
#define MSG_HELP_HELP      (WM_USER+51)

#define MSG_DESTROY     (WM_USER+60)
#define MSG_WM_COMMAND  (WM_USER+61)

#define IDM_BACKGROUND  300
#define IDM_BLACKSQUARE 301
#define IDM_WHITESQUARE 302
#define IDM_BLACKPIECE  303
#define IDM_WHITEPIECE  304
#define IDM_DEFAULT     305
#define IDM_TEXT        307

#define IDM_TIMECONTROL 306

#define AboutBox    262
#define REVIEW      263
#define NUMBERDLG   264
#define COLOR       266
#define TIMECONTROL 267
#define TEST        269
#define PAWNPROMOTE 273
#define MANUALDLG   274
#endif

