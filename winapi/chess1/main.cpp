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

#include "globals.h"
#include "winclass.h"
#include "mainwin.h"
#include "toolbox.h"
#include "resource.h"
#include "sim.h"
#include "book.h"
#include <ctime>

#ifdef WINCE
#define LPXSTR LPTSTR
#else
#define LPXSTR LPSTR
#endif

static void start_main(HINSTANCE hInstance, HWND hwnd, HWND compClr, Sim *sim)
{
    sim->NewGame(hwnd, compClr);

    try
    {
        GetOpenings(hInstance);
    }
    catch (UINT errId)
    {
        Toolbox().messageBox(hInstance, 0, errId, IDS_CHESS);
    }
    catch (LPCWSTR err)
    {
        ::MessageBoxW(0, err, L"Error", 0);
    }
    catch (...)
    {
        Toolbox().messageBox(hInstance, 0, IDS_UNKNOWNERR, IDS_CHESS);
    }
}

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPXSTR lpCmdLine, int nCmdShow)
{
    (void)lpCmdLine;
    (void)hPrevInstance;
    Sim sim;
    WinClass wc(hInstance, MainWindow::wndProc, TEXT("Chess"));
    HACCEL hAccel = ::LoadAccelerators(hInstance, TEXT("Chess"));
    MainWindow win(&wc, &sim, hAccel);

    try
    {
        wc.registerClass();
        win.create(TEXT("Chess"));
        win.show(nCmdShow);
        sim.init_main();
        start_main(hInstance, win.hwnd(), win.hComputerColor(), &sim);
        UpdateWindow(win.hwnd());
        player = opponent;
        ShowSidetoMove();
    }
    catch (LPCWSTR err)
    {
        ::MessageBoxW(0, err, L"Error", 0);
        sim.FreeGlobals();
        return -1;
    }
    catch (...)
    {
        ::MessageBox(0, TEXT("Unknown Error"), TEXT("Error"), 0);
        sim.FreeGlobals();
        return -1;
    }


    MSG msg;
    while (::GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(win.hwnd(), hAccel, &msg))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }

    return int(msg.wParam);
}


