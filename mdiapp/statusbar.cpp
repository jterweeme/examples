#include "statusbar.h"
#include "mdi_unit.h"
#include <commctrl.h>

StatusBar::StatusBar(HWND hwnd) : Element(hwnd)
{

}

void StatusBar::create(HINSTANCE hInstance)
{
    _hwnd = CreateWindowEx(0, STATUSCLASSNAME, NULL,
                    WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0,
                    _parent, (HMENU)ID_STATUSBAR, hInstance, NULL);
}

void StatusBar::setParts(int n, int *widths)
{
    sendMsgW(SB_SETPARTS, n, LPARAM(widths));
}

void StatusBar::setText(int part, LPCWSTR s)
{
    sendMsgW(SB_SETTEXT, part, LPARAM(s));
}

