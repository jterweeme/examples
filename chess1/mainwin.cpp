//mainwin.cpp
//Chess main window

#include "mainwin.h"
#include "globals.h"
#include "resource.h"
#include "sim.h"
#include "winclass.h"
#include "toolbox.h"
#include "palette.h"
#include "board.h"
#include "hittest.h"
#include "book.h"
#include "dialog.h"
#include "menubar.h"

MainWindow *MainWindow::_instance = 0;

MainWindow::MainWindow(WinClass *wc, Sim *sim, HACCEL hAccel)
    : _wc(wc), _sim(sim), coords(1), _hAccel(hAccel)
{
    _instance = this;
    _firstSq = -1;
    _gotFirst = FALSE;
    _editActive = FALSE;
    User_Move = TRUE;
}

HWND MainWindow::hComputerColor() const
{
    return _hComputerColor;
}

void MainWindow::GiveHint(HWND hWnd)
{
    TCHAR s[40];
    algbr(short(::hint >> 8), short(::hint & 0xFF), false);
    ::lstrcpy(s, TEXT("try "));
    ::lstrcat(s, ::mvstr[0]);
    ::MessageBox(hWnd, s, TEXT("Chess"), MB_OK);
}

/*
   Compare the string 's' to the list of legal moves available for the
   opponent. If a match is found, make the move on the board.
*/
int MainWindow::_verifyMove(HWND hWnd, TCHAR *s, short iop, WORD *mv)
{
    static short tempb, tempc, tempsf, tempst;
    static Leaf xnode;

    *mv = 0;

    if (iop == 2)
    {
        _sim->UnmakeMove(opponent, &xnode, &tempb, &tempc, &tempsf, &tempst);
        return false;
    }

    short cnt = 0;
    _sim->MoveList(opponent, 2);
    short pnt = TrPnt[2];

    while (pnt < TrPnt[3])
    {
        Leaf *node = &Tree[pnt++];
        algbr(node->f, node->t, short(node->flags));

        if (::lstrcmp(s, mvstr[0]) == 0 || ::lstrcmp(s, mvstr[1]) == 0 ||
            ::lstrcmp(s, mvstr[2]) == 0 || ::lstrcmp(s, mvstr[3]) == 0)
        {
            cnt++;
            xnode = *node;
        }
    }

    if (cnt == 1)
    {
        _sim->MakeMove(opponent, &xnode, &tempb, &tempc, &tempsf, &tempst, &INCscore);

        if (_sim->SqAtakd(PieceList[opponent][0], computer))
        {
            _sim->UnmakeMove(opponent, &xnode, &tempb, &tempc, &tempsf, &tempst);
            Toolbox().messageBox(hInstance(), hWnd, IDS_ILLEGALMOVE, IDS_CHESS);
            return false;
        }

        if (iop == 1)
            return true;

        UpdateDisplay(hWnd, _hComputerColor, xnode.f, xnode.t, 0, short(xnode.flags), flag.reverse);

        if (board[xnode.t] == PAWN || xnode.flags & CAPTURE || xnode.flags & CSTLMASK)
        {
            Game50 = GameCnt;
            _sim->ZeroRPT();
        }

        GameList[GameCnt].depth = GameList[GameCnt].score = 0;
        GameList[GameCnt].nodes = 0;
        ElapsedTime(1, ExtraTime, ResponseTime, ::ft);
        GameList[GameCnt].time = short(et);
        TimeControl.clock[opponent] -= et;
        --TimeControl.moves[opponent];
        *mv = xnode.f << 8 | xnode.t;
        algbr(xnode.f, xnode.t, false);
        return true;
    }

    if (cnt > 1)
        Toolbox().messageBox(hInstance(), hWnd, IDS_AMBIGUOUSMOVE, IDS_CHESS);

    return false;
}



void MainWindow::create(LPCTSTR caption)
{
#ifdef WINCE
    CONSTEXPR DWORD WINSTYLE = WS_CLIPCHILDREN;
#else
    CONSTEXPR DWORD WINSTYLE = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
#endif

    _hwnd = CreateWindow(_wc->className(), caption, WINSTYLE,
                    CW_USEDEFAULT, CW_USEDEFAULT,
                    CW_USEDEFAULT, CW_USEDEFAULT,
                    NULL, NULL, _wc->hInstance(), NULL);

    if (!_hwnd)
        throw TEXT("Cannot create main window");
}

