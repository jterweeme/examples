#include "globals.h"
#include "toolbox.h"
#include "resource.h"
#include "board.h"
#include "sim.h"
#include "zeit.h"
#include <iostream>

BookEntry *Book;
DWORD hashkey, hashbd;
WORD hint;
Leaf *Tree;
Flags flag;
GameRec *GameList;
TimeControlRec TimeControl;
HWND hMsgComputer, hStats;
GLOBALHANDLE hBook = 0;
TCHAR mvstr[4][6];
long ResponseTime, ExtraTime, Level, et, et0, time0, ft, NodeCnt, ETnodes, evrate;
short TrPnt[MAXDEPTH], PieceList[2][16];

short Mvboard[64];
short opponent, computer;
short dither, INCscore;
short Sdepth, GameCnt, Game50, epsquare, contempt;
short TCflag, TCmoves, TCminutes, OperatorTime, player;
short boarddraw[64], colordraw[64];
short board[64];
short color[64];
const short otherside[3] = {1, 0, 2};
HWND hWhosTurn;
HWND hComputerMove;
HWND hClockHuman;
HWND hClockComputer;

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
    SystemTime sysTime;
    sysTime.getTime();
    ::et = sysTime.unix() - ::time0;
    ::et = Toolbox::myMax(::et, long(0));
    ::ETnodes += 50;

    if (::et > ::et0 || iop == 1)
    {
        if (::et > responseTime + extra && ::Sdepth > 1)
            flag.timeout = true;

        et0 = et;

        if (iop == 1)
        {
            sysTime.getTime();
            time0 = sysTime.unix();
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

static LPCTSTR ColorStr[2] = {TEXT("White"), TEXT("Black")};

void ShowSidetoMove()
{
    TCHAR tmp[30];
    ::wsprintf(tmp, TEXT("It is %s's turn"), ColorStr[player]);
    ::SetWindowText(hWhosTurn, tmp);
}

void UpdateClocks()
{
    TCHAR tmp[20];
    short m = short(::et / 60);
    short s = short(::et - 60 * (long)m);

    if (TCflag)
    {
        m = short((TimeControl.clock[player] - ::et) / 60);
        s = short(TimeControl.clock[player] - ::et - 60 * (long)m);
    }

    m = Toolbox::myMax(m, short(0));
    s = Toolbox::myMax(s, short(0));
    ::wsprintf(tmp, TEXT("%0d:%02d"), m, s);
    ::SetWindowText(player == white ? hClockHuman : hClockComputer, tmp);

    if (flag.post)
        ShowNodeCnt(hStats, NodeCnt, evrate);
}

static INT_PTR CALLBACK
StatDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM)
{
    switch (message)
    {
    case WM_INITDIALOG:
        SetDlgItemText(hDlg, DEPTHTEXT,    TEXT(" "));
        SetDlgItemText(hDlg, POSITIONTEXT, TEXT(" "));
        SetDlgItemText(hDlg, NODETEXT,     TEXT(" "));
        SetDlgItemText(hDlg, BSTLINETEXT,  TEXT(" "));
        SetDlgItemText(hDlg, SCORETEXT,    TEXT(" "));
        SetDlgItemText(hDlg, NODESECTEXT,  TEXT(" "));
        return (TRUE);
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_CLOSE)
        {
            ::DestroyWindow(hDlg);
            return TRUE;
        }
        break;
    case WM_DESTROY:
        hStats = NULL;
        flag.post = false;
        break;
    }

    return FALSE;
}

void StatDialog(HWND hWnd, HINSTANCE hInst)
{
    CreateDialog(hInst, MAKEINTRESOURCE(STATS), hWnd, StatDlgProc);
}

void ShowPlayers(HWND hwnd)
{
    /* display in the status line what color the computer is playing */
    ::SetWindowText(hwnd, computer == black ? TEXT("Computer is black") : TEXT("Computer is white"));
}

void ShowCurrentMove(HWND hwnd, short pnt, short f, short t)
{
    TCHAR tmp[30];

    if (hwnd)
    {
        algbr(f, t, false);
        ::wsprintf(tmp, TEXT("(%2d) %4s"), pnt, LPCTSTR(mvstr[0]));
        ::SetDlgItemText(hwnd, POSITIONTEXT, tmp);
    }
}

void ShowNodeCnt(HWND hwnd, long NodeCnt, long evrate)
{
    TCHAR tmp[40];

    if (hwnd)
    {
        ::wsprintf(tmp, TEXT("%-8ld"), NodeCnt);
        ::SetDlgItemText(hwnd, NODETEXT, tmp);
        ::wsprintf(tmp, TEXT("%-5ld"), evrate);
        ::SetDlgItemText(hwnd, NODESECTEXT, tmp);
    }
}

void SearchStartStuff(short int)
{
}

static void DrawPiece(HWND hWnd, short f, bool reverse)
{
    int x = reverse ? 7 - f % 8 : f % 8;
    int y = reverse ? 7 - f / 8 : f / 8;
    POINT aptl[4];
    Board::QuerySqCoords(x, y, aptl + 0);
    HRGN hRgn;
#ifdef WINCE
    hRgn = ::CreateRectRgn(aptl[0].x, aptl[0].y, aptl[2].x, aptl[2].y);
#else
    hRgn = ::CreatePolygonRgn(aptl, 4, WINDING);
#endif
    ::InvalidateRgn(hWnd, hRgn, FALSE );
    ::DeleteObject(hRgn);
}

void UpdateDisplay(HWND hWnd, HWND compClr, short f, short t,
                   short redraw, short isspec, bool reverse)
{
    for (short sq = 0; sq < 64; sq++)
    {
        boarddraw[sq] = board[sq];
        colordraw[sq] = color[sq];
    }

    if (redraw)
    {
        ::InvalidateRect(hWnd, NULL, TRUE);
        ShowPlayers(compClr);
        ::UpdateWindow(hWnd);
        return;
    }


    DrawPiece(hWnd, f, reverse);
    DrawPiece(hWnd, t, reverse);

    if (isspec & CSTLMASK)
    {
        if (t > f)
        {
            DrawPiece(hWnd, f + 3, reverse);
            DrawPiece(hWnd, t - 1, reverse);
        }
        else
        {
            DrawPiece(hWnd, f - 4, reverse);
            DrawPiece(hWnd, t + 1, reverse);
        }
    }
    else if (isspec & EPMASK)
    {
        DrawPiece(hWnd, t - 8, reverse);
        DrawPiece(hWnd, t + 8, reverse);
    }

    UpdateWindow(hWnd);

}







