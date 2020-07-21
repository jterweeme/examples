#ifndef TABCONTROL_H
#define TABCONTROL_H

#include "element.h"
#include <commctrl.h>

class TabControl : public Element
{
public:
    TabControl(Element *parent, int x, int y, int w, int h);
    void create(HINSTANCE hInstance, HWND hwnd);
    void insert(int id, TCITEM *tie);
};

#endif

