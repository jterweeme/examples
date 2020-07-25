/*
  C source for GNU CHESS

  Revision: 1991-01-18

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

#include "chess.h"
#include "resource.h"

static TCHAR ManualDlgChar[8];

static INT_PTR CALLBACK
ManualMoveDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM)
{
    switch (iMessage)
    {
    case WM_INITDIALOG:
        ::SendMessage(GetDlgItem(hDlg, 100), EM_LIMITTEXT, sizeof(ManualDlgChar) - 1, 0);
        ::SetDlgItemText(hDlg, 100, ManualDlgChar);
        return TRUE;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_CLOSE)
        {
            ::EndDialog(hDlg, FALSE);
            return TRUE;
        }
        return FALSE;
    case WM_COMMAND:
        switch (wParam)
        {
        case IDOK:
            ::GetDlgItemText(hDlg, 100, ManualDlgChar, sizeof(ManualDlgChar) - 1);
            ::EndDialog(hDlg, TRUE);
            return TRUE;
        case IDCANCEL:
            ::EndDialog(hDlg, FALSE);
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

int DoManualMoveDlg(HINSTANCE hInst, HWND hWnd, TCHAR *szPrompt)
{
    ::lstrcpy(ManualDlgChar, TEXT(""));
    int stat = ::DialogBox(hInst, MAKEINTRESOURCE(MANUALDLG), hWnd, ::ManualMoveDlgProc);
    ::lstrcpy(szPrompt, ManualDlgChar);
    return stat;
}


