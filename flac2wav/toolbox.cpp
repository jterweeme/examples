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

void Toolbox::writeWLE(std::ostream &os, uint16_t w)
{
    os.put(w >> 0 & 0xff);
    os.put(w >> 8 & 0xff);
}

void Toolbox::writeDwLE(std::ostream &os, uint32_t dw)
{
    os.put(dw >>  0 & 0xff);
    os.put(dw >>  8 & 0xff);
    os.put(dw >> 16 & 0xff);
    os.put(dw >> 24 & 0xff);
}

BitInputStream::BitInputStream(std::istream *is) : _is(is)
{
}

void BitInputStream::alignToByte()
{
    _bitBufferLen -= _bitBufferLen % 8;
}

int BitInputStream::readByte()
{
    if (_bitBufferLen >= 8)
        return readUint(8);

    return _is->get();
}

bool BitInputStream::peek()
{
    if (_bitBufferLen > 0)
        return true;

    int temp = _is->get();

    if (temp == -1)
        return false;

    _bitBuffer = (_bitBuffer << 8) | temp;
    _bitBufferLen += 8;
    return true;
}

uint32_t BitInputStream::readUint(int n)
{
    while (_bitBufferLen < n)
    {
        int temp = _is->get();

        if (temp == -1)
            throw std::exception();

        _bitBuffer = (_bitBuffer << 8) | temp;
        _bitBufferLen += 8;
    }

    _bitBufferLen -= n;
    int result = int(_bitBuffer >> _bitBufferLen);

    if (n < 32)
        result &= (1 << n) - 1;

    return result;
}

int BitInputStream::readSignedInt(int n)
{
    return (readUint(n) << (32 - n)) >> (32 - n);
}

int64_t BitInputStream::readRiceSignedInt(int param)
{
    int64_t val = 0;

    while (readUint(1) == 0)
        ++val;

    val = (val << param) | readUint(param);
    return (val >> 1) ^ -(val & 1);
}


