#ifndef HITTEST_H
#define HITTEST_H

#include <windows.h>

class HitTest
{
private:
    int _deltay;
    HRGN _hitrgn[8];
    int _horzHitTest(int x, int y);
public:
    HitTest();
    void init();
    void destroy();
    int test(int x, int y);
};

#endif

