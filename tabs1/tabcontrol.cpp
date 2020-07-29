#include "tabcontrol.h"

TabControl::TabControl(Element *parent)
  :
    Element(parent, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT),
    _id(0)
{

}

TabControl::TabControl(Element *parent, int x, int y, int w, int h) :
    Element(parent, 0, x, y, w, h),
    _id(0)
{

}

void TabControl::create(HINSTANCE hInstance, HWND hwnd)
{
    Element::create(0, WC_TABCONTROL, L"", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
                    _x, _y, _width, _height, hwnd, NULL, hInstance, NULL);
}

void TabControl::insert(TCITEM *tie)
{
    TabCtrl_InsertItem(hwnd(), _id, tie);
    _id++;
}

int TabControl::getCurSel()
{
    return TabCtrl_GetCurSel(hwnd());
}

