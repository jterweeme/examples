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
#include <tchar.h>
#include <string.h>
#include <stdio.h>

#define CBLACK     RGB(0,0,0)
#define BLUE      RGB(0,0,255)
#define GREEN     RGB(0,255,0)
#define CYAN      RGB(128,255,255)
#define RED       RGB(255,0,0)
#define PINK      RGB(255,0,255)
#define YELLOW    RGB(255,255,0)
#define PALEGRAY  RGB(192,192,192)
#define DARKGRAY  RGB(127,127,127)
#define DARKBLUE  RGB(0,0,128)
#define DARKGREEN RGB(0,128,0)
#define DARKCYAN  RGB(0,255,255)
#define DARKRED   RGB(128,0,0)
#define DARKPINK  RGB(255,0,128)
#define BROWN     RGB(128,128,64)
#define CWHITE     RGB(255,255,255)

void SetStandardColors(VOID)
{
    clrBackGround = BROWN;
    clrBlackSquare = DARKGREEN;
    clrWhiteSquare = PALEGRAY;
    clrBlackPiece = RED;
    clrWhitePiece = CWHITE;
    clrText = CBLACK;
}

static TCHAR lpChessini[] = TEXT("chess.ini");
static TCHAR lpBackGround[] = TEXT("BackGround");
static TCHAR lpBlackSquare[] = TEXT("BlackSquare");
static TCHAR lpWhiteSquare[] = TEXT("WhiteSquare");
static TCHAR lpBlackPiece[]  = TEXT("BlackPiece");
static TCHAR lpWhitePiece[]  = TEXT("WhitePiece");
static TCHAR lpDefault[]     = TEXT("Default");
static TCHAR lpText[]        = TEXT("Text");

static TCHAR np08lX[] = TEXT("%08lX");

void SaveColors(LPTSTR appname)
{
    TCHAR ostring[30];
    wsprintf(ostring, np08lX, clrBackGround);
    WritePrivateProfileString(appname, lpBackGround, ostring, lpChessini);
    wsprintf(ostring, np08lX, clrBlackSquare);
    WritePrivateProfileString(appname, lpBlackSquare, ostring, lpChessini);
    wsprintf(ostring, np08lX, clrWhiteSquare);
    WritePrivateProfileString(appname, lpWhiteSquare, ostring,lpChessini);
    wsprintf(ostring, np08lX, clrBlackPiece);
    WritePrivateProfileString(appname, lpBlackPiece, ostring,lpChessini);
    wsprintf(ostring, np08lX, clrWhitePiece);
    WritePrivateProfileString(appname, lpWhitePiece, ostring,lpChessini);
    wsprintf(ostring, np08lX, clrText);
    WritePrivateProfileString(appname, lpText,ostring,lpChessini);
}

void GetStartupColors(LPCTSTR appname)
{
    TCHAR istring[30];
    SetStandardColors();
    GetPrivateProfileString(appname, lpBackGround, lpDefault,
                            istring, sizeof(istring), lpChessini);

    if (_tcscmp(istring, lpDefault) != 0)
        _stscanf(istring, np08lX, &clrBackGround);

    GetPrivateProfileString(appname, lpBlackSquare, lpDefault,
                            istring, sizeof(istring), lpChessini);

    if (_tcscmp(istring, lpDefault) != 0)
        _stscanf(istring, np08lX, &clrBlackSquare);

    GetPrivateProfileString(appname, lpWhiteSquare, lpDefault,
                            istring, sizeof(istring), lpChessini);

    if (_tcscmp(istring, lpDefault) != 0)
        _stscanf(istring, np08lX, &clrWhiteSquare);

    GetPrivateProfileString(appname, lpBlackPiece, lpDefault,istring,
                             sizeof(istring), lpChessini);

    if (_tcscmp(istring, lpDefault) != 0)
        _stscanf(istring, np08lX, &clrBlackPiece);

    GetPrivateProfileString(appname, lpWhitePiece,lpDefault,istring,
                             sizeof(istring), lpChessini);

    if (_tcscmp(istring, lpDefault) != 0)
        _stscanf(istring, np08lX, &clrWhitePiece);

    GetPrivateProfileString(appname, lpText, lpDefault,
                            istring, sizeof(istring), lpChessini);

    if (_tcscmp(istring, lpDefault) != 0)
        _stscanf(istring, np08lX, &clrText);
}

