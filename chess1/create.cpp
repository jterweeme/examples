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

#include "defs.h"
#include "chess.h"
#include "globals.h"

void Create_Children ( HWND hWnd, HINSTANCE hInst, short xchar, short ychar)
{
    POINT pt;
    static char lpStatic[] = "Static";

    /* Get the location of lower left conor of client area */
    QueryBoardSize ( &pt);
   
    hComputerColor = CreateWindowA(lpStatic, NULL,
                           WS_CHILD | SS_CENTER | WS_VISIBLE, 0, pt.y,
                            10*xchar, ychar, hWnd, HMENU(1000), hInst, NULL);

    hWhosTurn = CreateWindowA(lpStatic, NULL,
                     	WS_CHILD | SS_CENTER | WS_VISIBLE,
                     	10*xchar,
                     	pt.y,
                     	10*xchar,
                     	ychar,
                        hWnd, HMENU(1001), hInst, NULL);

    hComputerMove = CreateWindowA(lpStatic, NULL,
                     	WS_CHILD | SS_LEFT | WS_VISIBLE,
                     	375 /*0*/,
                     	10 /*pt.y+(3*ychar)/2*/,
                     	10*xchar,
                     	ychar,
                        hWnd, HMENU(1003), hInst, NULL);


    hClockComputer = CreateWindowA( lpStatic, NULL,
                     	WS_CHILD | SS_CENTER | WS_VISIBLE,
                        390, 55, 6*xchar, ychar, hWnd,
                        HMENU(1010), hInst, NULL);

    hClockHuman =  CreateWindowA( lpStatic, NULL,
                     	WS_CHILD | SS_CENTER | WS_VISIBLE,
                        390, 55+3*ychar, 6*xchar, ychar, hWnd,
                        HMENU(1011), hInst, NULL);

    hMsgComputer = CreateWindowA( lpStatic, "Black:",
                     	WS_CHILD | SS_CENTER | WS_VISIBLE,
                        390, 55-3*ychar/2, 6*xchar, ychar, hWnd,
                        HMENU(1020), hInst, NULL);

    hMsgHuman    = CreateWindowA( lpStatic, "White:",
                      	WS_CHILD | SS_CENTER | WS_VISIBLE,
                        390, 55+3*ychar/2, 6*xchar, ychar, hWnd,
                        HMENU(1021), hInst, NULL);
}
