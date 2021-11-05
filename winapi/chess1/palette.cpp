#include "palette.h"
#include "resource.h"

COLORREF Palette::indexToColor(int color)
{
    switch (color)
    {
    case CNT_BLACK:
        return CBLACK;
    case CNT_BLUE:
        return BLUE;
    case CNT_GREEN:
        return GREEN;
    case CNT_CYAN:
        return CYAN;
    case CNT_RED:
        return RED;
    case CNT_PINK:
        return PINK;
    case CNT_YELLOW:
        return YELLOW;
    case CNT_PALEGRAY:
        return PALEGRAY;
    case CNT_DARKGRAY:
        return DARKGRAY;
    case CNT_DARKBLUE:
        return DARKBLUE;
    case CNT_DARKGREEN:
        return DARKGREEN;
    case CNT_DARKCYAN:
        return DARKCYAN;
    case CNT_DARKRED:
        return DARKRED;
    case CNT_DARKPINK:
        return DARKPINK;
    case CNT_BROWN:
        return BROWN;
    case CNT_WHITE:
        return CWHITE;
    }

    return RED;
}

int Palette::colorToIndex(COLORREF color)
{
    switch (color)
    {
    case Palette::CBLACK:
        return CNT_BLACK;
    case Palette::BLUE:
        return CNT_BLUE;
    case Palette::GREEN:
        return CNT_GREEN;
    case Palette::CYAN:
        return CNT_CYAN;
    case Palette::RED:
        return CNT_RED;
    case Palette::PINK:
        return CNT_PINK;
    case Palette::YELLOW:
        return CNT_YELLOW;
    case Palette::PALEGRAY:
        return CNT_PALEGRAY;
    case Palette::DARKGRAY:
        return CNT_DARKGRAY;
    case Palette::DARKBLUE:
        return CNT_DARKBLUE;
    case Palette::DARKGREEN:
        return CNT_DARKGREEN;
    case Palette::DARKCYAN:
        return CNT_DARKCYAN;
    case Palette::DARKRED:
        return CNT_DARKRED;
    case Palette::DARKPINK:
        return CNT_DARKPINK;
    case Palette::BROWN:
        return CNT_BROWN;
    case Palette::CWHITE:
        return CNT_WHITE;
    }

    return CNT_WHITE;
}

