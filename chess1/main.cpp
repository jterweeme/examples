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

#include "protos.h"
#include "gnuchess.h"
#include "resource.h"
#include "globals.h"
#include "winclass.h"
#include "mainwin.h"
#include <ctime>

static TCHAR szAppName[] = TEXT("Chess");

#ifdef WINCE
#define LPXSTR LPTSTR
#else
#define LPXSTR LPSTR
#endif

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPXSTR lpCmdLine, int nCmdShow)
{
    (void)lpCmdLine;
    (void)hPrevInstance;
    hInst = hInstance;
    WinClass wc(hInstance, MainWindow::wndProc, szAppName);
    MainWindow win(&wc);

    try
    {
        wc.registerClass();
        win.create(szAppName);
        win.show(nCmdShow);
        init_main(win.hwnd());  //initialize chess
        UpdateWindow(win.hwnd());
        hAccel = ::LoadAccelerators(hInstance, szAppName);
        player = opponent;
        ShowSidetoMove();
    }
    catch (LPCWSTR err)
    {
        ::MessageBoxW(0, err, L"Error", 0);
        FreeGlobals();
        return -1;
    }
    catch (...)
    {
        ::MessageBox(0, TEXT("Error"), TEXT("Unknown Error"), 0);
        FreeGlobals();
        return -1;
    }


    MSG msg;
    while (::GetMessage(&msg, NULL, NULL, NULL))
    {
        if (!TranslateAccelerator(win.hwnd(), hAccel, &msg) )
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }

    return msg.wParam;
}

LRESULT
MainWindow::_wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    POINT point;

    switch (message)
    {
    case WM_CREATE:
        _createProc(hWnd, szAppName);
        break;
    case MSG_DESTROY:
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
    {
        TCHAR szHelpFileName[EXE_NAME_MAX_SIZE + 1];
        MakeHelpPathName(szHelpFileName);
        ::WinHelp(hWnd, szHelpFileName, HELP_QUIT, 0L);
        ::DeleteObject(hBrushBackGround);
        Hittest_Destructor();

        if (hBook)
            FreeBook();

        FreeGlobals();
        SaveColors(szAppName);
        ::PostQuitMessage(0);
    }
        break;
    case WM_PAINT:
        _paintProc(hWnd);
        break;
    case WM_CTLCOLORSTATIC:
    {
        HDC hdc = HDC(wParam);
        ::UnrealizeObject(hBrushBackGround);
        ::SetBkColor(hdc, clrBackGround);
        ::SetBkMode(hdc, TRANSPARENT);
        ::SetTextColor(hdc, clrText);
        POINT point;
        point.x = point.y = 0;
        ::ClientToScreen(hWnd, &point);
        ::SetBrushOrgEx(hdc, point.x, point.y, 0);
    }
        return LRESULT(hBrushBackGround);
    case WM_ERASEBKGND:
    {
        RECT rect;
        ::UnrealizeObject(HGDIOBJ(hBrushBackGround));
        ::GetClientRect(hWnd, &rect);
        ::FillRect(HDC(wParam), &rect, hBrushBackGround);
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

        if (GotFirst)
        {
            UnHiliteSquare(hWnd, FirstSq);
            GotFirst = FALSE;

            if ( EditActive == TRUE)
                ::PostMessage(hWnd, MSG_EDITBOARD, FirstSq << 8 | Hit, NULL);
            else if (User_Move == TRUE)
                ::PostMessage(hWnd, MSG_USER_ENTERED_MOVE, FirstSq << 8 | Hit, NULL);

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
            First = 63 - (wParam >> 8 & 0xff);
            Square  = 63 - (wParam & 0xff);
         } else {
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
         unsigned short mv;
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
            algbr_flag = promote + PromoteDialog(hWnd, hInstance());
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

            if (GotFirst)
			{
                UnHiliteSquare(hWnd, FirstSq);
                GotFirst = FALSE;
                FirstSq = -1;
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
        _commandProc(hWnd, wParam, szAppName);
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

