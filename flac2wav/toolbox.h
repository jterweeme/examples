//file: toolbox.h

#ifndef TOOLBOX_H
#define TOOLBOX_H

#include <istream>
#include <cstdint>

template <class T> class SimpleVector
{
private:
    T *_buf;
    unsigned _length;
public:
    unsigned length() const
    {
        return _length;
    }
};

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

    ~Matrix()
    {
        delete[] _buf;
    }

    unsigned width() const
    {
        return _width;
    }

    unsigned height() const
    {
        return _height;
    }

    T at(unsigned x, unsigned y) const
    {
        return *(_buf + x * _width + y);
    }

    void set(unsigned x, unsigned y, T value)
    {
        *(_buf + x * _width + y) = value;
    }
};

class Toolbox
{
public:
    static uint8_t numberOfLeadingZeros(uint32_t x);
    static void writeWLE(std::ostream &os, uint16_t w);
    static void writeDwLE(std::ostream &os, uint32_t dw);
};

class BitInputStream
{
private:
    std::istream *_is;
    long _bitBuffer;
    int _bitBufferLen;
public:
    BitInputStream(std::istream *is);
    void alignToByte();
    bool peek();
    uint32_t readUint(int n);
    int readByte();
    int readSignedInt(int n);
    int64_t readRiceSignedInt(int param);
};
#endif


