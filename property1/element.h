#ifndef ELEMENT_H
#define ELEMENT_H

#include <windows.h>

class Element
{
private:
    HWND _hwnd;
protected:
    Element *_parent;
    uintptr_t _id;
    int _x;
    int _y;
    int _width;
    int _height;
public:
    Element(Element *parent, uintptr_t id, int x, int y, int width, int height);
    HWND hwnd() const;
    virtual void move(int x, int y, int width, int height, BOOL repaint);
    virtual void move();

    void create(DWORD dwExStyle, LPCTSTR className, LPCTSTR windowName, DWORD style,
                int x, int y, int width, int height, HWND parent, HMENU menu,
                HINSTANCE hInstance, LPVOID param);
#ifndef WINCE
    LRESULT sendMsgA(UINT msg, WPARAM wp, LPARAM lp) const;
#endif
    LRESULT sendMsgW(UINT msg, WPARAM wp, LPARAM lp) const;
    LRESULT sendMsg(UINT msg, WPARAM wp, LPARAM lp) const;
    void setFocus();
};

#endif

