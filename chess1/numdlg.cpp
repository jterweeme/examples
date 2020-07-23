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

#include "chess.h"

#define IDD_INT 0x10
#define IDD_CHAR 0x11

static int NumberDlgInt;
static TCHAR NumberDlgChar[80];

static INT_PTR CALLBACK
NumberDlgDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM)
{
    int temp, Ier;
    switch (iMessage)
    {
    case WM_INITDIALOG:
        SetDlgItemText(hDlg, IDD_CHAR, NumberDlgChar);
        SetDlgItemInt(hDlg, IDD_INT, NumberDlgInt, TRUE);
        return TRUE;
    case WM_COMMAND:
        switch (wParam)
        {
        case IDOK:
            temp = GetDlgItemInt(hDlg, IDD_INT, &Ier, TRUE);

            if ( Ier != 0 )
            {
                  NumberDlgInt = temp;
                  EndDialog ( hDlg, TRUE);
            }
            return FALSE;
        case IDCANCEL:
            EndDialog ( hDlg, TRUE);
            return FALSE;
        }
        return FALSE;
    }
    return TRUE;
}

int DoGetNumberDlg(HINSTANCE hInst, HWND hWnd, TCHAR *szPrompt, int def)
{
    lstrcpy(::NumberDlgChar, szPrompt);
    ::NumberDlgInt = def;
    DialogBox(hInst, MAKEINTRESOURCE(NUMBERDLG), hWnd, NumberDlgDlgProc);
    return ::NumberDlgInt;
}
