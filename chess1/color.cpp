/*
  C source for GNU CHESS

  Revision: 1990-09-30 Daryl Baker

  Based on Ideas and code segments of Charles Petzold from artices in
  MicroSoft Systems Journal January 1990 Vol. 5 Number 1.

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

#include "resource.h"
#include "chess.h"
#include "globals.h"
#include "palette.h"

#if 0
static TCHAR lpChessini[] = TEXT("chess.ini");
static TCHAR lpBackGround[] = TEXT("BackGround");
static TCHAR lpBlackSquare[] = TEXT("BlackSquare");
static TCHAR lpWhiteSquare[] = TEXT("WhiteSquare");
static TCHAR lpBlackPiece[] = TEXT("BlackPiece");
static TCHAR lpWhitePiece[] = TEXT("WhitePiece");
static TCHAR lpDefault[] = TEXT("Default");
static TCHAR lpText[] = TEXT("Text");
static TCHAR np08lX[] = TEXT("%08lX");

void SaveColors(LPCTSTR appname)
{
    TCHAR ostring[30];
    ::wsprintf(ostring, np08lX, clrBackGround);
    WritePrivateProfileString(appname, lpBackGround, ostring, lpChessini);
    ::wsprintf(ostring, np08lX, clrBlackSquare);
    WritePrivateProfileString(appname, lpBlackSquare, ostring, lpChessini);
    ::wsprintf(ostring, np08lX, clrWhiteSquare);
    WritePrivateProfileString(appname, lpWhiteSquare, ostring, lpChessini);
    ::wsprintf(ostring, np08lX, clrBlackPiece);
    WritePrivateProfileString(appname, lpBlackPiece, ostring, lpChessini);
    ::wsprintf(ostring, np08lX, clrWhitePiece);
    WritePrivateProfileString(appname, lpWhitePiece, ostring, lpChessini);
    ::wsprintf(ostring, np08lX, clrText);
    WritePrivateProfileString(appname, lpText, ostring, lpChessini);
}

void GetStartupColors()
{

    SetStandardColors();

	TCHAR istring[30];

    GetPrivateProfileString(appname, lpBackGround, lpDefault,
                            istring, sizeof(istring), lpChessini);

    if (::lstrcmp(istring, lpDefault) != 0)
        sscanf(istring, np08lX, &clrBackGround);

    GetPrivateProfileString(appname, lpBlackSquare, lpDefault,
                            istring, sizeof(istring), lpChessini);

    if (::lstrcmp(istring, lpDefault) != 0)
        sscanf(istring, np08lX, &clrBlackSquare);

    GetPrivateProfileString(appname, lpWhiteSquare, lpDefault,
                            istring, sizeof(istring), lpChessini);

    if (::lstrcmp(istring, lpDefault) != 0)
        sscanf(istring, np08lX, &clrWhiteSquare);

    GetPrivateProfileString(appname, lpBlackPiece, lpDefault,istring,
                             sizeof(istring), lpChessini);

    if (::lstrcmp(istring, lpDefault) != 0)
        sscanf(istring, np08lX, &clrBlackPiece);

    GetPrivateProfileString(appname, lpWhitePiece,lpDefault,istring,
                             sizeof(istring), lpChessini);

    if (::lstrcmp(istring, lpDefault) != 0)
        sscanf(istring, np08lX, &clrWhitePiece);

    GetPrivateProfileString(appname, lpText, lpDefault,
                            istring, sizeof(istring), lpChessini);

    if (::lstrcmp(istring, lpDefault) != 0)
        sscanf(istring, np08lX, &clrText);
}
#endif

static TCHAR lpWBGC[] = TEXT("Window background color");
static TCHAR lpBS[] = TEXT("Black square color");
static TCHAR lpWS[] = TEXT("White square color");
static TCHAR lpBP[] = TEXT("Black piece color");
static TCHAR lpWP[] = TEXT("White piece color");
static TCHAR lpTX[] = TEXT("Text color");

static DWORD *pclr;
static int index;

static INT_PTR CALLBACK
ColorDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        LPCTSTR pchHeading;

        switch (lParam)
        {
        default:
        case IDM_BACKGROUND:
            pchHeading = LPCTSTR(lpWBGC);
            pclr = &clrBackGround;
            break;
        case IDM_BLACKSQUARE:
            pchHeading = LPCTSTR(lpBS);
            pclr = &clrBlackSquare;
            break;
        case IDM_WHITESQUARE:
            pchHeading = LPCTSTR(lpWS);
            pclr = &clrWhiteSquare;
            break;
        case IDM_BLACKPIECE:
            pchHeading = LPCTSTR(lpBP);
            pclr = &clrBlackPiece;
            break;
        case IDM_WHITEPIECE:
            pchHeading = LPCTSTR(lpWP);
            pclr = &clrWhitePiece;
            break;
        case IDM_TEXT:
            pchHeading = LPCTSTR(lpTX);
            pclr = &clrText;
            break;
        }

        SetDlgItemText(hDlg, IDD_HEADING, pchHeading);
        index = Palette::colorToIndex(*pclr);
        ::CheckRadioButton(hDlg, CNT_BLACK, CNT_WHITE, index);
    }
        return TRUE;
    case WM_COMMAND:
        switch (wParam)
        {
        case IDD_OK:
            ::EndDialog(hDlg, 1);
            *pclr = Palette::indexToColor(index);
            return TRUE;
        case IDD_CANCEL:
            ::EndDialog(hDlg, NULL);
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
            index = wParam;
            ::CheckRadioButton(hDlg, CNT_BLACK, CNT_WHITE, index);
            break;
        }
        break;
    }

    return FALSE;
}

int ColorDialog(HWND hWnd, HINSTANCE hInst, WPARAM Param)
{
    int status;
    status = DialogBoxParam(hInst, MAKEINTRESOURCE(COLOR), hWnd, ColorDlgProc, Param);
    return status;
}