void MainWindow::_createChildren(HWND hWnd, HINSTANCE hInst, short xchar, short ychar)
{
    static TCHAR lpStatic[] = TEXT("Static");

    /* Get the location of lower left conor of client area */
    POINT pt;
    Board::QueryBoardSize(&pt);
    CONSTEXPR DWORD style1 = WS_CHILD | SS_CENTER | WS_VISIBLE;
    _hComputerColor = CreateWindow(lpStatic, NULL, style1, 0, pt.y, 10 * xchar, ychar, hWnd, HMENU(1000), hInst, NULL);
    hWhosTurn = CreateWindow(lpStatic, NULL, style1, 10 * xchar, pt.y, 10 * xchar, ychar, hWnd, HMENU(1001), hInst, NULL);
    CONSTEXPR DWORD style2 = WS_CHILD | SS_LEFT | WS_VISIBLE;
    hComputerMove = CreateWindow(lpStatic, NULL, style2, 375, 10, 10 * xchar, ychar, hWnd, HMENU(1003), hInst, NULL);
    hClockComputer = CreateWindow(lpStatic, NULL, style1, 390, 55, 6 * xchar, ychar, hWnd, HMENU(1010), hInst, NULL);
    hClockHuman = CreateWindow(lpStatic, NULL, style1, 390, 55 + 3 * ychar, 6 * xchar, ychar, hWnd, HMENU(1011), hInst, NULL);
    hMsgComputer = CreateWindow(lpStatic, TEXT("Black:"), style1, 390, 55 - 3 * ychar / 2, 6 * xchar, ychar, hWnd, HMENU(1020), hInst, NULL);

    _hMsgHuman = CreateWindow(lpStatic, TEXT("White:"), WS_CHILD | SS_CENTER | WS_VISIBLE,
                         390, 55 + 3 * ychar / 2, 6 * xchar, ychar,
                         hWnd, HMENU(1021), hInst, NULL);
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

    if ((nFileNameLen + 13) < EXE_NAME_MAX_SIZE)
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
        Board::QuerySqOrigin(_firstSq % 8, _firstSq / 8, &pt);
        rect.left = pt.x;
        rect.right = pt.x + 48;
        rect.top = pt.y - 48;
        rect.bottom = pt.y;
        InvalidateRect(hWnd, &rect, FALSE);
    }

    PAINTSTRUCT ps;
    HDC hDC = ::BeginPaint(hWnd, &ps);
    _board->Draw_Board(hDC, flag.reverse, _clrBlackSquare, _clrWhiteSquare);

    if (coords)
        _board->DrawCoords(hDC, flag.reverse, _clrBackGround, _clrText);

    DrawAllPieces(hDC, _pieces, flag.reverse, boarddraw,
                  colordraw, _clrBlackPiece, _clrWhitePiece);

    ::EndPaint(hWnd, &ps);

    if (_firstSq != -1)
        _board->HiliteSquare(hWnd, _firstSq);
}

void MainWindow::_setStandardColors()
{
    _clrBackGround = Palette::BROWN;
    _clrBlackSquare = Palette::DARKGREEN;
    _clrWhiteSquare = Palette::PALEGRAY;
    _clrBlackPiece = Palette::RED;
    _clrWhitePiece = Palette::CWHITE;
    _clrText = Palette::CBLACK;
}

