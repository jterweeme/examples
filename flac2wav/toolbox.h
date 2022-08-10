//file: toolbox.h

#ifndef TOOLBOX_H
#define TOOLBOX_H

#include <cstdint>

template <class T> class Matrix
{
private:
    T *_buf;
    unsigned _width;
    unsigned _height;
public:
    Matrix(unsigned x, unsigned y)
    {
        _width = x;
        _height = y;
        _buf = new T[x * y];
    }

    ~Matrix() { delete[] _buf; }
    unsigned width() const { return _width; }
    unsigned height() const { return _height; }
    T *buf() const;
    T at(unsigned x, unsigned y);
};

class Toolbox
{
public:
    static uint8_t numberOfLeadingZeros(uint32_t x);
};

#endif

