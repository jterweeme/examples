#include "menubar.h"
#include <commctrl.h>

AbstractMenuBar::AbstractMenuBar(HINSTANCE hInstance, UINT id)
    : _hInstance(hInstance), _id(id)
{

}

HINSTANCE AbstractMenuBar::hInstance() const
{
    return _hInstance;
}

BOOL AbstractMenuBar::enableItem(UINT id)
{
    return enableItem(id, TRUE);
}

#ifdef WINCE
MenuBarCE::MenuBarCE(HINSTANCE hInstance, UINT id) : AbstractMenuBar(hInstance, id)
{

}

void MenuBarCE::enable(HWND hwnd)
{
    HWND hCommandBar = CommandBar_Create(hInstance(), hwnd, 500);
    CommandBar_InsertMenubar(hCommandBar, hInstance(), _id, 0);
    CommandBar_AddAdornments(hCommandBar, 0, 0);
}

void MenuBarCE::disable(HWND hwnd)
{

}

UINT MenuBarCE::check(UINT id, BOOL check)
{
    return 0;
}

BOOL MenuBarCE::enableItem(UINT id, BOOL en)
{
    return 0;
}

#else
MenuBar::MenuBar(HINSTANCE hInstance, UINT id)
    : AbstractMenuBar(hInstance, id), _hMenu(NULL)
{

}

void MenuBar::enable(HWND hwnd)
{
    _hMenu = ::LoadMenu(hInstance(), MAKEINTRESOURCE(_id));
    ::SetMenu(hwnd, _hMenu);
}

void MenuBar::disable(HWND)
{

}

UINT MenuBar::check(UINT id, BOOL check)
{
    UINT uCheck = check ? MF_CHECKED : MF_UNCHECKED;
    return CheckMenuItem(_hMenu, id, uCheck);
}

BOOL MenuBar::enableItem(UINT id, BOOL en)
{
    UINT uEn = en ? MF_ENABLED : MF_DISABLED;
    return EnableMenuItem(_hMenu, id, uEn);
}
#endif