void MainWindow::_commandProc(HWND hWnd, WPARAM wParam)
{
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
        _sim->NewGame(hWnd, _hComputerColor);

        if (hBook)
            FreeBook();

        GetOpenings(hInstance());
        break;
    case MSG_CHESS_ABOUT:
    {
        AboutDlg aboutDlg(hInstance());
        aboutDlg.run(hWnd);
    }
        break;
#ifndef WINCE
    case MSG_CHESS_EDIT:
    {
        _editActive = TRUE;
        HMENU hMenu = CreateMenu();
        ::AppendMenu(hMenu, MF_STRING, MSG_CHESS_EDITDONE, TEXT("&Done"));
        hMainMenu = GetMenu(hWnd);
        ::SetMenu(hWnd, hMenu);
        DrawMenuBar(hWnd);
    }
        break;
    case MSG_CHESS_EDITDONE:
    {
        _editActive = FALSE;
        HMENU hMenu = GetMenu(hWnd);
        SetMenu(hWnd, hMainMenu);
        DrawMenuBar(hWnd);
        DestroyMenu(hMenu);
        GameCnt = 0;
        Game50 = 1;
        _sim->ZeroRPT();
        Sdepth = 0;
        _sim->InitializeStats();
        ::PostMessage(hWnd, MSG_USER_MOVE, 0, 0);
    }
        break;
#endif
    case MSG_CHESS_REVIEW:
    {
        ReviewDialog revDlg(hInstance());
        revDlg.run(hWnd);
    }
        break;
    case MSG_CHESS_TEST:
    {
        TestDlg testDlg(hInstance(), _sim);
        testDlg.run(hWnd);
    }
        break;
    case MSG_CHESS_HASH:
        flag.hash = !flag.hash;
        break;
    case MSG_CHESS_BEEP:
        flag.beep = !flag.beep;
        break;
    case MSG_CHESS_COORD:
        coords = !coords;
        UpdateDisplay(hWnd, _hComputerColor, 0, 0, 1, 0, flag.reverse);
        break;
    case MSG_CHESS_BOTH:
        flag.bothsides = !flag.bothsides;
        flag.easy = true;
        Sdepth = 0;
        ::PostMessage(hWnd, MSG_USER_MOVE, 0, 0);
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
        ::LoadString(hInstance(), IDS_SETAWIN, str, sizeof(str));
        NumDlg numDlg(hInstance());
        short ret = numDlg.getInt(hWnd, str, _sim->aWindow());
        _sim->aWindow(ret);
    }
        break;
    case MSG_CHESS_BWIN:
    {
        TCHAR str[40];
        ::LoadString(hInstance(), IDS_SETBWIN, str, sizeof(str));
        NumDlg numDlg(hInstance());
        short ret = numDlg.getInt(hWnd, str, _sim->bWindow());
        _sim->bWindow(ret);
    }
        break;
    case MSG_CHESS_CONTEMP:
    {
        TCHAR str[40];
        LoadString(hInstance(), IDS_SETCONTEMPT, str, sizeof(str));
        NumDlg numDlg(hInstance());
        contempt = numDlg.getInt(hWnd, str, contempt);
    }
        break;
    case MSG_CHESS_FORCE:
        flag.force = !flag.force;
        player = opponent;
        ShowPlayers(_hComputerColor);
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
        ::LoadString(hInstance(), IDS_MAXSEARCH, str, sizeof(str));
        NumDlg numDlg(hInstance());
        short ret = numDlg.getInt(hWnd, str, _sim->maxSearchDepth());
        _sim->maxSearchDepth(ret);
    }
        break;
    case MSG_CHESS_REVERSE:
        flag.reverse = !flag.reverse;
        ShowPlayers(_hComputerColor);
        UpdateDisplay(hWnd, _hComputerColor, 0, 0, 1, 0, flag.reverse);
        break;
    case MSG_CHESS_SWITCH:
        ::computer = otherside[computer];
        ::opponent = otherside[opponent];
        ::flag.force = false;
        ::Sdepth = 0;
        ShowPlayers(_hComputerColor);
        ::PostMessage(hWnd, MSG_COMPUTER_MOVE, 0, 0);
        break;
    case MSG_CHESS_BLACK:
        ::computer = black;
        ::opponent = white;
        ::flag.force = false;
        ::Sdepth = 0;
        ShowPlayers(_hComputerColor);
        ::PostMessage(hWnd, MSG_COMPUTER_MOVE, 0, 0);
        break;
    case MSG_CHESS_WHITE:
        computer = white;
        opponent = black;
        flag.force = false;
        Sdepth = 0;
        ShowPlayers(_hComputerColor);
        ::PostMessage(hWnd, MSG_COMPUTER_MOVE, 0, 0);
        break;
    case IDM_BACKGROUND:
    {
        ColorDlg colorDlg(hInstance(), IDM_BACKGROUND, &_clrBackGround);

        if (colorDlg.run(hWnd, IDM_BACKGROUND))
        {
            ::InvalidateRect(hWnd, NULL, TRUE);
            ::DeleteObject(_hBrushBackGround);
            _hBrushBackGround = ::CreateSolidBrush(_clrBackGround);
            ::InvalidateRect(_hComputerColor, NULL, TRUE);
            ::InvalidateRect(hComputerMove, NULL, TRUE);
            ::InvalidateRect(hWhosTurn, NULL, TRUE);
            ::InvalidateRect(hClockHuman, NULL, TRUE);
            ::InvalidateRect(hClockComputer, NULL, TRUE);
            ::InvalidateRect(hMsgComputer, NULL, TRUE);
            ::InvalidateRect(_hMsgHuman, NULL, TRUE);
        }
    }
        break;
    case IDM_BLACKSQUARE:
    {
        ColorDlg colorDlg(hInstance(), IDM_BLACKSQUARE, &_clrBlackSquare);

        if (colorDlg.run(hWnd, IDM_BLACKSQUARE))
            ::InvalidateRect(hWnd, NULL, TRUE);
    }
        break;
    case IDM_WHITESQUARE:
    {
        ColorDlg colorDlg(hInstance(), IDM_WHITESQUARE, &_clrWhiteSquare);

        if (colorDlg.run(hWnd, IDM_WHITESQUARE))
            ::InvalidateRect(hWnd, NULL, TRUE);
    }
        break;
    case IDM_BLACKPIECE:
    {
        ColorDlg colorDlg(hInstance(), IDM_BLACKPIECE, &_clrBlackPiece);

        if (colorDlg.run(hWnd, IDM_BLACKPIECE))
            ::InvalidateRect(hWnd, NULL, TRUE);
    }
        break;
    case IDM_WHITEPIECE:
    {
        ColorDlg colorDlg(hInstance(), IDM_WHITEPIECE, &_clrWhitePiece);

        if (colorDlg.run(hWnd, IDM_WHITEPIECE))
            ::InvalidateRect(hWnd, NULL, TRUE);
    }
        break;
    case IDM_TEXT:
    {
        ColorDlg colorDlg(hInstance(), IDM_TEXT, &_clrText);

        if (colorDlg.run(hWnd, IDM_TEXT))
        {
            ::InvalidateRect(hWnd, NULL, TRUE);
            ::InvalidateRect(_hComputerColor, NULL, TRUE);
            ::InvalidateRect(hComputerMove, NULL, TRUE);
            ::InvalidateRect(hWhosTurn, NULL, TRUE);
            ::InvalidateRect(hClockHuman, NULL, TRUE);
            ::InvalidateRect(hClockComputer, NULL, TRUE);
            ::InvalidateRect(hMsgComputer, NULL, TRUE);
            ::InvalidateRect(_hMsgHuman, NULL, TRUE);
        }
    }
        break;
    case IDM_DEFAULT:
        _setStandardColors();
        ::InvalidateRect(hWnd, NULL, TRUE);
        ::DeleteObject(_hBrushBackGround);
        _hBrushBackGround = ::CreateSolidBrush(_clrBackGround);
        ::InvalidateRect(_hComputerColor, NULL, TRUE);
        ::InvalidateRect(hComputerMove, NULL, TRUE);
        ::InvalidateRect(hWhosTurn, NULL, TRUE);
        ::InvalidateRect(hClockHuman, NULL, TRUE);
        ::InvalidateRect(hClockComputer, NULL, TRUE);
        ::InvalidateRect(hMsgComputer, NULL, TRUE);
        ::InvalidateRect(_hMsgHuman, NULL, TRUE);
        break;
    case IDM_TIMECONTROL:
    {
        TimeCtrlDlg timeCtrlDlg(hInstance());

        if (timeCtrlDlg.run(hWnd, wParam))
        {
            ::TCflag = ::TCmoves > 1;
            SetTimeControl(::ft);
        }
    }
        break;
    case MSG_HELP_INDEX:
        break;
    case MSG_HELP_HELP:
        break;
    }
}

