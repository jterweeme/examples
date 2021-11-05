#ifndef MENUBAR_H
#define MENUBAR_H

#include "toolbox.h"

class AbstractMenuBar
{
private:
    HINSTANCE _hInstance;
public:
    AbstractMenuBar(HINSTANCE hInstance);
    HINSTANCE hInstance() const;
    virtual void enable(HWND hwnd) = 0;
};

#ifdef WINCE
class MenuBarCE : public AbstractMenuBar
{
public:
    MenuBarCE(HINSTANCE hInstance);
};
#else
class MenuBar : public AbstractMenuBar
{
public:
    MenuBar(HINSTANCE hInstance);
    void enable(HWND hwnd) OVERRIDE;
};
#endif

#endif

