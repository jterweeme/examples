#include "element.h"

Element::Element(Element *parent, int id, int x, int y, int width, int height) :
    _parent(parent), _id(id), _x(x), _y(y), _width(width), _height(height)
{
}

void Element::move(int x, int y, int width, int height, BOOL repaint)
{
    _x = x, _y = y, _width = width, _height = height;
    ::MoveWindow(_hwnd, x, y, width, height, repaint);
}

void Element::move()
{
    move(_x, _y, _width, _height, TRUE);
}

HWND Element::hwnd() const
{
    return _hwnd;
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

void Element::setFocus()
{
    ::SetFocus(_hwnd);
}

void Element::create(
    DWORD dwExStyle, LPCWSTR className, LPCWSTR windowName, DWORD style,
    int x, int y, int width, int height, HWND parent, HMENU menu,
    HINSTANCE hInstance, LPVOID param)
{
    _hwnd = ::CreateWindowExW(dwExStyle, className, windowName, style,
                   x, y, width, height, parent, menu, hInstance, param);

    if (_hwnd == 0)
        throw "Cannot create element";
}

