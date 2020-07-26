#include "mainwin.h"
#include "globals.h"
#include "gnuchess.h"
#include "protos.h"
#include "resource.h"
#include "winclass.h"
#include <ctime>

MainWindow *MainWindow::_instance = 0;

MainWindow::MainWindow(WinClass *wc) : _wc(wc)
{
    _instance = this;
    FirstSq = -1;
    GotFirst = FALSE;
    EditActive = FALSE;
    User_Move = TRUE;
}

#ifdef WINCE
#define WINSTYLE WS_CLIPCHILDREN
#else
#define WINSTYLE WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN
#endif

void MainWindow::create(LPCTSTR appName)
{
    _hwnd = CreateWindow(appName, appName, WINSTYLE,
                    CW_USEDEFAULT, CW_USEDEFAULT,
                    CW_USEDEFAULT, CW_USEDEFAULT,
                    NULL, NULL, _wc->hInstance(), NULL);

    if (!_hwnd)
        throw TEXT("Cannot create main window");
}

HWND MainWindow::hwnd() const
{
    return _hwnd;
}

void MainWindow::show(int nCmdShow)
{
    ::ShowWindow(_hwnd, nCmdShow);
}

void MainWindow::_initMenus(WPARAM wParam, LPARAM lParam)
{
    UINT status;
    HMENU hMenu = HMENU(wParam);

    if (lParam == MENU_ID_FILE)
    {
        status = GameCnt > 0 ? MF_ENABLED : MF_GRAYED;
        ::EnableMenuItem(hMenu, MSG_CHESS_SAVE, status);
        ::EnableMenuItem(hMenu, MSG_CHESS_LIST, status);
    }
    else if (lParam == MENU_ID_EDIT)
    {
        status = GameCnt > 0 ? MF_ENABLED : MF_GRAYED;
        ::EnableMenuItem(hMenu, MSG_CHESS_REVIEW, status);
        ::EnableMenuItem(hMenu, MSG_CHESS_UNDO, status);
        status = GameCnt > 1 ? MF_ENABLED : MF_GRAYED;
        ::EnableMenuItem(hMenu, MSG_CHESS_REMOVE, status);
        status = flag.force == false ? MF_UNCHECKED : MF_CHECKED;
        ::CheckMenuItem(hMenu, MSG_CHESS_FORCE, status);
    }
    else if (lParam == MENU_ID_OPTIONS)
    {
        status = flag.hash == false ? MF_UNCHECKED : MF_CHECKED;
        ::CheckMenuItem(hMenu, MSG_CHESS_HASH, status);
        status = coords == false ? MF_UNCHECKED : MF_CHECKED;
        ::CheckMenuItem(hMenu, MSG_CHESS_COORD, status);
        status = flag.beep == false ? MF_UNCHECKED : MF_CHECKED;
        ::CheckMenuItem(hMenu, MSG_CHESS_BEEP, status);
        status = flag.bothsides == false ? MF_UNCHECKED : MF_CHECKED;
        ::CheckMenuItem(hMenu, MSG_CHESS_BOTH, status);
        status = Book == NULL ? MF_UNCHECKED : MF_CHECKED;
        ::CheckMenuItem(hMenu, MSG_CHESS_BOOK, status);
        status = flag.post == false ? MF_UNCHECKED : MF_CHECKED;
        ::CheckMenuItem(hMenu, MSG_CHESS_POST, status);
    }
    else if (lParam == MENU_ID_SKILL)
    {
        status = dither == 0 ? MF_UNCHECKED : MF_CHECKED;
        ::CheckMenuItem(hMenu, MSG_CHESS_RANDOM, status);
        status = flag.easy == false ? MF_UNCHECKED : MF_CHECKED;
        ::CheckMenuItem(hMenu, MSG_CHESS_EASY, status);
    }
    else if (lParam == MENU_ID_SIDE)
    {
        status = flag.reverse == false ? MF_UNCHECKED : MF_CHECKED;
        ::CheckMenuItem(hMenu, MSG_CHESS_REVERSE, status);

        if (computer == black)
        {
            ::CheckMenuItem(hMenu, MSG_CHESS_BLACK, MF_CHECKED);
            ::CheckMenuItem(hMenu, MSG_CHESS_WHITE, MF_UNCHECKED);
        }
        else
        {
            ::CheckMenuItem(hMenu, MSG_CHESS_WHITE, MF_CHECKED);
            ::CheckMenuItem(hMenu, MSG_CHESS_BLACK, MF_UNCHECKED);
        }
    }
}

