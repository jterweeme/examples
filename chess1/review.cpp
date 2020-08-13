/*
  C source for GNU CHESS

  Revision: 1990-12-27

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

#include "protos.h"
#include "globals.h"
#include "resource.h"
#include "review.h"

ReviewDialog *ReviewDialog::_instance;

ReviewDialog::ReviewDialog(HINSTANCE hInstance) : _hInstance(hInstance)
{
    _instance = this;
}

INT_PTR ReviewDialog::run(HWND hwnd)
{
    return DialogBox(_hInstance, MAKEINTRESOURCE(IDD_REVIEW), hwnd, dlgProc);
}

INT_PTR ReviewDialog::_dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        for (int i = 1; i <= GameCnt; i++)
        {
            int f = GameList[i].gmove >> 8;
            int t = (GameList[i].gmove & 0xFF);
            algbr(f, t, false);
            TCHAR tmp[50];

            wsprintf(tmp, TEXT("%4d-%c\t%5s\t%-5d\t%-2d\t%-5d"),
                    (i + 1) / 2, i % 2 ? 'w' : 'b', (char *)mvstr[0],
                    GameList[i].score, GameList[i].depth,
                    GameList[i].time);

            ::SendDlgItemMessage(hwnd, 100, LB_ADDSTRING, 0, LPARAM(tmp));
        }
        ::SendDlgItemMessage(hwnd, 100, WM_SETREDRAW, TRUE, 0);
        return TRUE;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_CLOSE)
        {
            ::EndDialog(hwnd, NULL);
            return TRUE;
        }
        break;
    case WM_COMMAND:
        switch (wParam)
        {
        case IDOK:
            ::EndDialog(hwnd, 1);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK ReviewDialog::dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return _instance->_dlgProc(hwnd, msg, wParam, lParam);
}