static TCHAR lpWBGC[] = TEXT("Window background color");
static TCHAR lpBS[] = TEXT("Black square color");
static TCHAR lpWS[] = TEXT("White square color");
static TCHAR lpBP[] = TEXT("Black piece color");
static TCHAR lpWP[] = TEXT("White piece color");
static TCHAR lpTX[] = TEXT("Text color");

static DWORD *pclr;
static int index;

static DWORD IndexToColor(int color)
{
    switch (color)
    {
    case CNT_BLACK:
        return CBLACK;
    case CNT_BLUE:
        return BLUE;
    }

    if (color == CNT_BLACK)
        return CBLACK;

    if (color == CNT_BLUE)
        return BLUE;

    if (color == CNT_GREEN)
        return GREEN;
    if (color == CNT_CYAN)
        return CYAN;
    if (color == CNT_RED)
        return RED;
    if (color == CNT_PINK)
        return PINK;
    else if (color == CNT_YELLOW) return YELLOW;
    else if (color == CNT_PALEGRAY) return PALEGRAY;
    else if (color == CNT_DARKGRAY) return DARKGRAY;
    else if (color == CNT_DARKBLUE) return DARKBLUE;
    else if (color == CNT_DARKGREEN) return DARKGREEN;
    else if (color == CNT_DARKCYAN) return DARKCYAN;
    else if (color == CNT_DARKRED) return DARKRED;
    else if (color == CNT_DARKPINK) return DARKPINK;
    else if (color == CNT_BROWN) return BROWN;
    else if (color == CNT_WHITE) return CWHITE;
    return RGB(255,0,0);
}

static int ColorToIndex(DWORD color)
{
   if (color == CBLACK ) return CNT_BLACK;
   else if ( color == BLUE) return CNT_BLUE;
   else if ( color == GREEN) return CNT_GREEN;
   else if ( color == CYAN) return CNT_CYAN;
   else if ( color == RED) return CNT_RED;
   else if ( color == PINK) return CNT_PINK;
   else if ( color == YELLOW) return CNT_YELLOW;
   else if ( color == PALEGRAY) return CNT_PALEGRAY;
   else if ( color == DARKGRAY) return CNT_DARKGRAY;
   else if ( color == DARKBLUE) return CNT_DARKBLUE;
   else if ( color == DARKGREEN) return CNT_DARKGREEN;
   else if ( color == DARKCYAN) return CNT_DARKCYAN;
   else if ( color == DARKRED) return CNT_DARKRED;
   else if ( color == DARKPINK) return CNT_DARKPINK;
   else if ( color == BROWN) return CNT_BROWN;
   else if ( color == CWHITE) return CNT_WHITE;
   return CNT_WHITE;
}

static LRESULT CALLBACK
ColorDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    char *pchHeading;

    switch (message)
    {
        case WM_INITDIALOG:
            switch (lParam)
            {
            default:
            case IDM_BACKGROUND:
               pchHeading = (char far *) lpWBGC;
               pclr       = &clrBackGround;
               break;

            case IDM_BLACKSQUARE:
               pchHeading = (char far *)lpBS;
               pclr       = &clrBlackSquare;
               break;

            case IDM_WHITESQUARE:
               pchHeading = (char far *)lpWS;
               pclr       = &clrWhiteSquare;
               break;

            case IDM_BLACKPIECE:
               pchHeading = (char far *) lpBP;
               pclr       = &clrBlackPiece;
               break;

            case IDM_WHITEPIECE:
               pchHeading = (char far *) lpWP;
               pclr       = &clrWhitePiece;
               break;

            case IDM_TEXT:
               pchHeading = (char far *) lpTX;
               pclr       = &clrText;
               break;
         }

         SetDlgItemTextA(hDlg, IDD_HEADING, pchHeading);
         index = ColorToIndex(*pclr);
         CheckRadioButton(hDlg, CNT_BLACK, CNT_WHITE, index);
         return TRUE;
       case WM_COMMAND:
         switch (wParam)
         {

            case IDD_OK:
                 EndDialog(hDlg, 1);
               *pclr = IndexToColor (index);
                 return TRUE;

            case IDD_CANCEL:
              EndDialog(hDlg, NULL);
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
                CheckRadioButton(hDlg, CNT_BLACK, CNT_WHITE, index);
                break;
        }
        break;
    }

    return FALSE;
}

int ColorDialog(HWND hWnd, HINSTANCE hInst, WPARAM Param)
{
    int status;
    status = DialogBoxParamA(hInst, MAKEINTRESOURCEA(COLOR), hWnd, ColorDlgProc, Param);
    return status;
}