HINSTANCE MainWindow::hInstance() const
{
    return _wc->hInstance();
}

void MainWindow::MakeHelpPathName(TCHAR *szFileName)
{
    int nFileNameLen = ::GetModuleFileName(hInstance(), szFileName, EXE_NAME_MAX_SIZE);
    TCHAR *pcFileName = szFileName + nFileNameLen;

    while (pcFileName > szFileName)
    {
        if (*pcFileName == '\\' || *pcFileName == ':')
        {
            *(++pcFileName) = '\0';
            break;
        }
        nFileNameLen--;
        pcFileName--;
    }

    if ((nFileNameLen+13) < EXE_NAME_MAX_SIZE)
    {
        ::lstrcat(szFileName, TEXT("chess.hlp"));
    }
    else
    {
        ::lstrcat(szFileName, TEXT("?"));
    }
}

void MainWindow::_paintProc(HWND hWnd) const
{
    if (FirstSq != -1)
    {
        POINT pt;
        RECT rect;
        QuerySqOrigin(FirstSq % 8, FirstSq / 8, &pt);
        rect.left = pt.x;
        rect.right=pt.x+48;
        rect.top = pt.y-48;
        rect.bottom = pt.y;
        InvalidateRect(hWnd, &rect, FALSE);
    }

    PAINTSTRUCT ps;
    HDC hDC = ::BeginPaint(hWnd, &ps);
    Draw_Board(hDC, flag.reverse, clrBlackSquare, clrWhiteSquare);

    if (coords)
        DrawCoords(hDC, flag.reverse, clrBackGround, clrText);

    DrawAllPieces(hDC, flag.reverse, boarddraw, colordraw, clrBlackPiece, clrWhitePiece);
    ::EndPaint(hWnd, &ps);

    if (FirstSq != -1)
        HiliteSquare(hWnd, FirstSq);
}

