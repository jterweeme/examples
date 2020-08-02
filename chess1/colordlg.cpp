#include "colordlg.h"
#include "globals.h"
#include "palette.h"
#include <resource.h>

ColorDlg *ColorDlg::_instance;

ColorDlg::ColorDlg(HINSTANCE hInstance, UINT variant, COLORREF *item) :
    _hInstance(hInstance), _variant(variant), _pclr(item)
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
