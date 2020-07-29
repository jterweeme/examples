#ifndef TABCONTROL_H
#define TABCONTROL_H

#include "element.h"
#include <commctrl.h>

class TabControl : public Element
{
private:
    int _id;
public:
    TabControl(Element *parent);
    TabControl(Element *parent, int x, int y, int w, int h);
    void create(HINSTANCE hInstance, HWND hwnd);
    void insert(TCITEM *tie);
    int getCurSel();
};

#endif

