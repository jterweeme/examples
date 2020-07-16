#ifndef TOOLBAR_H
#define TOOLBAR_H
#include "element.h"
#include <windows.h>
#include <commctrl.h>

class Toolbar : public Element
{
private:
    TBADDBITMAP tbab;
    TBBUTTON tbb[9];
public:
    Toolbar(HWND parent);
    void create(HINSTANCE hInst);
};

#endif

