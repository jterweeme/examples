#include "editbox.h"

EditBox::EditBox(Element *parent, uintptr_t id, int x, int y, int w, int h) :
    Element(parent, id, x, y, w, h)
{
}

EditBox::EditBox(Element *parent, uintptr_t id) :
    Element(parent, id, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT)
{
}

void EditBox::create(HINSTANCE hInstance, HWND hwnd)
{
    Element::create(0, TEXT("EDIT"), TEXT(""),
                    WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | ES_MULTILINE | ES_WANTRETURN,
                    _x, _y, _width, _height, hwnd, HMENU(_id), hInstance, NULL);

    setFont(SYSTEM_FIXED_FONT);
}

void EditBox::setFont(int font)
{
    sendMsgW(WM_SETFONT, WPARAM(GetStockObject(font)), MAKELPARAM(TRUE, 0));
}
