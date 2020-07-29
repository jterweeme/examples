#include "mainwin.h"
#include "globals.h"
#include "gnuchess.h"
#include "protos.h"
#include "resource.h"
#include "sim.h"
#include "winclass.h"
#include "toolbox.h"
#include "palette.h"
#include "board.h"
#include <ctime>

MainWindow *MainWindow::_instance = 0;

MainWindow::MainWindow(WinClass *wc, Sim *sim) : _wc(wc), _sim(sim)
{
    _instance = this;
    _firstSq = -1;
    _gotFirst = FALSE;
    EditActive = FALSE;
    User_Move = TRUE;
}

static int coords = 1;

#ifdef WINCE
#define WINSTYLE WS_CLIPCHILDREN
#else
#define WINSTYLE WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN
#endif

static void GiveHint(HWND hWnd)
{
    TCHAR s[40];
    algbr(short(hint >> 8), short(hint & 0xFF), false);
    ::lstrcpy(s, TEXT("try "));
    ::lstrcat(s, mvstr[0]);
    ::MessageBox(hWnd, s, TEXT("Chess"), MB_OK);
}

void MainWindow::create(LPCTSTR caption)
{
    _hwnd = CreateWindow(_wc->className(), caption, WINSTYLE,
                    CW_USEDEFAULT, CW_USEDEFAULT,
                    CW_USEDEFAULT, CW_USEDEFAULT,
                    NULL, NULL, _wc->hInstance(), NULL);

    if (!_hwnd)
        throw TEXT("Cannot create main window");
}

void MainWindow::_createChildren(HWND hWnd, HINSTANCE hInst, short xchar, short ychar)
{
    POINT pt;
    static TCHAR lpStatic[] = TEXT("Static");

    /* Get the location of lower left conor of client area */
    QueryBoardSize(&pt);

    hComputerColor = CreateWindow(lpStatic, NULL, WS_CHILD | SS_CENTER | WS_VISIBLE,
                         0, pt.y, 10 * xchar, ychar, hWnd, HMENU(1000), hInst, NULL);

    hWhosTurn = CreateWindow(lpStatic, NULL, WS_CHILD | SS_CENTER | WS_VISIBLE,
                        10*xchar, pt.y, 10*xchar, ychar, hWnd, HMENU(1001), hInst, NULL);

    hComputerMove = CreateWindow(lpStatic, NULL,
                        WS_CHILD | SS_LEFT | WS_VISIBLE,
                        375 /*0*/,
                        10 /*pt.y+(3*ychar)/2*/,
                        10*xchar,
                        ychar,
                        hWnd, HMENU(1003), hInst, NULL);


    hClockComputer = CreateWindow(lpStatic, NULL,
                        WS_CHILD | SS_CENTER | WS_VISIBLE,
                        390, 55, 6*xchar, ychar, hWnd,
                        HMENU(1010), hInst, NULL);

    hClockHuman =  CreateWindow(lpStatic, NULL,
                        WS_CHILD | SS_CENTER | WS_VISIBLE,
                        390, 55+3*ychar, 6*xchar, ychar, hWnd,
                        HMENU(1011), hInst, NULL);

    hMsgComputer = CreateWindow(lpStatic, TEXT("Black:"),
                        WS_CHILD | SS_CENTER | WS_VISIBLE,
                        390, 55-3*ychar/2, 6*xchar, ychar, hWnd,
                        HMENU(1020), hInst, NULL);

    hMsgHuman    = CreateWindow(lpStatic, TEXT("White:"),
                        WS_CHILD | SS_CENTER | WS_VISIBLE,
                        390, 55+3*ychar/2, 6*xchar, ychar, hWnd,
                        HMENU(1021), hInst, NULL);
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

void MainWindow::_makeHelpPathName(TCHAR *szFileName)
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
    if (_firstSq != -1)
    {
        POINT pt;
        RECT rect;
        QuerySqOrigin(_firstSq % 8, _firstSq / 8, &pt);
        rect.left = pt.x;
        rect.right=pt.x+48;
        rect.top = pt.y-48;
        rect.bottom = pt.y;
        InvalidateRect(hWnd, &rect, FALSE);
    }

    PAINTSTRUCT ps;
    HDC hDC = ::BeginPaint(hWnd, &ps);
    _board->Draw_Board(hDC, flag.reverse, clrBlackSquare, clrWhiteSquare);

    if (coords)
        _board->DrawCoords(hDC, flag.reverse, clrBackGround, clrText);

    DrawAllPieces(hDC, flag.reverse, boarddraw, colordraw, clrBlackPiece, clrWhitePiece);
    ::EndPaint(hWnd, &ps);

    if (_firstSq != -1)
        HiliteSquare(hWnd, _firstSq);
}