void MainWindow::_userMoveProc(HWND hwnd)
{
    if (flag.mate)
        return;

    if (flag.bothsides)
    {
        _sim->SelectMove(hInstance(), hwnd, _hComputerColor, opponent, 1,
                   _sim->maxSearchDepth(), ::ft);

        if (flag.beep)
            MessageBeep(0);

        ::PostMessage(hwnd, MSG_COMPUTER_MOVE, 0, 0);
        return;
    }

    User_Move = TRUE;
    ::ft = 0;
    player = opponent;
    ShowSidetoMove();
}

void MainWindow::_getStartupColors()
{
    _setStandardColors();

    //TODO: *.ini file lezen
}

void MainWindow::_createProc(HWND hWnd)
{
#ifdef WINCE
    _menuBar = new MenuBarCE(hInstance(), IDM_MAIN);
#else
    _menuBar = new MenuBar(hInstance(), IDM_MAIN);
#endif
    _menuBar->enable(hWnd);
    _board = new Board;
    _pieces = new PIECEBITMAP[7];
    _getStartupColors();
    _hBrushBackGround = ::CreateSolidBrush(_clrBackGround);

    //Dit is verwarrend: de bitmap resources hebben namelijk nummers
    for (int i = PAWN; i < PAWN + 6; ++i)
    {
        _pieces[i].piece = ::LoadBitmap(hInstance(), MAKEINTRESOURCE(PAWNBASE + i));
        _pieces[i].mask = ::LoadBitmap(hInstance(), MAKEINTRESOURCE(PAWNBASE + 6 + i));
        _pieces[i].outline = ::LoadBitmap(hInstance(), MAKEINTRESOURCE(PAWNBASE + 12 + i));
    }

    HDC hDC = ::GetDC(hWnd);
    TEXTMETRIC tm;
    ::GetTextMetrics(hDC, &tm);
    int xchar = tm.tmMaxCharWidth;
    int ychar = tm.tmHeight + tm.tmExternalLeading;

    /*Autosize main window */
    POINT point;
    Board::QueryBoardSize(&point);
#ifdef WINCE
    const int w = point.x + 1 * 2 + 50;
    const int h = point.y + 1 * 2 + GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYCAPTION) + ychar;
