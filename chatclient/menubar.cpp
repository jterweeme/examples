#include "menubar.h"
#include "resource.h"

AbstractMenuBar::AbstractMenuBar(HINSTANCE hInstance) : _hInstance(hInstance)
{

}

MenuBar::MenuBar(HINSTANCE hInstance) : AbstractMenuBar(hInstance)
{

}

HINSTANCE AbstractMenuBar::hInstance() const
{
    return _hInstance;
}

void MenuBar::enable(HWND hwnd)
{
    HMENU hMenu = ::LoadMenu(hInstance(), MAKEINTRESOURCE(IDM_MAIN));
    ::SetMenu(hwnd, hMenu);
}
