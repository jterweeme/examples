#ifndef HITTEST_H
#define HITTEST_H

#include <windows.h>

class AbstractHitTest
{
public:
    virtual ~AbstractHitTest();
    virtual void init(HDC) = 0;
    virtual void destroy() = 0;
    virtual int test(int x, int y, HDC hdc) = 0;
};

#ifdef WINCE
class HitTestCE : public AbstractHitTest
{
private:
    HRGN _hitrgn[64];
public:
    void init(HDC) override;
    void destroy() override;
    int test(int x, int y, HDC hdc) override;
};
#else
class HitTest : public AbstractHitTest
{
private:
    HRGN _hitrgn[64];
public:
    void init(HDC) override;
    void destroy() override;
    int test(int x, int y, HDC hdc) override;
};
#endif


#endif

