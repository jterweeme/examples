#include "tabcontrol.h"

TabControl::TabControl(Element *parent, int x, int y, int w, int h) :
    Element(parent, 0, x, y, w, h)
{

}

void TabControl::create(HINSTANCE hInstance, HWND hwnd)
{
    Element::create(0, WC_TABCONTROL, L"", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
                    _x, _y, _width, _height, hwnd, NULL, hInstance, NULL);
}

void TabControl::insert(int id, TCITEM *tie)
{
    TabCtrl_InsertItem(hwnd(), id, tie);
}

