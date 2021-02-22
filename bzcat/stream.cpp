#include "stream.h"

DecStream::DecStream(BitInputBase *bi) : _bi(bi)
{
    _bi->ignore(32);
}

#if 0
int DecStreamBuf::underflow()
{
    if (gptr() < egptr())
        return (int)(*gptr());

    char *base = _buffer;
    char *start = base;

    if (eback() == base)
    {
        memmove(base, egptr() - _put_back, _put_back);
        start += _put_back;
    }

    _lastRead2 = _ds.read(start, 264 - (start - base));
    _pos2 += _lastRead2;
    if (_lastRead2 == 0) return EOF;
    setg(base, start, start + _lastRead2);
    return (uint8_t)(*gptr());
}

std::streampos DecStreamBuf::seekoff(std::ios::streamoff off, std::ios::seekdir way, std::ios::openmode m)
{
    std::streampos pos = _pos2 - ((uint64_t)egptr() - (uint64_t)gptr());
    return pos;
}
#endif

int DecStream::read(char *buf, int n)
{
    int i = 0;
    for (i = 0; i < n; i++)
    {
        int c = read();

        if (c == -1)
            return i;

        buf[i] = c;
    }
    return i;
}

int DecStream::read()
{
    int nextByte = _bd.read();
    return nextByte == -1 && _initNextBlock() ? _bd.read() : nextByte;
}

bool DecStream::_initNextBlock()
{
    if (_streamComplete)
        return false;

    uint32_t marker1 = _bi->readBits(24), marker2 = _bi->readBits(24);

    if (marker1 == 0x314159 && marker2 == 0x265359)
    {
        _bd.init(_bi);
        return true;
    }
    else if (marker1 == 0x177245 && marker2 == 0x385090)
    {
        _streamComplete = true;
        _bi->readInt();
        return false;
    }

    _streamComplete = true;
    throw "BZip2 stream format error";
}

void DecStream::extractTo(std::ostream &os)
{
    int b;

    while ((b = read()) != -1)
        os.put(b);
}

