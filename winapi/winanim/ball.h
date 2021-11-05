#ifndef BALL_H
#define BALL_H
#include <windows.h>

class Ball
{
private:
    const UINT idTimer1 = 1;
    UINT nTimerDelay = 10;
    HBITMAP _hbmBall;
    HBITMAP _hbmMask;
    BITMAP _bm;
    int _ballX, _ballY;
    int _deltaX, _deltaY;
    int deltaValue = 4;
    void _drawBall(HDC hdc);
    void _eraseBall(HDC hdc);
    void _updateBall(HWND hwnd);
public:
    void create(HINSTANCE hInstance, HWND hwnd);
    void timer(HWND hwnd);
    void paint(HWND hwnd);
    void destroy(HWND hwnd);
};

#endif

