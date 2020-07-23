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

#include "gnuchess.h"
#include "chess.h"
#include "defs.h"
#include <ctime>

void TestSpeed(HWND hWnd, int cnt, void (*f) (short int side, short int ply))
{
    long t2, evrate;
    TCHAR tmp[40];
    long t1;
#ifndef WINCE
	t1 = time(0);
#endif

    for (short i = 0; i < 10000; i++)
    {
        f(opponent, 2);
    }
#ifndef WINCE
    t2 = time(0);
#endif
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
        SetDlgItemText(hDlg, 100, TEXT(" "));
        SetDlgItemText(hDlg, 101, TEXT(" "));
        PostMessage(hDlg, WM_USER + 1, 0, 0);
        return (TRUE);
    case (WM_USER+1):
        hCursor = ::SetCursor(::LoadCursor(NULL, IDC_WAIT) );
        ::ShowCursor(TRUE);
        ::TestSpeed(hDlg, 100, MoveList);
        ::TestSpeed(hDlg, 101, CaptureList);
        ::ShowCursor(FALSE);
        ::SetCursor(hCursor);
        break;
    case WM_SYSCOMMAND:
        if ((wParam&0xfff0) == SC_CLOSE)
        {
            ::EndDialog(hDlg, NULL);
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