#else
    const int w = point.x + GetSystemMetrics(SM_CXFRAME) * 2 + 50;
    const int h = point.y + GetSystemMetrics(SM_CYFRAME) * 2 + GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYCAPTION) + ychar;
#endif
    ::SetWindowPos(hWnd, hWnd, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER);
#ifdef WINCE
    _hitTest = new HitTestCE();
#else
    _hitTest = new HitTest();
#endif
    _hitTest->init(hDC);
    ::ReleaseDC(hWnd, hDC);
    _createChildren(hWnd, hInstance(), xchar, ychar);
}

void MainWindow::_entryPoint(HWND hWnd)
{
    TCHAR str[10];
    ::lstrcpy(str, mvstr[0]);
    WORD mv;
    int temp = _verifyMove(hWnd, str, 0, &mv);

    if (!temp)
    {
        ::PostMessage(hWnd, MSG_USER_MOVE, 0, 0);
        return;
    }

    ElapsedTime(1, ExtraTime, ResponseTime, ::ft);

    if (flag.force)
    {
        computer = opponent;
        opponent = otherside[computer];
    }

    if (mv != hint)
    {
        Sdepth = 0;
        ::ft = 0;
    }

    ::PostMessage(hWnd, MSG_COMPUTER_MOVE, 0, 0);
}

