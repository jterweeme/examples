//file: toolbox.cpp

#include "toolbox.h"

uint8_t Toolbox::numberOfLeadingZeros(uint32_t x)
{
    // Keep shifting x by one until leftmost bit
    // does not become 1.
    int total_bits = sizeof(x) * 8;
    int res = 0;

    while ( !(x & (1 << (total_bits - 1))) )
    {
        x = (x << 1);
        res++;
    }

    return res;
}

Matrix::Matrix(unsigned x, unsigned y)
{
    _width = x;
    _height = y;
    _buf = new int[x * y];
}

Matrix::~Matrix()
{
    delete[] _buf;
}

unsigned Matrix::width() const
{
    return _width;
}

unsigned Matrix::height() const
{
    return _height;
}

