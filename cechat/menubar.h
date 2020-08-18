#ifndef MENUBAR_H
#define MENUBAR_H
#include <windows.h>

class CommandBar
{
private:
    HINSTANCE _hInstance;
    HWND _hwnd;
public:
    CommandBar(HINSTANCE hInstance);
    void create(HWND hwnd);
    void insertMenuBar();
    void insertComboBox();
    void addAdornments();
};

class AbstractMenuBar
{
protected:
    HINSTANCE _hInstance;
public:
    AbstractMenuBar(HINSTANCE hInstance);
};

class MenuBarCE : public AbstractMenuBar
{
public:
    MenuBarCE(HINSTANCE hInstance);
};

#endif