void MainWindow::_lButtonDownProc(HWND hWnd, LPARAM lParam)
{
    /* If computer is thinking on human's time stop it at the first
        button click.  add test to ensure that "human" can't interupt
        the computer from thinking through its turn */
    if (User_Move)
    {
        ::flag.timeout = true;
        ::flag.bothsides = false;
    }

    /* Don't continue unless reason to */
    if (!(_editActive || User_Move))
        return;

    POINT point;
    point.x = LOWORD(lParam);
    point.y = HIWORD(lParam);

    //HDC tijdelijk om te testen
    HDC hdc = GetDC(hWnd);
    int hit = _hitTest->test(point.x, point.y, hdc);
    ReleaseDC(hWnd, hdc);

    if (hit == -1 )
    {
        if (_firstSq != -1)
        {
            _board->UnHiliteSquare(hWnd, _firstSq);
            _gotFirst = FALSE;
            _firstSq = -1;
        }
        return;
    }

    if (_gotFirst)
    {
        _board->UnHiliteSquare(hWnd, _firstSq);
        _gotFirst = FALSE;

        if (_editActive == TRUE)
            ::PostMessage(hWnd, MSG_EDITBOARD, _firstSq << 8 | hit, 0);
        else if (User_Move == TRUE)
            ::PostMessage(hWnd, MSG_USER_ENTERED_MOVE, _firstSq << 8 | hit, 0);

        _firstSq = -1;
    }
    else
    {
        _gotFirst = TRUE;
        _firstSq = hit;
        _board->HiliteSquare(hWnd, hit);
    }
}

LRESULT
MainWindow::_wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    //POINT point;

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
        ::DeleteObject(_hBrushBackGround);
        _hitTest->destroy();
        delete _hitTest;

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
#ifndef WINCE
        ::UnrealizeObject(_hBrushBackGround);
#endif
        ::SetBkColor(hdc, _clrBackGround);
        ::SetBkMode(hdc, TRANSPARENT);
        ::SetTextColor(hdc, _clrText);
        POINT point;
        point.x = point.y = 0;
        ::ClientToScreen(hWnd, &point);
        ::SetBrushOrgEx(hdc, point.x, point.y, 0);
    }
        return LRESULT(_hBrushBackGround);
    case WM_ERASEBKGND:
    {
#ifndef WINCE
        ::UnrealizeObject(HGDIOBJ(_hBrushBackGround));
#endif
        RECT rect;
        ::GetClientRect(hWnd, &rect);
        ::FillRect(HDC(wParam), &rect, _hBrushBackGround);
    }
        return 1;
    case WM_INITMENUPOPUP:
        if (!_editActive)
            _initMenus(wParam, lParam);

        break;
    case WM_LBUTTONDOWN:
        _lButtonDownProc(hWnd, lParam);
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
            First = wParam >> 8 & 0xff;
            Square = wParam & 0xff;
        }

        board[Square] = board[First];
        color[Square] = color[First];
        board[First] = NO_PIECE;
        color[First] = NEUTRAL;
        UpdateDisplay(hWnd, _hComputerColor, First, Square, false, false, flag.reverse);
    }
        break;
    case MSG_USER_MOVE:
        _userMoveProc(hWnd);
        break;
    case MSG_USER_ENTERED_MOVE:
    {
        int Square, First;
        int algbr_flag;
        User_Move = FALSE;

        /* Fix coord's if user "reversed" board */
        if (flag.reverse)
        {
            First = 63 - (wParam >> 8 & 0xff);
            Square  = 63 - (wParam & 0xff);
        }
        else
        {
            First = wParam >> 8 & 0xff;
            Square  = wParam & 0xff;
        }

        PromoteDlg promoteDlg(hInstance());

        /* Logic to allow selection for pawn promotion */
        if (board[First] == PAWN && (Square < 8 || Square > 55))
        {
            algbr_flag = PROMOTE + int(promoteDlg.run(hWnd));
        }
        else
        {
            algbr_flag = 0;
        }

        algbr(First, Square, algbr_flag);
        _entryPoint(hWnd);
    }
        break;
    case MSG_MANUAL_ENTRY_POINT:
        _entryPoint(hWnd);
        break;
    case MSG_COMPUTER_MOVE:
        if (!(flag.quit || flag.mate || flag.force))
        {
            _sim->SelectMove(hInstance(), hWnd, _hComputerColor, computer, 1,
                       _sim->maxSearchDepth(), ::ft);

            if (flag.beep)
                ::MessageBeep(0);
        }
        ::PostMessage(hWnd, MSG_USER_MOVE, 0, 0);
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
                _board->UnHiliteSquare(hWnd, _firstSq);
                _gotFirst = FALSE;
                _firstSq = -1;
            }

            ManualDlg manualDlg(hInstance());

            if (manualDlg.run(hWnd, tmpmove))
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
        _commandProc(hWnd, wParam);
        break;
    default:
        return ::DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK MainWindow::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (_instance)
        return _instance->_wndProc(hwnd, msg, wParam, lParam);
    return 0;
}

