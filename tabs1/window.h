#ifndef WINDOW_H
#define WINDOW_H

#include "element.h"

class WinClass;

class Window : public Element
{
protected:
    WinClass *_wc;
public:
    Window(WinClass *wc, int x, int y, int width, int height);
    void create(LPCTSTR caption);
    void show(int nCmdShow);
    void update();
    HINSTANCE hInstance() const;
};

#endif

