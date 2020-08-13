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

#include "protos.h"
#include "resource.h"
#include "globals.h"
#include "toolbox.h"
#include "mainwin.h"
#include <ctime>
#include <iostream>

/*
   Generate move strings in different formats.
*/
void algbr(short f, short t, short flag)
{
    int m3p;

    if (f != t)
    {
        /* algebraic notation */
        ::mvstr[0][0] = char('a' + column(f));
        ::mvstr[0][1] = char('1' + (f >> 3));
        ::mvstr[0][2] = char('a' + column(t));
        ::mvstr[0][3] = char('1' + (t >> 3));
        ::mvstr[0][4] = ::mvstr[3][0] = '\0';

        if ((mvstr[1][0] = " PNBRQK"[board[f]]) == 'P')
        {
            if (mvstr[0][0] == mvstr[0][2])       /* pawn did not eat */
            {
                mvstr[2][0] = mvstr[1][0] = mvstr[0][2];  /* to column */
                mvstr[2][1] = mvstr[1][1] = mvstr[0][3];  /* to row */
                m3p = 2;
            }
            else    /* pawn ate */
            {
                mvstr[2][0] = mvstr[1][0] = mvstr[0][0];  /* from column */
                mvstr[2][1] = mvstr[1][1] = mvstr[0][2];  /* to column */
                mvstr[2][2] = mvstr[0][3];
                m3p = 3;          /* to row */
            }

            mvstr[2][m3p] = mvstr[1][2] = '\0';

            if (flag & PROMOTE)
            {
                mvstr[0][4] = mvstr[1][2] = mvstr[2][m3p] = " pnbrqk"[flag & PMASK];
                mvstr[1][3] = mvstr[2][m3p + 1] = mvstr[0][5] = '\0';
            }
        }
        else    /* not a pawn */
        {
            mvstr[2][0] = mvstr[1][0];
            mvstr[2][1] = mvstr[0][1];
            mvstr[2][2] = mvstr[1][1] = mvstr[0][2];      /* to column */
            mvstr[2][3] = mvstr[1][2] = mvstr[0][3];      /* to row */
            mvstr[2][4] = mvstr[1][3] = '\0';
            ::lstrcpy(::mvstr[3], ::mvstr[2]);
            mvstr[3][1] = mvstr[0][0];

            if (flag & CSTLMASK)
            {
                if (t > f)
                {
                    ::lstrcpy(mvstr[1], TEXT("o-o"));
                    ::lstrcpy(mvstr[2], TEXT("O-O"));
                }
                else
                {
                    ::lstrcpy(mvstr[1], TEXT("o-o-o"));
                    ::lstrcpy(mvstr[2], TEXT("O-O-O"));
                }
            }
        }
    }
    else
    {
        mvstr[0][0] = mvstr[1][0] = mvstr[2][0] = mvstr[3][0] = '\0';
    }

    std::cout << mvstr[0] << " " << mvstr[1] << " " << mvstr[2] << "\r\n";
    std::cout.flush();
}

/*
  Determine the time that has passed since the search was started. If
  the elapsed time exceeds the target (ResponseTime+ExtraTime) then set
  timeout to true which will terminate the search.
*/
void ElapsedTime(short iop, long extra, long responseTime, long xft)
{
    ::et = ::time(0) - ::time0;
    ::et = myMax(::et, long(0));
    ::ETnodes += 50;

    if (::et > ::et0 || iop == 1)
    {
        if (::et > responseTime + extra && ::Sdepth > 1)
            flag.timeout = true;

        et0 = et;

        if (iop == 1)
        {
            time0 = ::time(0);
            et0 = 0;
        }

        /* evrate used to be Nodes / cputime I dont know why */
        ::evrate = ::et > 0 ? ::NodeCnt / (::et + xft) : 0;
        ETnodes = NodeCnt + 50;
        UpdateClocks();
    }
}

void SetTimeControl(long xft)
{
    if (::TCflag)
    {
        ::TimeControl.moves[white] = ::TimeControl.moves[black] = ::TCmoves;
        ::TimeControl.clock[white] = ::TimeControl.clock[black] = 60 * long(::TCminutes);
    }
    else
    {
        ::TimeControl.moves[white] = TimeControl.moves[black] = 0;
        ::TimeControl.clock[white] = TimeControl.clock[black] = 0;
        ::Level = 60 * long(TCminutes);
    }
    ::et = 0;
    ElapsedTime(1, ExtraTime, ResponseTime, xft);
}

void GetGame(HWND hWnd, HWND compClr, char *fname)
{
    WORD m;
    GameRec tmp_rec;
    FILE *fd = fopen(fname, "r");

    if (fd == NULL)
        throw IDS_LOADFAILED;

    fscanf(fd, "%hd%hd%hd", &computer, &opponent, &Game50);
    fscanf(fd, "%hd%hd", &castld[white], &castld[black]);
    fscanf(fd, "%hd%hd", &TCflag, &OperatorTime);

    fscanf(fd, "%ld%ld%hd%hd",
          &TimeControl.clock[white], &TimeControl.clock[black],
          &TimeControl.moves[white], &TimeControl.moves[black]);

    for (short sq = 0; sq < 64; sq++)
    {
        ::fscanf(fd, "%hd%hd", &m, &Mvboard[sq]);
        board[sq] = (m >> 8);
        color[sq] = (m & 0xFF);

        if (color[sq] == 0)
            color[sq] = NEUTRAL;
        else
            --color[sq];
    }

    GameCnt = 0;
    int c = '?';

    while (c != EOF)
    {
        ++GameCnt;
        c = fscanf(fd, "%hd%hd%hd%ld%hd%hd%hd", &tmp_rec.gmove,
                  &tmp_rec.score, &tmp_rec.depth,
                  &tmp_rec.nodes, &tmp_rec.time,
                  &tmp_rec.piece, &tmp_rec.color);

        GameList[GameCnt] = tmp_rec;

        if (GameList[GameCnt].color == 0)
            GameList[GameCnt].color = NEUTRAL;
        else
            --GameList[GameCnt].color;
    }

    GameCnt--;

    if (TimeControl.clock[white] > 0)
        TCflag = true;

    computer--;
    opponent--;
    ::fclose(fd);
    InitializeStats();
    Sdepth = 0;
    UpdateDisplay(hWnd, compClr, 0, 0, 1, 0, flag.reverse);
}

void ListGame(char *fname)
{
    FILE *fd = fopen(fname, "w");

    if (fd == NULL)
        throw TEXT("Cannot write chess.lst");

    ::fprintf(fd, "\n");
    ::fprintf(fd, "       score  depth   nodes  time         ");
    ::fprintf(fd, "       score  depth   nodes  time\n");

    for (short i = 1; i <= GameCnt; i++)
    {
        short f = GameList[i].gmove >> 8;
        short t = GameList[i].gmove & 0xFF;
        algbr(f, t, false);

        ::fprintf(fd, "%5s  %5d     %2d %7ld %5d", mvstr[0],
               GameList[i].score, GameList[i].depth,
               GameList[i].nodes, GameList[i].time);

        if (i % 2 == 0)
            fprintf(fd, "\n");
        else
            fprintf(fd, "         ");
    }

    ::fprintf(fd, "\n\n");
    ::fclose(fd);
}

