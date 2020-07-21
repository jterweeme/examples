#include "window.h"
#include "winclass.h"

Window::Window(WinClass *wc, int x, int y, int width, int height) :
    Element(0, 0, x, y, width, height), _wc(wc)
{

}

void Window::create(LPCTSTR caption)
{
    Element::create(WS_EX_CLIENTEDGE, _wc->className(), caption,
                    WS_OVERLAPPEDWINDOW, _x, _y, _width, _height, NULL, NULL,
                    _wc->hInstance(), NULL);
}

void Window::show(int nCmdShow)
{
    ::ShowWindow(hwnd(), nCmdShow);
}

void Window::update()
{
    ::UpdateWindow(hwnd());
}
