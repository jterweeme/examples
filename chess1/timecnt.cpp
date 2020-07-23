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
#include "resource.h"
#include "globals.h"
#include <windows.h>

static int tmpTCmoves;
static int tmpTCminutes;

static INT_PTR CALLBACK
TimeControlDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM)
{
    switch (message)
    {
    case WM_INITDIALOG:
        CheckRadioButton(hDlg, TMDLG_1MOV, TMDLG_60MOV, TCmoves + TMDLG_MOV);
        CheckRadioButton(hDlg, TMDLG_5MIN, TMDLG_600MIN, TCminutes+TMDLG_MIN);
        tmpTCminutes = TCminutes;
        tmpTCmoves   = TCmoves;
        return TRUE;
    case WM_SYSCOMMAND:
        if ((wParam&0xfff0) == SC_CLOSE)
        {
            EndDialog(hDlg, NULL);
            return TRUE;
        }
        break;
    case WM_COMMAND:
         switch (wParam) {
         
            case IDOK:
               TCminutes = tmpTCminutes;
               TCmoves   = tmpTCmoves;
		         EndDialog(hDlg, 1);
		         return TRUE;
               break;

             case IDCANCEL:
   		      EndDialog(hDlg, NULL);
	   	      return TRUE;
               break;

            case TMDLG_1MOV:
            case TMDLG_10MOV:
            case TMDLG_20MOV:
            case TMDLG_40MOV:
            case TMDLG_60MOV:
                  tmpTCmoves = wParam - TMDLG_MOV;       
                  CheckRadioButton ( hDlg, TMDLG_1MOV, TMDLG_60MOV, wParam);
                  break;

            case TMDLG_5MIN:
            case TMDLG_15MIN:
            case TMDLG_30MIN:
            case TMDLG_60MIN:
            case TMDLG_600MIN:
                  tmpTCminutes = wParam - TMDLG_MIN;
                  CheckRadioButton ( hDlg, TMDLG_5MIN, TMDLG_600MIN, wParam);
                  break;
        }
        break;
    }

    return FALSE;
}

int TimeControlDialog(HWND hWnd, HINSTANCE hInst, DWORD Param )
{
    int status = DialogBoxParam(hInst, MAKEINTRESOURCE(TIMECONTROL), hWnd, TimeControlDlgProc, Param);
    return status;
}

