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

#include "globals.h"
#include "resource.h"
#include "dialog.h"
#include "palette.h"

Dialog::Dialog(HINSTANCE hInstance) : _hInstance(hInstance)
{

}

HINSTANCE Dialog::hInstance() const
{
    return _hInstance;
}

AboutDlg::AboutDlg(HINSTANCE hInstance) : Dialog(hInstance)
{

}

static TCHAR Version[100];

INT_PTR CALLBACK AboutDlg::dlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        SetDlgItemText(hDlg, 106, Version);
        return TRUE;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_CLOSE)
        {
            ::EndDialog(hDlg, NULL);
            return TRUE;
        }
        break;
    case WM_COMMAND:
        if (wParam == IDOK)
        {
            ::EndDialog(hDlg, NULL);
            return TRUE;
        }
        break;
    }

    return FALSE;
    return 0;
    //return _instance->_dlgProc(hwnd, msg, wParam, lParam);
}

INT_PTR AboutDlg::run(HWND hwnd)
{
    return DialogBox(hInstance(), MAKEINTRESOURCE(IDD_ABOUT), hwnd, dlgProc);
}

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

ColorDlg *ColorDlg::_instance;

ColorDlg::ColorDlg(HINSTANCE hInstance, UINT variant, COLORREF *item) :
    Dialog(hInstance), _variant(variant), _pclr(item)
{
    _instance = this;
}

INT_PTR ColorDlg::run(HWND hwnd, LPARAM lParam)
{
    return DialogBoxParam(hInstance(), MAKEINTRESOURCE(COLOR), hwnd, dlgProc, lParam);
}

static TCHAR lpWBGC[] = TEXT("Window background color");
static TCHAR lpBS[] = TEXT("Black square color");
static TCHAR lpWS[] = TEXT("White square color");
static TCHAR lpBP[] = TEXT("Black piece color");
static TCHAR lpWP[] = TEXT("White piece color");
static TCHAR lpTX[] = TEXT("Text color");

void ColorDlg::_initDialogProc(HWND hwnd)
{
    LPCTSTR pchHeading;

    switch (_variant)
    {
    default:
    case IDM_BACKGROUND:
        pchHeading = lpWBGC;
        break;
    case IDM_BLACKSQUARE:
        pchHeading = lpBS;
        break;
    case IDM_WHITESQUARE:
        pchHeading = lpWS;
        break;
    case IDM_BLACKPIECE:
        pchHeading = lpBP;
        break;
    case IDM_WHITEPIECE:
        pchHeading = lpWP;
        break;
    case IDM_TEXT:
        pchHeading = lpTX;
        break;
    }

    ::SetDlgItemText(hwnd, IDD_HEADING, pchHeading);
    _index = Palette::colorToIndex(*_pclr);
    ::CheckRadioButton(hwnd, CNT_BLACK, CNT_WHITE, _index);
}

INT_PTR ColorDlg::_commandProc(HWND hwnd, WPARAM wParam)
{
    switch (wParam)
    {
    case IDD_OK:
        ::EndDialog(hwnd, 1);
        *_pclr = Palette::indexToColor(_index);
        return TRUE;
    case IDD_CANCEL:
        ::EndDialog(hwnd, NULL);
        return TRUE;
    case CNT_BLACK:
    case CNT_BLUE:
    case CNT_GREEN:
    case CNT_CYAN:
    case CNT_RED:
    case CNT_PINK:
    case CNT_YELLOW:
    case CNT_PALEGRAY:
    case CNT_DARKGRAY:
    case CNT_DARKBLUE:
    case CNT_DARKGREEN:
    case CNT_DARKCYAN:
    case CNT_DARKRED:
    case CNT_DARKPINK:
    case CNT_BROWN:
    case CNT_WHITE:
        _index = wParam;
        ::CheckRadioButton(hwnd, CNT_BLACK, CNT_WHITE, _index);
        break;
    }

    return FALSE;
}

INT_PTR ColorDlg::_dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        _initDialogProc(hwnd);
        return TRUE;
    case WM_COMMAND:
        return _commandProc(hwnd, wParam);
    }

    return FALSE;
}

INT_PTR ColorDlg::dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return _instance->_dlgProc(hwnd, msg, wParam, lParam);
}

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

static TCHAR ManualDlgChar[8];
ManualDlg *ManualDlg::_instance;

ManualDlg::ManualDlg(HINSTANCE hInstance) : _hInstance(hInstance)
{
    _instance = this;
}

INT_PTR ManualDlg::run(HWND hwnd, TCHAR *szPrompt)
{
    ::lstrcpy(ManualDlgChar, TEXT(""));
    int stat = ::DialogBox(_hInstance, MAKEINTRESOURCE(MANUALDLG), hwnd, dlgProc);
    ::lstrcpy(szPrompt, ManualDlgChar);
    return stat;
}

INT_PTR ManualDlg::_dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        ::SendMessage(GetDlgItem(hwnd, 100), EM_LIMITTEXT, sizeof(ManualDlgChar) - 1, 0);
        ::SetDlgItemText(hwnd, 100, ManualDlgChar);
        return TRUE;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_CLOSE)
        {
            ::EndDialog(hwnd, FALSE);
            return TRUE;
        }
        return FALSE;
    case WM_COMMAND:
        switch (wParam)
        {
        case IDOK:
            ::GetDlgItemText(hwnd, 100, ManualDlgChar, sizeof(ManualDlgChar) - 1);
            ::EndDialog(hwnd, TRUE);
            return TRUE;
        case IDCANCEL:
            ::EndDialog(hwnd, FALSE);
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

INT_PTR CALLBACK ManualDlg::dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return _instance->_dlgProc(hwnd, msg, wParam, lParam);
}

TimeCtrlDlg *TimeCtrlDlg::_instance;

TimeCtrlDlg::TimeCtrlDlg(HINSTANCE hInstance) : Dialog(hInstance)
{
    _instance = this;
}

static int tmpTCmoves;
static int tmpTCminutes;

INT_PTR CALLBACK
TimeCtrlDlg::dlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM)
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
        if ((wParam & 0xfff0) == SC_CLOSE)
        {
            EndDialog(hDlg, NULL);
            return TRUE;
        }
        break;
    case WM_COMMAND:
        switch (wParam)
        {
        case IDOK:
            TCminutes = tmpTCminutes;
            TCmoves = tmpTCmoves;
            ::EndDialog(hDlg, 1);
            return TRUE;
        case IDCANCEL:
            EndDialog(hDlg, NULL);
            return TRUE;
        case TMDLG_1MOV:
        case TMDLG_10MOV:
        case TMDLG_20MOV:
        case TMDLG_40MOV:
        case TMDLG_60MOV:
            tmpTCmoves = wParam - TMDLG_MOV;
            CheckRadioButton(hDlg, TMDLG_1MOV, TMDLG_60MOV, wParam);
            break;
        case TMDLG_5MIN:
        case TMDLG_15MIN:
        case TMDLG_30MIN:
        case TMDLG_60MIN:
        case TMDLG_600MIN:
            tmpTCminutes = wParam - TMDLG_MIN;
            CheckRadioButton(hDlg, TMDLG_5MIN, TMDLG_600MIN, wParam);
            break;
        }
        break;
    }

    return FALSE;
}

INT_PTR TimeCtrlDlg::run(HWND hwnd, LPARAM param)
{
    return DialogBoxParam(hInstance(), MAKEINTRESOURCE(TIMECONTROL), hwnd, dlgProc, param);
}

