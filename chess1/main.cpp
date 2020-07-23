/*
  C source for GNU CHESS

  Revision: 1991-01-20

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

#include "gnuchess.h"
#include "defs.h"
#include "chess.h"
#include "resource.h"
#include "globals.h"
#include "winclass.h"
#include <time.h>

static HBRUSH hBrushBackGround;

TCHAR szAppName[] = TEXT("Chess");

class MainWindow
{
private:
    int FirstSq;         /* Flag is a square is selected */
    int GotFirst;
    int EditActive;   /* Edit mode? */
    int User_Move;     /* User or computer's turn */
    HMENU hMainMenu;
    void MakeHelpPathName(TCHAR *szFileName);
    static MainWindow *_instance;
    LRESULT _wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
public:
    MainWindow();
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

#define EXE_NAME_MAX_SIZE  128

void MainWindow::MakeHelpPathName(TCHAR *szFileName)
{
    TCHAR *pcFileName;

    int nFileNameLen = GetModuleFileName(hInst, szFileName, EXE_NAME_MAX_SIZE);
    pcFileName = szFileName + nFileNameLen;

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
        lstrcat(szFileName, TEXT("chess.hlp"));
    }
    else
    {
        lstrcat(szFileName, TEXT("?"));
    }
}

MainWindow *MainWindow::_instance = 0;

MainWindow::MainWindow()
{
    _instance = this;
	FirstSq = -1;
	GotFirst = FALSE;
	EditActive = FALSE;
	User_Move = TRUE;
}

