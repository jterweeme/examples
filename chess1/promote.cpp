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

#include "promote.h"
#include "chess.h"
#include "resource.h"

static int xstatus;

PromoteDlg *PromoteDlg::_instance;

PromoteDlg::PromoteDlg(HINSTANCE hInstance) : _hInstance(hInstance)
{
    _instance = this;
}

INT_PTR PromoteDlg::run(HWND hwnd)
{
    return DialogBox(_hInstance, MAKEINTRESOURCE(PAWNPROMOTE), hwnd, dlgProc);
}

INT_PTR CALLBACK
PromoteDlg::dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return _instance->_dlgProc(hwnd, msg, wParam, lParam);
}

INT_PTR
PromoteDlg::_dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        xstatus = 5;
        CheckRadioButton(hwnd, 100, 103, 103);
        return TRUE;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_CLOSE)
        {
            ::EndDialog(hwnd, xstatus);
            return xstatus;
        }
        break;
    case WM_COMMAND:
        switch (wParam)
        {
        case IDOK:
            ::EndDialog(hwnd, xstatus);
            return TRUE;
        case 100:
        case 101:
        case 102:
        case 103:
            xstatus = 2 + wParam - 100;
            CheckRadioButton(hwnd, 100, 103, wParam);
            break;
        }
        break;
    }

    return FALSE;
}

