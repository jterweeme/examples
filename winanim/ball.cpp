#include "ball.h"

void Ball::_eraseBall(HDC hdc)
{
    RECT rc;
    rc.left = _ballX;
    rc.top = _ballY;
    rc.right = _ballX + _bm.bmWidth;
    rc.bottom = _ballY + _bm.bmHeight;
    FillRect(hdc, &rc, HBRUSH(COLOR_BTNFACE + 1));
}

void Ball::_drawBall(HDC hdc)
{
    HDC hdcMemory = ::CreateCompatibleDC(hdc);
    ::SelectObject(hdcMemory, _hbmMask);
    ::BitBlt(hdc, _ballX, _ballY, _bm.bmWidth, _bm.bmHeight, hdcMemory, 0, 0, SRCAND);
    ::SelectObject(hdcMemory, _hbmBall);
    ::BitBlt(hdc, _ballX, _ballY, _bm.bmWidth, _bm.bmHeight, hdcMemory, 0, 0, SRCPAINT);
    ::DeleteDC(hdcMemory);
}

void Ball::_updateBall(HWND hwnd)
{
    RECT rc;
    ::GetClientRect(hwnd, &rc);

    _ballX += _deltaX;
    _ballY += _deltaY;

    if (_ballX < 0)
    {
        _ballX = 0;
        _deltaX = deltaValue;
    }
    else if (_ballX + _bm.bmWidth > rc.right)
    {
        _ballX = rc.right - _bm.bmWidth;
        _deltaX = -deltaValue;
    }

    if (_ballY < 0)
    {
        _ballY = 0;
        _deltaY = deltaValue;
    }
    else if (_ballY + _bm.bmHeight > rc.bottom)
    {
        _ballY = rc.bottom - _bm.bmHeight;
        _deltaY = -deltaValue;
    }
}

void Ball::create(HINSTANCE hInstance, HWND hwnd)
{
    _hbmBall = ::LoadBitmapA(hInstance, "BALLBMP");
    _hbmMask = ::LoadBitmapA(hInstance, "MASKBMP");

    if (!_hbmBall || !_hbmMask)
        throw "Load of resources failed";

    ::GetObject(_hbmBall, sizeof(_bm), &_bm);
    ::SetTimer(hwnd, idTimer1, nTimerDelay, NULL);
    _ballX = 0;
    _ballY = 0;
    _deltaX = deltaValue;
    _deltaY = deltaValue;
}

void Ball::timer(HWND hwnd)
{
    if (_hbmBall && _hbmMask)
    {
        HDC hdcWindow = ::GetDC(hwnd);
        _eraseBall(hdcWindow);
        _updateBall(hwnd);
        _drawBall(hdcWindow);
        ::ReleaseDC(hwnd, hdcWindow);
    }
}

void Ball::paint(HWND hwnd)
{
    if (_hbmBall && _hbmMask)
    {
        PAINTSTRUCT ps;
        HDC hdcWindow = BeginPaint(hwnd, &ps);
        _drawBall(hdcWindow);
        EndPaint(hwnd, &ps);
    }
}

void Ball::destroy(HWND hwnd)
{
    ::KillTimer(hwnd, idTimer1);
    ::DeleteObject(_hbmBall);
    ::DeleteObject(_hbmMask);
}

