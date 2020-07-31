#include "colordlg.h"
#include "globals.h"
#include "palette.h"
#include <resource.h>

ColorDlg *ColorDlg::_instance;

ColorDlg::ColorDlg(HINSTANCE hInstance, UINT variant) :
    _hInstance(hInstance), _variant(variant)
{
    _instance = this;
}

INT_PTR ColorDlg::run(HWND hwnd, LPARAM lParam)
{
    return DialogBoxParam(_hInstance, MAKEINTRESOURCE(COLOR), hwnd, dlgProc, lParam);
}

static TCHAR lpWBGC[] = TEXT("Window background color");
static TCHAR lpBS[] = TEXT("Black square color");
static TCHAR lpWS[] = TEXT("White square color");
static TCHAR lpBP[] = TEXT("Black piece color");
static TCHAR lpWP[] = TEXT("White piece color");
static TCHAR lpTX[] = TEXT("Text color");

static COLORREF *pclr;
static int index;

void ColorDlg::_initDialogProc(HWND hwnd)
{
    LPCTSTR pchHeading;

    switch (_variant)
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

    ::SetDlgItemText(hwnd, IDD_HEADING, pchHeading);
    index = Palette::colorToIndex(*pclr);
    ::CheckRadioButton(hwnd, CNT_BLACK, CNT_WHITE, index);
}

INT_PTR ColorDlg::_commandProc(HWND hwnd, WPARAM wParam)
{
    switch (wParam)
    {
    case IDD_OK:
        ::EndDialog(hwnd, 1);
        *pclr = Palette::indexToColor(index);
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
        index = wParam;
        ::CheckRadioButton(hwnd, CNT_BLACK, CNT_WHITE, index);
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
