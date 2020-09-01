#ifndef EDITBOX_H
#define EDITBOX_H

#include "element.h"

class EditBox : public Element
{
public:
    EditBox(Element *parent, UINT_PTR id, int x, int y, int w, int h);
    EditBox(Element *parent, UINT_PTR id);
    void create(HINSTANCE hInstance, HWND hwnd);
    void setFont(int font);
};

#endif