void MainWindow::_commandProc(HWND hWnd, WPARAM wParam, LPCTSTR appName)
{
    OFSTRUCT pof;
    TCHAR str[80];
    HMENU hMenu;
    TCHAR FileName[256];

    switch (wParam)
    {
    case MSG_CHESS_QUIT:
        ::PostMessage(hWnd, MSG_DESTROY, 0, 0);
        break;
    case MSG_CHESS_HINT:
        GiveHint(hWnd);
        break;
    case MSG_CHESS_LIST:
    {
        int status;

        if (!DoFileSaveDlg(hInstance(), hWnd, TEXT("chess.lst"), TEXT(".lst"), &status, FileName, &pof))
            break;

        if (status == 1)
        {
            ::lstrcpy(str, TEXT("Replace Existing"));
            ::lstrcat(str, FileName);

            if (::MessageBox(hWnd, str, appName, MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL) == IDNO)
                break;
        }
        else
        {
#ifndef UNICODE
              OpenFile(FileName, &pof, OF_PARSE);
#endif
        }

        ListGame(hWnd, pof.szPathName);
    }
        break;
    case MSG_CHESS_SAVE:
    {
        int status;

        if (!DoFileSaveDlg(hInstance(), hWnd, TEXT("chess.chs"), TEXT(".chs"), &status, FileName, &pof))
            break;

        if (status == 1)
        {
            ::lstrcpy(str, TEXT("Replace Existing"));
            ::lstrcat(str, FileName);

            if (::MessageBox(hWnd, str, appName, MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL) == IDNO)
                break;
        }
        else
        {
#ifndef UNICODE
              OpenFile(FileName, &pof, OF_PARSE);
#endif
        }

        SaveGame(hWnd, pof.szPathName);
    }
        break;
    case MSG_CHESS_GET:
    {
          if (!DoFileOpenDlg(hInstance(), hWnd, TEXT("*.chs"), TEXT(".chs"), 0x0, FileName, &pof))
             break;
#ifndef UNICODE
          HFILE hFile;

          if ((hFile = OpenFile(FileName, &pof, OF_READ|OF_REOPEN)) == -1)
          {
             lstrcpy(str, "Cannot open file: ");
             lstrcat(str, FileName);
             ::MessageBox(hWnd, str, appName, MB_OK | MB_APPLMODAL | MB_ICONQUESTION);
             break;
          }

          _lclose(hFile);
#endif
          GetGame(hWnd, pof.szPathName);
    }
          break;
    case MSG_CHESS_NEW:
        NewGame(hWnd);

        if (hBook)
              FreeBook();

        GetOpenings(hInstance());
        break;
    case MSG_CHESS_ABOUT:
        DialogBox(hInstance(), MAKEINTRESOURCE(AboutBox), hWnd, About);
        break;
    case MSG_CHESS_EDIT:
        EditActive = TRUE;
        hMenu = CreateMenu();
        AppendMenu(hMenu, MF_STRING, MSG_CHESS_EDITDONE, TEXT("&Done"));
        hMainMenu = GetMenu(hWnd);
        SetMenu(hWnd, hMenu);
        DrawMenuBar(hWnd);
        break;
    case MSG_CHESS_EDITDONE:
          EditActive = FALSE;
          hMenu = GetMenu(hWnd);
          SetMenu(hWnd, hMainMenu);
          DrawMenuBar(hWnd);
          DestroyMenu(hMenu);
          GameCnt = 0;
          Game50 = 1;
          ZeroRPT ();
          Sdepth = 0;
          InitializeStats();
          ::PostMessage(hWnd, MSG_USER_MOVE, NULL, (long)NULL);
          break;
       case MSG_CHESS_REVIEW:
          ReviewDialog(hWnd, hInstance());
          break;
       case MSG_CHESS_TEST:
          TestDialog(hWnd, hInstance());
          break;
       case MSG_CHESS_HASH:
          flag.hash = !flag.hash;
          break;
       case MSG_CHESS_BEEP:
          flag.beep = !flag.beep;
          break;
       case MSG_CHESS_COORD:
          coords = !coords;
          UpdateDisplay(hWnd, 0, 0, 1, 0);
          break;
       case MSG_CHESS_BOTH:
          flag.bothsides = !flag.bothsides;
          flag.easy = true;
          Sdepth = 0;
          ::PostMessage(hWnd, MSG_USER_MOVE, NULL, (long)NULL);
          break;
       case MSG_CHESS_BOOK:
#if 0
          if ( Book != NULL ) {
             FreeBook ();
             Book = NULL;
          }
#endif
          break;
       case MSG_CHESS_POST:
          if (flag.post)
          {
             ::SendMessage(hStats, WM_SYSCOMMAND, SC_CLOSE, 0);
             flag.post = false;
          }
          else
          {
             StatDialog(hWnd, hInstance());
             flag.post = TRUE;
          }
          break;
       case MSG_CHESS_AWIN:
       {
          TCHAR str[40];
          ::LoadString(hInstance(), IDS_SETAWIN, str, sizeof(str)),
          Awindow = DoGetNumberDlg(hInstance(), hWnd, str, Awindow);
       }
          break;
       case MSG_CHESS_BWIN:
       {
          TCHAR str[40];
          ::LoadString(hInstance(), IDS_SETBWIN, str, sizeof(str));
          Bwindow = DoGetNumberDlg(hInstance(), hWnd, str, Bwindow);
       }
          break;

       case MSG_CHESS_CONTEMP:
       {
          TCHAR str[40];
          LoadString(hInstance(), IDS_SETCONTEMPT, str, sizeof(str));
          contempt = DoGetNumberDlg(hInstance(), hWnd, str, contempt);
       }
          break;
       case MSG_CHESS_UNDO:
          if (GameCnt >0)
          {
             Undo(hWnd);
             player = opponent;
             ShowSidetoMove();
          }
          break;
       case MSG_CHESS_REMOVE:
          if (GameCnt > 1)
          {
             Undo(hWnd);
             Undo(hWnd);
             ShowSidetoMove();
          }
          break;
       case MSG_CHESS_FORCE:
          flag.force = !flag.force;
          player = opponent;
          ShowPlayers ();
          break;
       case MSG_CHESS_RANDOM:
          if (dither == 0)
              dither = 6;
          else
              dither = 0;
          break;
       case MSG_CHESS_EASY:
          flag.easy = !flag.easy;
          break;

       case MSG_CHESS_DEPTH:
       {
          TCHAR str[40];
          LoadString(hInstance(), IDS_MAXSEARCH, str, sizeof(str));
          MaxSearchDepth = DoGetNumberDlg(hInstance(), hWnd, str, MaxSearchDepth);
       }
          break;
       case MSG_CHESS_REVERSE:
          flag.reverse = !flag.reverse;
          ShowPlayers ();
          UpdateDisplay(hWnd,0,0,1,0);
          break;
       case MSG_CHESS_SWITCH:
          computer = otherside[computer];
          opponent = otherside[opponent];
          flag.force = false;
          Sdepth = 0;
          ShowPlayers();
          PostMessage(hWnd, MSG_COMPUTER_MOVE, NULL, (long)NULL);
          break;
       case MSG_CHESS_BLACK:
          computer = black;
          opponent = white;
          flag.force = false;
          Sdepth = 0;
          ShowPlayers ();
          PostMessage ( hWnd, MSG_COMPUTER_MOVE, NULL, (long)NULL);
          break;

       case MSG_CHESS_WHITE:
          computer = white;
          opponent = black;
          flag.force = false;
          Sdepth = 0;
          ShowPlayers();
          PostMessage(hWnd, MSG_COMPUTER_MOVE, NULL, (long)NULL);
          break;

       case IDM_BACKGROUND:
          if (ColorDialog(hWnd, hInstance(), wParam))
          {
             InvalidateRect (hWnd, NULL, TRUE);
             DeleteObject (hBrushBackGround);
             hBrushBackGround = CreateSolidBrush ( clrBackGround );

             /*Invalidate the text windows so they repaint */
             InvalidateRect(hComputerColor, NULL, TRUE);
             InvalidateRect(hComputerMove, NULL, TRUE);
             InvalidateRect(hWhosTurn, NULL, TRUE);
             InvalidateRect(hClockHuman, NULL, TRUE);
             InvalidateRect(hClockComputer, NULL, TRUE);
             InvalidateRect(hMsgComputer, NULL, TRUE);
             InvalidateRect(hMsgHuman, NULL, TRUE);
          }
          break;

       case IDM_BLACKSQUARE:
          if (ColorDialog(hWnd, hInstance(), wParam))
          {
             ::InvalidateRect(hWnd, NULL, TRUE);
          }
          break;
       case IDM_WHITESQUARE:
          if (ColorDialog(hWnd, hInstance(), wParam))
          {
             ::InvalidateRect(hWnd, NULL, TRUE);
          }
          break;
       case IDM_BLACKPIECE:
          if (ColorDialog(hWnd, hInstance(), wParam))
          {
             ::InvalidateRect(hWnd, NULL, TRUE);
          }
          break;
       case IDM_WHITEPIECE:
          if (ColorDialog(hWnd, hInstance(), wParam))
          {
              InvalidateRect(hWnd, NULL, TRUE);
          }
          break;
       case IDM_TEXT:
          if (ColorDialog(hWnd, hInstance(), wParam))
          {
             /*Invalidate the text windows so they repaint */
             InvalidateRect (hWnd, NULL, TRUE);
             InvalidateRect (hComputerColor, NULL, TRUE);
             InvalidateRect (hComputerMove, NULL, TRUE);
             InvalidateRect (hWhosTurn, NULL, TRUE);
             InvalidateRect (hClockHuman, NULL, TRUE);
             InvalidateRect (hClockComputer, NULL, TRUE);
             InvalidateRect (hMsgComputer, NULL, TRUE);
             InvalidateRect (hMsgHuman, NULL, TRUE);
          }
          break;

    case IDM_DEFAULT:
        SetStandardColors();
        ::InvalidateRect(hWnd, NULL, TRUE);
        ::DeleteObject(hBrushBackGround);
        hBrushBackGround = CreateSolidBrush ( clrBackGround );

        /*Invalidate the text windows so they repaint */
        ::InvalidateRect(hComputerColor, NULL, TRUE);
        ::InvalidateRect(hComputerMove, NULL, TRUE);
        ::InvalidateRect(hWhosTurn, NULL, TRUE);
        ::InvalidateRect(hClockHuman, NULL, TRUE);
        ::InvalidateRect(hClockComputer, NULL, TRUE);
        ::InvalidateRect(hMsgComputer, NULL, TRUE);
        ::InvalidateRect(hMsgHuman, NULL, TRUE);
        break;
    case IDM_TIMECONTROL:
        if (TimeControlDialog(hWnd, hInstance(), wParam))
        {
            TCflag = (TCmoves>1);

            if (TCflag)
            {
            }
            SetTimeControl();
        }
        break;
    case MSG_HELP_INDEX:
    {
        TCHAR szHelpFileName[EXE_NAME_MAX_SIZE+1];
        MakeHelpPathName(szHelpFileName);
        ::WinHelp(hWnd,szHelpFileName,HELP_INDEX,0L);
    }
        break;
    case MSG_HELP_HELP:
        ::WinHelp(hWnd, TEXT("WINHELP.HLP"), HELP_INDEX, 0L);
        break;
    }
}