void MainWindow::QuerySqCenter(short x, short y, POINT *pptl)
{
    POINT aptl[4];
    Board::QuerySqCoords(x, y, aptl);
    pptl->x = (aptl[0].x + aptl[1].x + aptl[2].x + aptl[3].x) / 4;
    pptl->y = (aptl[0].y + aptl[2].y) / 2;
}

void MainWindow::PieceOriginFromCenter(POINT *pptl)
{
    pptl->x -= PIECE_XAXIS / 2;
    pptl->y -= PIECE_YAXIS / 2;
}

void MainWindow::QuerySqPieceOrigin(short x, short y, POINT *pptl)
{
    QuerySqCenter(x, y, pptl);
    PieceOriginFromCenter (pptl);
}

/*
   Draw a piece in the specificed point

   Piece_bitmap is a structure with the handles for the mask,
   outline and piece.
*/
void MainWindow::ShowPiece(HDC hdc, POINT *pptl, PIECEBITMAP *Piece_bitmap, COLORREF color)
{
    HBRUSH hOldBrush = HBRUSH(::SelectObject(hdc, ::GetStockObject(BLACK_BRUSH)));
    HPEN hOldPen = HPEN(::SelectObject(hdc, ::GetStockObject(BLACK_PEN)));
    HDC hMemDC = ::CreateCompatibleDC(hdc);

    /* Write the mask to clear the space */
    ::SelectObject(hMemDC, Piece_bitmap->mask);
    ::BitBlt(hdc, pptl->x, pptl->y, PIECE_XAXIS, PIECE_YAXIS, hMemDC, 0, 0,SRCAND);

    /* Write out the piece with an OR */
    HBRUSH hBrush = ::CreateSolidBrush(color);
    ::SelectObject(hdc, hBrush);
    ::SelectObject(hMemDC, Piece_bitmap->piece);
    ::BitBlt(hdc, pptl->x, pptl->y, PIECE_XAXIS, PIECE_YAXIS, hMemDC, 0, 0, 0xB80746L);

    /* The draw the outline */
    ::SelectObject(hdc, GetStockObject(BLACK_BRUSH));
    ::SelectObject(hMemDC, Piece_bitmap->outline);
    ::BitBlt(hdc, pptl->x, pptl->y, PIECE_XAXIS, PIECE_YAXIS, hMemDC, 0, 0, 0xB80746L);
    ::SelectObject(hdc, hOldBrush);
    ::SelectObject(hdc, hOldPen);
    ::DeleteObject(hBrush);

    if (::DeleteDC(hMemDC) == 0)
        ::MessageBeep(0);
}

void MainWindow::DrawOnePiece(HDC hdc, short x, short y, PIECEBITMAP *piece, COLORREF color)
{
    POINT origin;
    QuerySqPieceOrigin(x, y, &origin);
    ShowPiece(hdc, &origin, piece, color);
}

void MainWindow::DrawAllPieces(HDC hDC, PIECEBITMAP *pieces, int reverse, short *pbrd,
                   short *color, COLORREF clrblack, COLORREF clrwhite)
{
    for (short y = 0; y < 8; ++y)
    {
        for (short x = 0; x < 8; ++x)
        {
            short i = ConvertCoordToIndex(x, y);
            short *colori = color + i;

            if (*colori == 2)
                continue;

            COLORREF colorRef = *colori == BLACK ? clrblack : clrwhite;

            if (reverse == 0)
                DrawOnePiece(hDC, x, y, pieces + *(pbrd + i), colorRef);
            else
                DrawOnePiece(hDC, 7 - x, 7 - y, pieces + *(pbrd + i), colorRef);
        }
    }
}

