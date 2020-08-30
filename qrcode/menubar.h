#ifndef MENUBAR_H
#define MENUBAR_H

#include <windows.h>

class AbstractMenuBar
{
private:
    HINSTANCE _hInstance;
protected:
    UINT _id;
public:
    AbstractMenuBar(HINSTANCE hInstance, UINT id);
    HINSTANCE hInstance() const;
    virtual void enable(HWND hwnd) = 0;
    virtual void disable(HWND hwnd) = 0;
    virtual UINT check(UINT id, BOOL check) = 0;
    virtual BOOL enableItem(UINT id, BOOL en) = 0;
    BOOL enableItem(UINT id);
};

#ifdef WINCE
class MenuBarCE : public AbstractMenuBar
{
public:
    MenuBarCE(HINSTANCE hInstance, UINT id);
    void enable(HWND hwnd) override;
    void disable(HWND hwnd) override;
    UINT check(UINT id, BOOL check) override;
    BOOL enableItem(UINT id, BOOL en) override;
};
#else
class MenuBar : public AbstractMenuBar
{
private:
    HMENU _hMenu;
public:
    MenuBar(HINSTANCE hInstance, UINT id);
    void enable(HWND hwnd) override;
    void disable(HWND hwnd) override;
    UINT check(UINT id, BOOL check) override;
    BOOL enableItem(UINT id, BOOL en) override;
};
#endif

#endif

