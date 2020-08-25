#include "globals.h"
#include "toolbox.h"
#include "resource.h"
#include "board.h"
#include "sim.h"
#include "zeit.h"
#include <iostream>

BookEntry *Book;
DWORD hashkey, hashbd;
WORD hint, PrVar[MAXDEPTH];
Leaf *Tree;
Flags flag;
GameRec *GameList;
TimeControlRec TimeControl;
HWND hMsgComputer, hStats;
GLOBALHANDLE hBook = 0;
TCHAR mvstr[4][6];
long ResponseTime, ExtraTime, Level, et, et0, time0, ft, NodeCnt, ETnodes, evrate;
short TrPnt[MAXDEPTH], PieceList[2][16], castld[2], Mvboard[64];
short opponent, computer, Awindow, Bwindow, dither, INCscore;
short Sdepth, GameCnt, Game50, epsquare, contempt;
short TCflag, TCmoves, TCminutes, OperatorTime, player;
short Pindex[64], PieceCnt[2], c1, c2;
short Tscore[MAXDEPTH], boarddraw[64], colordraw[64];
short board[64], color[64];
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
    Sim::InitializeStats();
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

void TestSpeed(HWND hWnd, int cnt, void (*f) (short int side, short int ply))
{
    long evrate;
    TCHAR tmp[40];
    SystemTime sysTime;
    sysTime.getTime();
    long t1 = sysTime.unix();

    for (short i = 0; i < 10000; i++)
    {
        f(opponent, 2);
    }

    sysTime.getTime();
    long t2 = sysTime.unix();
    NodeCnt = 10000L * (TrPnt[3] - TrPnt[2]);
    evrate = NodeCnt / (t2 - t1);
    wsprintf(tmp, TEXT("Nodes= %8ld, Nodes/Sec= %5ld"), NodeCnt, evrate);
    SetDlgItemText(hWnd, cnt, tmp);
}

static INT_PTR CALLBACK
TestDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM)
{
    HCURSOR hCursor;

    switch (message)
    {
    case WM_INITDIALOG:
        ::SetDlgItemText(hDlg, 100, TEXT(" "));
        ::SetDlgItemText(hDlg, 101, TEXT(" "));
        ::PostMessage(hDlg, WM_USER + 1, 0, 0);
        return TRUE;
    case (WM_USER+1):
        hCursor = ::SetCursor(::LoadCursor(NULL, IDC_WAIT) );
        ::ShowCursor(TRUE);
        ::TestSpeed(hDlg, 100, Sim::MoveList);
        ::TestSpeed(hDlg, 101, CaptureList);
        ::ShowCursor(FALSE);
        ::SetCursor(hCursor);
        break;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_CLOSE)
        {
            ::EndDialog(hDlg, 0);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

int TestDialog(HWND hWnd, HINSTANCE hInst)
{
    int status;
    status = DialogBox(hInst, MAKEINTRESOURCE(TEST), hWnd, TestDlgProc);
    return status;
}

static int NumberDlgInt;
static TCHAR NumberDlgChar[80];

static INT_PTR CALLBACK
NumberDlgDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM)
{
    int temp, Ier;
    switch (iMessage)
    {
    case WM_INITDIALOG:
        SetDlgItemText(hDlg, IDD_CHAR, NumberDlgChar);
        SetDlgItemInt(hDlg, IDD_INT, NumberDlgInt, TRUE);
        return TRUE;
    case WM_COMMAND:
        switch (wParam)
        {
        case IDOK:
            temp = GetDlgItemInt(hDlg, IDD_INT, &Ier, TRUE);

            if (Ier != 0)
            {
                  NumberDlgInt = temp;
                  EndDialog ( hDlg, TRUE);
            }
            return FALSE;
        case IDCANCEL:
            EndDialog ( hDlg, TRUE);
            return FALSE;
        }
        return FALSE;
    }
    return TRUE;
}

int DoGetNumberDlg(HINSTANCE hInst, HWND hWnd, TCHAR *szPrompt, int def)
{
    ::lstrcpy(::NumberDlgChar, szPrompt);
    ::NumberDlgInt = def;
    ::DialogBox(hInst, MAKEINTRESOURCE(NUMBERDLG), hWnd, NumberDlgDlgProc);
    return ::NumberDlgInt;
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
