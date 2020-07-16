#include "element.h"

Element::Element() :
    _hwnd(0), _parent(0), _id(0), _x(0), _y(0), _width(0), _height(0)
{

}

Element::Element(HWND parent) :
    _hwnd(0), _parent(parent), _id(0), _x(0), _y(0), _width(0), _height(0)
{

}

Element::Element(int id, int x, int y, int width, int height) :
    _id(id), _x(x), _y(y), _width(width), _height(height)
{
}

Element::Element(HWND parent, int id, int x, int y, int width, int height) :
    _parent(parent), _id(id), _x(x), _y(y), _width(width), _height(height)
{
}


void Element::move()
{
    ::MoveWindow(::GetDlgItem(_hwnd, _id), _x, _y, _width, _height, TRUE);
}

#ifndef WINCE
LRESULT Element::sendMsgA(UINT msg, WPARAM wp, LPARAM lp) const
{
    return SendMessageA(_hwnd, msg, wp, lp);
}
#endif

LRESULT Element::sendMsgW(UINT msg, WPARAM wp, LPARAM lp) const
{
    return SendMessageW(_hwnd, msg, wp, lp);
}

HWND Element::handle() const
{
    return _hwnd;
}

