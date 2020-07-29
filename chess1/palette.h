#ifndef PALETTE_H
#define PALETTE_H

#include <windows.h>

class Palette
{
public:
    static const COLORREF CBLACK = RGB(0,0,0);
    static const COLORREF BLUE = RGB(0,0,255);
    static const COLORREF GREEN = RGB(0,255,0);
    static const COLORREF CYAN = RGB(128,255,255);
    static const COLORREF RED = RGB(255,0,0);
    static const COLORREF PINK = RGB(255,0,255);
    static const COLORREF YELLOW = RGB(255,255,0);
    static const COLORREF PALEGRAY = RGB(192,192,192);
    static const COLORREF DARKGRAY = RGB(127,127,127);
    static const COLORREF DARKBLUE = RGB(0,0,128);
    static const COLORREF DARKGREEN = RGB(0,128,0);
    static const COLORREF DARKCYAN = RGB(0,255,255);
    static const COLORREF DARKRED = RGB(128,0,0);
    static const COLORREF DARKPINK = RGB(255,0,128);
    static const COLORREF BROWN = RGB(128,128,64);
    static const COLORREF CWHITE = RGB(255,255,255);
    static COLORREF indexToColor(int color);
    static int colorToIndex(COLORREF color);
};

#endif