#ifdef WINCE
#define LPXSTR LPTSTR
#define WINSTYLE WS_CLIPCHILDREN
#else
#define LPXSTR LPSTR
#define WINSTYLE WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN
#endif

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPXSTR lpCmdLine, int nCmdShow)
{
    POINT pt;
    (void)lpCmdLine;
    (void)hPrevInstance;
    WinClass wc(hInstance, MainWindow::wndProc, szAppName);
    MainWindow win;
    wc.registerClass();
    hInst = hInstance;
    QueryBoardSize(&pt);

    //Create the main window.  It will be autosized in WM_CREATE message
    HWND hWnd = CreateWindow(szAppName, szAppName, WINSTYLE,
                    CW_USEDEFAULT, CW_USEDEFAULT,
                    CW_USEDEFAULT, CW_USEDEFAULT,
                    NULL, NULL, hInstance, NULL);

    if (!hWnd)
        return 0;

    ShowWindow(hWnd, nCmdShow);

    /* Initialize chess */
    if (init_main(hWnd))
    {
        SMessageBox(hWnd, IDS_INITERROR, IDS_CHESS);
        FreeGlobals();
        return 0;
    }

    UpdateWindow(hWnd);
    hAccel = LoadAccelerators(hInstance, szAppName);
    player = opponent;
    ShowSidetoMove();

    MSG msg;
    while (GetMessage(&msg, NULL, NULL, NULL))
    {
        if (!TranslateAccelerator(hWnd, hAccel, &msg) )
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return msg.wParam;
}

LRESULT
MainWindow::_wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HMENU hMenu;
    PAINTSTRUCT ps;
    HDC hDC;
    TEXTMETRIC tm;
    POINT point;
#ifndef WINCE
    OFSTRUCT pof;
#endif
    TCHAR FileName[256];
    TCHAR str[80];
    int Status;
    HFILE hFile;

    switch (message)
    {
        case WM_CREATE:
        {
            int xchar, ychar;
            GetStartupColors(szAppName);
            hBrushBackGround = ::CreateSolidBrush(clrBackGround);

            for (int i = pawn; i < pawn + 6; i++ )
            {
                pieces[i].piece = LoadBitmap(hInst, MAKEINTRESOURCE(PAWNBASE+i));
                pieces[i].mask = LoadBitmap(hInst, MAKEINTRESOURCE(PAWNBASE+6+i));
                pieces[i].outline = LoadBitmap(hInst, MAKEINTRESOURCE(PAWNBASE+12+i));
            }

            hDC = GetDC (hWnd);
            GetTextMetrics ( hDC, &tm);
            xchar = tm.tmMaxCharWidth;
            ychar = tm.tmHeight+tm.tmExternalLeading;

            /*Autosize main window */
            QueryBoardSize (&point);
#ifndef WINCE
            SetWindowPos( hWnd, hWnd, 0,0,
                 point.x+GetSystemMetrics(SM_CXFRAME)*2+50,
                 point.y+GetSystemMetrics(SM_CYFRAME)*2+GetSystemMetrics(SM_CYMENU)+
                 GetSystemMetrics(SM_CYCAPTION) + ychar,
                 SWP_NOMOVE | SWP_NOZORDER);
#endif

            ReleaseDC(hWnd, hDC);
            InitHitTest();
            Create_Children(hWnd, hInst, xchar, ychar);
        }
            break;
        case MSG_DESTROY:
            DestroyWindow(hWnd);
            break;
        case WM_DESTROY:
        {
            TCHAR szHelpFileName[EXE_NAME_MAX_SIZE + 1];
            MakeHelpPathName(szHelpFileName);
#ifndef WINCE
            WinHelp(hWnd, szHelpFileName, HELP_QUIT, 0L);
#endif
            DeleteObject(hBrushBackGround);
            Hittest_Destructor();

            if (hBook)
                FreeBook();

            FreeGlobals();
            SaveColors(szAppName);
            PostQuitMessage(0);
        }
            break;
        case WM_PAINT:
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

            hDC = BeginPaint(hWnd, &ps);
            Draw_Board(hDC, flag.reverse, clrBlackSquare, clrWhiteSquare);

            if (coords)
                DrawCoords(hDC, flag.reverse, clrBackGround, clrText);

            DrawAllPieces(hDC, flag.reverse, boarddraw, colordraw, clrBlackPiece, clrWhitePiece );
            EndPaint(hWnd, &ps);

            if (FirstSq != -1)
                HiliteSquare(hWnd, FirstSq);
            break;
        case WM_CTLCOLORSTATIC:
        {
            HDC hdc = HDC(wParam);
            POINT point;
#ifndef WINCE
            UnrealizeObject(hBrushBackGround);
#endif
            SetBkColor(hdc, clrBackGround);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, clrText);
            point.x = point.y = 0;
            ClientToScreen(hWnd, &point);
            SetBrushOrgEx(hdc, point.x, point.y, 0);
            return (LRESULT)(hBrushBackGround);
      }
      case WM_ERASEBKGND:
      {
         RECT rect;
#ifndef WINCE
         UnrealizeObject(HGDIOBJ(hBrushBackGround));
#endif
         GetClientRect(hWnd, &rect);
         FillRect((HDC)wParam, &rect, hBrushBackGround);
         return 1;
      }

      case WM_INITMENUPOPUP:
         if ( !EditActive )
             Init_Menus (hWnd, wParam, lParam);

         break;

      case WM_LBUTTONDOWN:

         /* If computer is thinking on human's time stop it at the first
            button click.  add test to ensure that "human" can't interupt
            the computer from thinking through its turn */
         if ( User_Move )
         {
            flag.timeout = true;
            flag.bothsides = false;
         }

         /* Don't continue unless reason to */
         if ( !(EditActive || User_Move))
             break;

         //point = MAKEPOINT(lParam );
         point.x = LOWORD(lParam);
         point.y = HIWORD(lParam);


      {
         int Hit;

         Hit = HitTest (point.x, point.y );

         if ( Hit == -1 )
         {
            if ( FirstSq != -1)
            {
               UnHiliteSquare ( hWnd, FirstSq);
               GotFirst = FALSE;
               FirstSq = -1;
            }
            break;
         }

         if ( GotFirst )
         {
            UnHiliteSquare( hWnd, FirstSq);
            GotFirst = FALSE;

            if ( EditActive == TRUE)
            {
               PostMessage(hWnd, MSG_EDITBOARD,((FirstSq<<8)|Hit), (LONG) NULL);
            } else if ( User_Move == TRUE) {
               PostMessage(hWnd, MSG_USER_ENTERED_MOVE, ((FirstSq<<8)|Hit), (LONG) NULL);
            }
            FirstSq = -1;
         } else {
            GotFirst = TRUE;
            FirstSq = Hit;
            HiliteSquare ( hWnd, Hit);
         }
      }
         break;

      case MSG_EDITBOARD:
      {
         int Square, First;

         if ( flag.reverse ) {
            First = 63 - ((wParam>>8) & 0xff);
            Square  = 63 - (wParam & 0xff);
         } else {
            First = (wParam>>8) & 0xff;
            Square  = wParam & 0xff;
         }
         
         board[Square] = board[First];
         color[Square] = color[First];

         board[First] = no_piece;
         color[First] = neutral;

         UpdateDisplay (hWnd, First, Square, false, false);
      }
         break;

      case MSG_USER_MOVE:
         if ( flag.bothsides && !flag.mate ) {
            SelectMove ( hWnd, opponent, 1);
            if ( flag.beep ) MessageBeep (0);
            PostMessage ( hWnd, MSG_COMPUTER_MOVE, NULL, (long)NULL);
         } else if (!flag.mate)
         {
            User_Move = TRUE;

            ft = 0;
            player = opponent;
            ShowSidetoMove ();
         {
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
               if ( VerifyMove (hWnd, s,1, &mv) ) {
                  SelectMove ( hWnd, computer, 2);
                  VerifyMove ( hWnd, mvstr[0], 2, &mv);
                  if ( Sdepth>0 ) Sdepth --;
               }
#ifndef WINCE
               ft = time(NULL) - time0;
#endif
               epsquare = tmp;
            }
            }
         }
         break;

      case MSG_USER_ENTERED_MOVE:
      {
         int temp;
         unsigned short mv;
         int Square,First;
         TCHAR str[10];
         int algbr_flag;
         User_Move = FALSE;
/*         player = opponent;*/

         /* Fix coord's if user "reversed" board */
         if ( flag.reverse ) {
            First = 63 - ((wParam>>8) & 0xff);
            Square  = 63 - (wParam & 0xff);
         } else {
            First = (wParam>>8) & 0xff;
            Square  = wParam & 0xff;
         }

         /* Logic to allow selection for pawn promotion */
         if ( (board[First] == pawn) &&( (Square <8) || (Square>55)) ) {
            algbr_flag = promote + PromoteDialog (hWnd, hInst);
         } else algbr_flag = 0;
         algbr ( First, Square, algbr_flag);

      /* Entry point for manual entry of move */
      case MSG_MANUAL_ENTRY_POINT:
         lstrcpy(str, mvstr[0]);
         
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

            if ( flag.beep )
                MessageBeep (0);
         }
         PostMessage ( hWnd, MSG_USER_MOVE, NULL, (long)NULL);
         break;


      case WM_CHAR:
         /* Allow Control C to abort bothsides (autoplay) mode */
         /* And abort computer thinking */
         if ( wParam == 3 ) {
            flag.timeout = true;
            flag.bothsides = false;
         } else if ( wParam == VK_ESCAPE ) {
            ShowWindow ( hWnd, SW_MINIMIZE);
         }
         break;

      case WM_KEYDOWN:
         if ( User_Move && wParam == VK_F2 )
         {
            /* To invoke manual move entry */
            TCHAR tmpmove[8];
            flag.timeout = true;
            flag.bothsides = false;

            if (GotFirst)
			{
               UnHiliteSquare( hWnd, FirstSq);
               GotFirst = FALSE;
               FirstSq = -1;
            }

            if (DoManualMoveDlg(hInst, hWnd, tmpmove))
            {
               lstrcpy(mvstr[0], tmpmove);
               PostMessage(hWnd, MSG_MANUAL_ENTRY_POINT, 0, 0);
            }
         }
         break;

      case WM_SYSCOMMAND:
         if (wParam == SC_CLOSE)
		 {     /*Abort easy mode */
            flag.timeout = true;
            flag.bothsides = false;
            PostMessage ( hWnd, MSG_DESTROY, 0, 0);
         }
		 else
			 return (DefWindowProc(hWnd, message, wParam, lParam));
         break;
      case WM_CLOSE:
         flag.timeout = true;
         flag.bothsides = false;
         PostMessage ( hWnd, MSG_DESTROY, 0, 0);
         break;
      case WM_COMMAND:

         /* When we execute a command stop any look ahead */
         /* Then call actual routine to process */
         flag.timeout = true;
         flag.bothsides = false;
         PostMessage(hWnd, MSG_WM_COMMAND, wParam, lParam);
         break;
      
      case MSG_WM_COMMAND:
         switch (wParam)
		 {
            case MSG_CHESS_QUIT:
                 PostMessage ( hWnd, MSG_DESTROY, 0, 0);
               break;

            case MSG_CHESS_HINT:
               GiveHint (hWnd);
               break;

            case MSG_CHESS_LIST:
#ifndef WINCE
               if (!DoFileSaveDlg(hInst, hWnd, TEXT("chess.lst"), TEXT(".lst"), &Status, FileName, &pof))
                   break;
#endif
               if (Status == 1)
               {
                   lstrcpy(str, TEXT("Replace Existing"));
                   lstrcat(str, FileName);

                    if (::MessageBox(hWnd, str, szAppName, MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL) == IDNO)
                        break;
               }
               else
               {
#ifndef UNICODE
                   OpenFile(FileName, &pof, OF_PARSE);
#endif
               }
#ifndef WINCE
               ListGame(hWnd, pof.szPathName);
#endif
               break;
            case MSG_CHESS_SAVE:
#ifndef WINCE
               if (!DoFileSaveDlg(hInst, hWnd, TEXT("chess.chs"), TEXT(".chs"), &Status, FileName, &pof))
                   break;
#endif
               if (Status == 1)
               {
                    lstrcpy(str, TEXT("Replace Existing"));
                    lstrcat(str, FileName);

                    if (::MessageBox(hWnd, str, szAppName, MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL) == IDNO)
                    {
                        break;
                    }
               }
               else
               {
#ifndef UNICODE
                   OpenFile(FileName, &pof, OF_PARSE);
#endif
               }
#ifndef WINCE
               SaveGame(hWnd, pof.szPathName);
#endif
               break;
            case MSG_CHESS_GET:
#ifndef WINCE
               if (!DoFileOpenDlg(hInst, hWnd, TEXT("*.chs"), TEXT(".chs"), 0x0, FileName, &pof))
                  break;
#ifndef UNICODE
               if ((hFile = OpenFile(FileName, &pof, OF_READ|OF_REOPEN)) == -1)
               {
                  lstrcpy(str, "Cannot open file: ");
                  lstrcat(str, FileName);
                  ::MessageBox(hWnd, str, szAppName, MB_OK | MB_APPLMODAL | MB_ICONQUESTION);
                  break;
               }

               _lclose(hFile);
#endif
               GetGame(hWnd, pof.szPathName);
#endif
               break;
            case MSG_CHESS_NEW:
               NewGame(hWnd);

               if (hBook)
                   FreeBook();

               GetOpenings(hWnd);
               break;
            case MSG_CHESS_ABOUT:
#ifndef WINCE
                DialogBoxA(hInst, MAKEINTRESOURCEA(AboutBox), hWnd, About);
#endif
                break;
            case MSG_CHESS_EDIT:
               EditActive = TRUE;
               hMenu = CreateMenu ();
               AppendMenu(hMenu, MF_STRING, MSG_CHESS_EDITDONE, TEXT("&Done"));
#ifndef WINCE
               hMainMenu = GetMenu(hWnd);
               SetMenu(hWnd, hMenu);
#endif
               DrawMenuBar(hWnd);
               break;
            case MSG_CHESS_EDITDONE:
               EditActive = FALSE;
#ifndef WINCE
               hMenu = GetMenu(hWnd);
               SetMenu(hWnd, hMainMenu);
#endif
               DrawMenuBar(hWnd);
               DestroyMenu(hMenu);
               GameCnt = 0;
               Game50 = 1;
               ZeroRPT ();
               Sdepth = 0;
               InitializeStats ();
               PostMessage ( hWnd, MSG_USER_MOVE, NULL, (long)NULL);
               break;

            case MSG_CHESS_REVIEW:
               ReviewDialog ( hWnd, hInst );
               break;

            case MSG_CHESS_TEST:
               TestDialog (hWnd, hInst);
               break;
        
            case MSG_CHESS_HASH:
               flag.hash = !flag.hash;
               break;

            case MSG_CHESS_BEEP:
               flag.beep = !flag.beep;
               break;

            case MSG_CHESS_COORD:
               coords = !coords;
               UpdateDisplay(hWnd,0,0,1,0);
               break;

            case MSG_CHESS_BOTH:
               flag.bothsides = !flag.bothsides;
               flag.easy = true;
               Sdepth = 0;
               PostMessage(hWnd, MSG_USER_MOVE, NULL, (long)NULL);
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
                  SendMessage(hStats, WM_SYSCOMMAND, SC_CLOSE, 0);
                  flag.post = false;
               }
               else
               {
                  StatDialog(hWnd, hInst);
                  flag.post = TRUE;
               }
               break;
            case MSG_CHESS_AWIN:
            {
               TCHAR str[40];
               LoadString(hInst, IDS_SETAWIN, str, sizeof(str)),
               Awindow = DoGetNumberDlg(hInst, hWnd, str, Awindow);
            }
               break;
            case MSG_CHESS_BWIN:
            {
               TCHAR str[40];
               LoadString(hInst, IDS_SETBWIN, str, sizeof(str));
               Bwindow = DoGetNumberDlg(hInst, hWnd, str, Bwindow);
            }
               break;

            case MSG_CHESS_CONTEMP:
            {
               TCHAR str[40];
               LoadString(hInst, IDS_SETCONTEMPT, str, sizeof(str));
               contempt = DoGetNumberDlg(hInst, hWnd, str, contempt);
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
               LoadString(hInst, IDS_MAXSEARCH, str, sizeof(str));
               MaxSearchDepth = DoGetNumberDlg(hInst, hWnd, str, MaxSearchDepth);
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
               if (ColorDialog(hWnd, hInst, wParam))
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
               if (ColorDialog(hWnd, hInst, wParam))
               {
                  InvalidateRect(hWnd, NULL, TRUE);
               }
               break;
            case IDM_WHITESQUARE:
               if (ColorDialog(hWnd, hInst, wParam))
               {
                  InvalidateRect(hWnd, NULL, TRUE);
               }
               break;
            case IDM_BLACKPIECE:
               if (ColorDialog ( hWnd, hInst, wParam) ) {
                  InvalidateRect (hWnd, NULL, TRUE);
               }
               break;
            case IDM_WHITEPIECE:
               if (ColorDialog ( hWnd, hInst, wParam) ) {
                  InvalidateRect (hWnd, NULL, TRUE);
               }
               break;
            case IDM_TEXT:
               if ( ColorDialog (hWnd, hInst, wParam) ) {
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
               SetStandardColors ();
               InvalidateRect (hWnd, NULL, TRUE);
               DeleteObject (hBrushBackGround);
               hBrushBackGround = CreateSolidBrush ( clrBackGround );

               /*Invalidate the text windows so they repaint */
               InvalidateRect (hComputerColor, NULL, TRUE);
               InvalidateRect (hComputerMove, NULL, TRUE);
               InvalidateRect (hWhosTurn, NULL, TRUE);
               InvalidateRect (hClockHuman, NULL, TRUE);
               InvalidateRect (hClockComputer, NULL, TRUE);
               InvalidateRect (hMsgComputer, NULL, TRUE);
               InvalidateRect (hMsgHuman, NULL, TRUE);
               break;

            case IDM_TIMECONTROL:
               if ( TimeControlDialog (hWnd, hInst, wParam) ) {
                  TCflag = (TCmoves>1);
                  if (TCflag) {
                  }
                  SetTimeControl ();
               }
               break;

            case MSG_HELP_INDEX:
            {
               TCHAR szHelpFileName[EXE_NAME_MAX_SIZE+1];
               MakeHelpPathName(szHelpFileName);
#ifndef WINCE
               WinHelp(hWnd,szHelpFileName,HELP_INDEX,0L);
#endif
               break;
            }
            case MSG_HELP_HELP:
#ifndef WINCE
               WinHelp(hWnd, TEXT("WINHELP.HLP"), HELP_INDEX, 0L);
#endif
               break;
         }
         break;

      default:
         return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return NULL;
}

LRESULT CALLBACK MainWindow::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (_instance)
        return _instance->_wndProc(hwnd, msg, wParam, lParam);
    return 0;
}

