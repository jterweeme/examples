#ifndef STATUSBAR_H
#define STATUSBAR_H
#include "element.h"

class StatusBar : public Element
{
public:
    StatusBar(HWND parent);
    void create(HINSTANCE hInstance);
    void setParts(int n, int *widths);
    void setText(int part, LPCWSTR s);
};

#endif

