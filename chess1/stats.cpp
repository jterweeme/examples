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

#include "globals.h"
#include "resource.h"

static INT_PTR CALLBACK
StatDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM)
{
    switch (message)
    {
    case WM_INITDIALOG:
        SetDlgItemText(hDlg, DEPTHTEXT,    TEXT(" "));
        SetDlgItemText(hDlg, POSITIONTEXT, TEXT(" "));
        SetDlgItemText(hDlg, NODETEXT,     TEXT(" "));
        SetDlgItemText(hDlg, BSTLINETEXT,  TEXT(" "));
        SetDlgItemText(hDlg, SCORETEXT,    TEXT(" "));
        SetDlgItemText(hDlg, NODESECTEXT,  TEXT(" "));
        return (TRUE);
    case WM_SYSCOMMAND:
        if ((wParam&0xfff0) == SC_CLOSE)
        {
            ::DestroyWindow(hDlg);
            return TRUE;
        }
        break;
    case WM_DESTROY:
        hStats = NULL;
        flag.post = false;
        break;
    }

    return FALSE;
}

int StatDialog(HWND hWnd, HINSTANCE hInst)
{
    CreateDialog(hInst, MAKEINTRESOURCE(STATS), hWnd, StatDlgProc);
	return 0;
    //return hStats;
}

