#ifndef ELEMENT_H
#define ELEMENT_H

#include <windows.h>

class Element
{
protected:
    HWND _hwnd;
    HWND _parent;
    Element *_parentElement;
    int _id;
    int _x;
    int _y;
    int _width;
    int _height;
public:
    //Element();
    Element(HWND parent);
    Element(int id, int x, int y, int width, int height);
    Element(HWND parent, int id, int x, int y, int width, int height);
    virtual void move();
#ifndef WINCE
    LRESULT sendMsgA(UINT msg, WPARAM wp, LPARAM lp) const;
#endif
    LRESULT sendMsgW(UINT msg, WPARAM wp, LPARAM lp) const;
    HWND handle() const;
};

#endif

