#include "menubar.h"
#include "resource.h"
#include <commctrl.h>

CommandBar::CommandBar(HINSTANCE hInstance) : _hInstance(hInstance)
{

}

void CommandBar::create(HWND hwnd)
{
    (void)hwnd;
#ifdef WINCE
    _hwnd = CommandBar_Create(_hInstance, hwnd, IDC_CMDBAR);
#endif
}

void CommandBar::insertMenuBar()
{
#ifdef WINCE
    CommandBar_InsertMenubar(_hwnd, _hInstance, ID_MENU, 0);
#endif
}

void CommandBar::insertComboBox()
{
#ifdef WINCE
    CommandBar_InsertComboBox(_hwnd, _hInstance, 140, CBS_DROPDOWNLIST, IDC_COMPORT, 1);
#endif
}

void CommandBar::addAdornments()
{
#ifdef WINCE
    CommandBar_AddAdornments(_hwnd, 0, 0);
#endif
}

AbstractMenuBar::AbstractMenuBar(HINSTANCE hInstance) :_hInstance(hInstance)
{

}

MenuBarCE::MenuBarCE(HINSTANCE hInstance) : AbstractMenuBar(hInstance)
{

}