void MainWindow::_userMoveProc(HWND hwnd)
{
    if (flag.bothsides && !flag.mate)
    {
        SelectMove(hwnd, opponent, 1);
        if (flag.beep)
            MessageBeep(0);

        ::PostMessage(hwnd, MSG_COMPUTER_MOVE, NULL, (long)NULL);
    }
    else if (!flag.mate)
    {
        User_Move = TRUE;
        ft = 0;
        player = opponent;
        ShowSidetoMove();

        /* Set up to allow computer to think while user takes move*/
        int tmp;
        unsigned short mv;
        TCHAR s[10];
        if ( hint>0 && !flag.easy /*&& Book == NULL*/ )
        {
#ifndef WINCE
            time0 = time(NULL);
#endif
            algbr ( hint>>8, hint&0xff, false);
            lstrcpy(s, mvstr[0]);
            tmp = epsquare;

            if (VerifyMove(hwnd, s,1, &mv))
            {
                SelectMove(hwnd, computer, 2);
                VerifyMove(hwnd, mvstr[0], 2, &mv);

                if (Sdepth > 0)
                    Sdepth --;
            }
#ifndef WINCE
            ft = time(NULL) - time0;
#endif
            epsquare = tmp;
        }
    }
}

void MainWindow::_createProc(HWND hWnd, LPCTSTR appName)
{
    int xchar, ychar;
    GetStartupColors(appName);
    hBrushBackGround = ::CreateSolidBrush(clrBackGround);

    for (int i = pawn; i < pawn + 6; i++ )
    {
        pieces[i].piece = LoadBitmap(hInstance(), MAKEINTRESOURCE(PAWNBASE+i));
        pieces[i].mask = LoadBitmap(hInstance(), MAKEINTRESOURCE(PAWNBASE+6+i));
        pieces[i].outline = LoadBitmap(hInstance(), MAKEINTRESOURCE(PAWNBASE+12+i));
    }

    HDC hDC = GetDC(hWnd);
    TEXTMETRIC tm;
    GetTextMetrics ( hDC, &tm);
    xchar = tm.tmMaxCharWidth;
    ychar = tm.tmHeight+tm.tmExternalLeading;

    /*Autosize main window */
    POINT point;
    QueryBoardSize(&point);

    SetWindowPos(hWnd, hWnd, 0,0,
         point.x+GetSystemMetrics(SM_CXFRAME)*2+50,
         point.y+GetSystemMetrics(SM_CYFRAME)*2+GetSystemMetrics(SM_CYMENU)+
         GetSystemMetrics(SM_CYCAPTION) + ychar,
         SWP_NOMOVE | SWP_NOZORDER);

    ::ReleaseDC(hWnd, hDC);
    InitHitTest();
    Create_Children(hWnd, hInstance(), xchar, ychar);
}