void MainWindow::_setStandardColors()
{
    clrBackGround = Palette::BROWN;
    clrBlackSquare = Palette::DARKGREEN;
    clrWhiteSquare = Palette::PALEGRAY;
    clrBlackPiece = Palette::RED;
    clrWhitePiece = Palette::CWHITE;
    clrText = Palette::CBLACK;
}

void MainWindow::_commandProc(HWND hWnd, WPARAM wParam)
{
    HMENU hMenu;

    switch (wParam)
    {
    case MSG_CHESS_QUIT:
        ::PostMessage(hWnd, MSG_DESTROY, 0, 0);
        break;
    case MSG_CHESS_HINT:
        GiveHint(hWnd);
        break;
    case MSG_CHESS_LIST:
        break;
    case MSG_CHESS_SAVE:
        break;
    case MSG_CHESS_GET:
        break;
    case MSG_CHESS_NEW:
        _sim->NewGame(hWnd);

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
        if (Book != NULL)
        {
            FreeBook();
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
            ShowPlayers();
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
            ShowPlayers();
            UpdateDisplay(hWnd,0,0,1,0);
            break;
       case MSG_CHESS_SWITCH:
            computer = otherside[computer];
            opponent = otherside[opponent];
            flag.force = false;
            Sdepth = 0;
            ShowPlayers();
            ::PostMessage(hWnd, MSG_COMPUTER_MOVE, NULL, (long)NULL);
            break;
       case MSG_CHESS_BLACK:
            computer = black;
            opponent = white;
            flag.force = false;
            Sdepth = 0;
            ShowPlayers();
            ::PostMessage(hWnd, MSG_COMPUTER_MOVE, NULL, (long)NULL);
            break;
       case MSG_CHESS_WHITE:
            computer = white;
            opponent = black;
            flag.force = false;
            Sdepth = 0;
            ShowPlayers();
            ::PostMessage(hWnd, MSG_COMPUTER_MOVE, NULL, (long)NULL);
            break;
       case IDM_BACKGROUND:
            if (ColorDialog(hWnd, hInstance(), wParam))
            {
                InvalidateRect (hWnd, NULL, TRUE);
                DeleteObject(_hBrushBackGround);
                _hBrushBackGround = ::CreateSolidBrush(clrBackGround);

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
                ::InvalidateRect(hWnd, NULL, TRUE);
            }
            break;
    case IDM_TEXT:
        if (ColorDialog(hWnd, hInstance(), wParam))
        {
            /*Invalidate the text windows so they repaint */
            ::InvalidateRect(hWnd, NULL, TRUE);
            ::InvalidateRect(hComputerColor, NULL, TRUE);
            ::InvalidateRect(hComputerMove, NULL, TRUE);
            ::InvalidateRect(hWhosTurn, NULL, TRUE);
            ::InvalidateRect(hClockHuman, NULL, TRUE);
            ::InvalidateRect(hClockComputer, NULL, TRUE);
            ::InvalidateRect(hMsgComputer, NULL, TRUE);
            ::InvalidateRect(hMsgHuman, NULL, TRUE);
        }
        break;
    case IDM_DEFAULT:
        _setStandardColors();
        ::InvalidateRect(hWnd, NULL, TRUE);
        ::DeleteObject(_hBrushBackGround);
        _hBrushBackGround = ::CreateSolidBrush(clrBackGround);

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
            TCflag = TCmoves > 1;

            if (TCflag)
            {
            }
            SetTimeControl();
        }
        break;
    case MSG_HELP_INDEX:
    {
        TCHAR szHelpFileName[EXE_NAME_MAX_SIZE+1];
        _makeHelpPathName(szHelpFileName);
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

        if (hint > 0 && !flag.easy /*&& Book == NULL*/ )
        {
            time0 = time(NULL);
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
            ft = time(NULL) - time0;
            epsquare = tmp;
        }
    }
}

void MainWindow::_getStartupColors()
{
    _setStandardColors();

    //TODO: *.ini file lezen
}

void MainWindow::_createProc(HWND hWnd)
{
    _board = new Board;
    _getStartupColors();
    _hBrushBackGround = ::CreateSolidBrush(clrBackGround);

    //Dit is verwarrend: de bitmap resources hebben namelijk nummers
    for (int i = pawn; i < pawn + 6; i++ )
    {
        pieces[i].piece = ::LoadBitmap(hInstance(), MAKEINTRESOURCE(PAWNBASE+i));
        pieces[i].mask = ::LoadBitmap(hInstance(), MAKEINTRESOURCE(PAWNBASE+6+i));
        pieces[i].outline = ::LoadBitmap(hInstance(), MAKEINTRESOURCE(PAWNBASE+12+i));
    }

    HDC hDC = ::GetDC(hWnd);
    TEXTMETRIC tm;
    ::GetTextMetrics(hDC, &tm);
    int xchar = tm.tmMaxCharWidth;
    int ychar = tm.tmHeight + tm.tmExternalLeading;

    /*Autosize main window */
    POINT point;
    QueryBoardSize(&point);

    ::SetWindowPos(hWnd, hWnd, 0,0,
         point.x+GetSystemMetrics(SM_CXFRAME)*2+50,
         point.y+GetSystemMetrics(SM_CYFRAME)*2+GetSystemMetrics(SM_CYMENU)+
         GetSystemMetrics(SM_CYCAPTION) + ychar,
         SWP_NOMOVE | SWP_NOZORDER);

    ::ReleaseDC(hWnd, hDC);
    InitHitTest();
    _createChildren(hWnd, hInstance(), xchar, ychar);
}

LRESULT
MainWindow::_wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    POINT point;

    switch (message)
    {
    case WM_CREATE:
        _createProc(hWnd);
        break;
    case MSG_DESTROY:
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
    {
        TCHAR szHelpFileName[EXE_NAME_MAX_SIZE + 1];
        _makeHelpPathName(szHelpFileName);
        ::WinHelp(hWnd, szHelpFileName, HELP_QUIT, 0L);
        ::DeleteObject(_hBrushBackGround);
        Hittest_Destructor();

        if (hBook)
            FreeBook();

        _sim->FreeGlobals();
#if 0
        SaveColors(TEXT("Chess"));
#endif
        ::PostQuitMessage(0);
    }
        break;
    case WM_PAINT:
        _paintProc(hWnd);
        break;
    case WM_CTLCOLORSTATIC:
    {
        HDC hdc = HDC(wParam);
        ::UnrealizeObject(_hBrushBackGround);
        ::SetBkColor(hdc, clrBackGround);
        ::SetBkMode(hdc, TRANSPARENT);
        ::SetTextColor(hdc, clrText);
        POINT point;
        point.x = point.y = 0;
        ::ClientToScreen(hWnd, &point);
        ::SetBrushOrgEx(hdc, point.x, point.y, 0);
    }
        return LRESULT(_hBrushBackGround);
    case WM_ERASEBKGND:
    {
        RECT rect;
        ::UnrealizeObject(HGDIOBJ(_hBrushBackGround));
        ::GetClientRect(hWnd, &rect);
        ::FillRect(HDC(wParam), &rect, _hBrushBackGround);
    }
        return 1;
    case WM_INITMENUPOPUP:
        if (!EditActive)
            _initMenus(wParam, lParam);

        break;
    case WM_LBUTTONDOWN:
    {
        /* If computer is thinking on human's time stop it at the first
            button click.  add test to ensure that "human" can't interupt
            the computer from thinking through its turn */
        if (User_Move)
        {
            flag.timeout = true;
            flag.bothsides = false;
        }

        /* Don't continue unless reason to */
        if (!(EditActive || User_Move))
            break;

        point.x = LOWORD(lParam);
        point.y = HIWORD(lParam);
        int Hit = HitTest(point.x, point.y);

        if (Hit == -1 )
        {
            if (_firstSq != -1)
            {
                UnHiliteSquare(hWnd, _firstSq);
                _gotFirst = FALSE;
                _firstSq = -1;
            }
            break;
        }

        if (_gotFirst)
        {
            UnHiliteSquare(hWnd, _firstSq);
            _gotFirst = FALSE;

            if (EditActive == TRUE)
                ::PostMessage(hWnd, MSG_EDITBOARD, _firstSq << 8 | Hit, NULL);
            else if (User_Move == TRUE)
                ::PostMessage(hWnd, MSG_USER_ENTERED_MOVE, _firstSq << 8 | Hit, NULL);

            _firstSq = -1;
        }
        else
        {
            _gotFirst = TRUE;
            _firstSq = Hit;
            HiliteSquare ( hWnd, Hit);
        }
    }
        break;
    case MSG_EDITBOARD:
    {
        int Square, First;

        if (flag.reverse)
        {
            First = 63 - (wParam >> 8 & 0xff);
            Square  = 63 - (wParam & 0xff);
        }
        else
        {
            First = (wParam>>8) & 0xff;
            Square  = wParam & 0xff;
        }

         board[Square] = board[First];
         color[Square] = color[First];
         board[First] = no_piece;
         color[First] = neutral;
         UpdateDisplay(hWnd, First, Square, false, false);
    }
        break;
    case MSG_USER_MOVE:
        _userMoveProc(hWnd);
        break;
    case MSG_USER_ENTERED_MOVE:
    {
        int temp;
        WORD mv;
         int Square,First;
         TCHAR str[10];
         int algbr_flag;
         User_Move = FALSE;

         /* Fix coord's if user "reversed" board */
         if (flag.reverse)
         {
            First = 63 - ((wParam>>8) & 0xff);
            Square  = 63 - (wParam & 0xff);
         } else {
            First = (wParam>>8) & 0xff;
            Square  = wParam & 0xff;
         }

         /* Logic to allow selection for pawn promotion */
         if ((board[First] == pawn) && ((Square <8) || (Square>55)))
         {
            algbr_flag = PROMOTE + PromoteDialog(hWnd, hInstance());
         }
         else
         {
             algbr_flag = 0;
         }
         algbr( First, Square, algbr_flag);

      /* Entry point for manual entry of move */
      case MSG_MANUAL_ENTRY_POINT:
          ::lstrcpy(str, mvstr[0]);

            temp = VerifyMove(hWnd, str, 0, &mv);
            if (!temp)
                PostMessage(hWnd, MSG_USER_MOVE, NULL, (long)NULL);
         else
         {
            ElapsedTime (1);
            if ( flag.force ) {
               computer = opponent;
               opponent = otherside[computer];
            }
            if ( mv != hint) {
               Sdepth = 0;
               ft = 0;
               PostMessage ( hWnd, MSG_COMPUTER_MOVE, NULL, (long)NULL);
            } else {
               PostMessage ( hWnd, MSG_COMPUTER_MOVE, NULL, (long)NULL);
            }
         }
      }
         break;
    case MSG_COMPUTER_MOVE:
        if ( !(flag.quit || flag.mate || flag.force) )
        {
            SelectMove(hWnd, computer, 1);

            if (flag.beep)
                ::MessageBeep(0);
        }
        ::PostMessage(hWnd, MSG_USER_MOVE, NULL, (long)NULL);
        break;
    case WM_CHAR:
        /* Allow Control C to abort bothsides (autoplay) mode */
        /* And abort computer thinking */
        if (wParam == 3)
        {
            flag.timeout = true;
            flag.bothsides = false;
        }
        else if (wParam == VK_ESCAPE)
        {
            ::ShowWindow(hWnd, SW_MINIMIZE);
        }
        break;
    case WM_KEYDOWN:
        if (User_Move && wParam == VK_F2)
        {
            /* To invoke manual move entry */
            TCHAR tmpmove[8];
            flag.timeout = true;
            flag.bothsides = false;

            if (_gotFirst)
            {
                UnHiliteSquare(hWnd, _firstSq);
                _gotFirst = FALSE;
                _firstSq = -1;
            }

            if (DoManualMoveDlg(hInst, hWnd, tmpmove))
            {
                ::lstrcpy(mvstr[0], tmpmove);
                ::PostMessage(hWnd, MSG_MANUAL_ENTRY_POINT, 0, 0);
            }
        }
        break;
    case WM_SYSCOMMAND:
        if (wParam == SC_CLOSE)
        {
            /*Abort easy mode */
            flag.timeout = true;
            flag.bothsides = false;
            ::PostMessage(hWnd, MSG_DESTROY, 0, 0);
            break;
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_CLOSE:
        flag.timeout = true;
        flag.bothsides = false;
        ::PostMessage(hWnd, MSG_DESTROY, 0, 0);
        break;
    case WM_COMMAND:
        /* When we execute a command stop any look ahead */
        /* Then call actual routine to process */
        flag.timeout = true;
        flag.bothsides = false;
        ::PostMessage(hWnd, MSG_WM_COMMAND, wParam, lParam);
        break;
    case MSG_WM_COMMAND:
        try
        {
            _commandProc(hWnd, wParam);
        }
        catch (UINT err)
        {
            Toolbox().messageBox(hInstance(), hWnd, err, IDS_CHESS);
        }
        catch (...)
        {
            Toolbox().messageBox(hInstance(), hWnd, IDS_UNKNOWNERR, IDS_CHESS);
        }

        break;
    default:
        return ::DefWindowProc(hWnd, message, wParam, lParam);
    }
    return NULL;
}

LRESULT CALLBACK MainWindow::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (_instance)
        return _instance->_wndProc(hwnd, msg, wParam, lParam);
    return 0;
}

