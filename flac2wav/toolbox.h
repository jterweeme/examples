//file: toolbox.h

#ifndef TOOLBOX_H
#define TOOLBOX_H

#include <cstdint>

class Matrix
{
private:
    int *_buf;
    unsigned _width;
    unsigned _height;
public:
    Matrix(unsigned x, unsigned y);
    ~Matrix();
    unsigned width() const;
    unsigned height() const;
    int *buf() const;
    int at(unsigned x, unsigned y);
};

class Toolbox
{
public:
    static uint8_t numberOfLeadingZeros(uint32_t x);
};

#endif

