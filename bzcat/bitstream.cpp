#include "bitstream.h"

bool BitInputBase::readBool()
{
    return readBit() == 1;
}

void BitInputBase::ignore(int n)
{
    while (n--)
        readBool();
}

uint32_t BitInputBase::readUInt32()
{
    return readBits(16) << 16 | readBits(16);
}

uint32_t BitInputBase::readInt()
{
    return readUInt32();
}

uint32_t BitInputBase::readUnary()
{
    uint32_t u = 0;
    while (readBool()) u++;
    return u;
}

uint32_t BitInput::readBits(uint32_t count)
{
    if (count > 24)
        throw "Maximaal 24 bits";

    for (; _bitCount < count; _bitCount += 8)
        _bitBuffer = _bitBuffer << 8 | _getc();

    _bitCount -= count;
    return _bitBuffer >> _bitCount & ((1 << count) - 1);
}

uint32_t BitInput::readBit()
{
    return readBits(1);
}

BitInputStream::BitInputStream(std::istream *is) : _is(is)
{

}

int BitInputStream::_getc()
{
    return _is->get();
}



