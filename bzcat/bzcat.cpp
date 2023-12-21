//This is a comment
//I love comments

#define FAST

#include <cstdint>
#include <cassert>
#ifdef FAST
#include <unistd.h>
#include <fcntl.h>
#else
#include <iostream>
#include <fstream>
#endif

#ifdef FAST
class istream
{
private:
    uint32_t _cap;
    uint32_t _head = 0, _tail = 0;
    uint8_t *_buf;
protected:
    int _fd;
public:
    ~istream() { delete[] _buf; }

    istream(int fd = -1, uint32_t capacity = 8192)
      : _cap(capacity), _buf(new uint8_t[capacity]), _fd(fd) { }

    int get()
    {
        if (_tail == _head)
        {
            ssize_t n = ::read(_fd, _buf, _cap);
            if (n < 1) return -1;
            _head = n;
            _tail = 0;
        }
        return _buf[_tail++];
    }
};

class ifstream : public istream
{
public:
    void close() { ::close(_fd); }
    void open(const char *fn) { _fd = ::open(fn, O_RDONLY); }
};

class ostream
{
public:
    virtual void put(char) { }
    virtual void flush() { }
    virtual ostream& operator<<(const char *) { return *this; }
};

class outstream : public ostream
{
    int _fd;
    uint32_t _cap;
    uint32_t _pos = 0;
    char *_buf;
public:
    ~outstream() { delete[] _buf; }
    void put(char c) override { if (_pos > _cap) flush(); _buf[_pos++] = c; }
    ostream& operator<<(const char *s) override { while (*s) put(*s++); return *this; }
    void flush() { ::write(_fd, _buf, _pos), _pos = 0; }

    outstream(int fd, uint32_t capacity = 8192)
      : _fd(fd), _cap(capacity), _buf(new char[capacity]) { }
};

class NullStream : public ostream
{
public:
    ostream& operator<<(const char *) override { return *this; }
};

static istream cin(0, 8192);
static outstream cout(1, 8192);
static outstream cerr(2, 8192);
static NullStream nullstream;
#else
class nullbuf : public std::streambuf
{
public:
    int overflow(int c) override { return c; }
};

using std::istream;
using std::ostream;
using std::ifstream;
using std::cin;
using std::cout;
using std::cerr;
static nullbuf nb;
static std::ostream nullstream(&nb);
#endif

class Toolbox
{
public:
    template <class T> static T min(T a, T b) { return b < a ? b : a; }
    template <class T> static T max(T a, T b) { return a < b ? b : a; }

    static char nibble(uint8_t n)
    { return n <= 9 ? '0' + char(n) : 'a' + char(n - 10); }

    static void hex32(unsigned dw, ostream &os)
    { for (unsigned i = 0; i <= 28; i += 4) os.put(nibble(dw >> 28 - i & 0xf)); }
};

class BitInputStream
{
    istream &_is;
    unsigned _window = 0, _bitCount = 0;
public:
    BitInputStream(istream &is) : _is(is) { }

    unsigned readBits24(unsigned n)
    {
        assert(n <= 24);
        while (_bitCount < n)
            _window = _window << 8 | _is.get(), _bitCount += 8;
        _bitCount -= n;
        return _window >> _bitCount & (1 << n) - 1;
    }

    unsigned readBits32(unsigned n)
    {
        assert(n <= 32);
        unsigned ret = 0;
        while (n)
        {
            unsigned foo = Toolbox::min(16U, n);
            ret = ret << foo | readBits24(foo), n -= foo;
        }
        return ret;
    }
};

class MoveToFront
{
    uint8_t _buf[256];
public:
    MoveToFront()
    { for (unsigned i = 0; i < 256; ++i) _buf[i] = i; }

    uint8_t indexToFront(unsigned i)
    { uint8_t val = _buf[i]; for (; i; --i) _buf[i] = _buf[i - 1]; return _buf[0] = val; }
};

class CRC32
{
    uint32_t _table[256];
    uint32_t _crc = 0xffffffff;
public:
    void update(uint8_t c) { _crc = _crc << 8 ^ _table[(_crc >> 24 ^ c) & 0xff]; }
    uint32_t crc() const { return ~_crc; }

    CRC32()
    {
        //https://github.com/gcc-mirror/gcc/blob/master/libiberty/crc32.c
        for (uint32_t i = 0, j, c; i < 256; ++i)
        {
            for (c = i << 24, j = 8; j > 0; --j)
                c = c & 0x80000000 ? c << 1 ^ 0x04c11db7 : c << 1;
            _table[i] = c;
        }
    }
};

class Table
{
    uint8_t _codeLengths[258];
    unsigned _pos = 0;
    unsigned _bases[25] = {0};
    unsigned _limits[24] = {0};
    unsigned _symbols[258] = {0};
    uint8_t _minLen = 23;
    uint8_t _maxLen = 0;
public:
    void read(BitInputStream &bis, unsigned symbolCount);
    uint8_t minLength() const { return _minLen; }
    uint32_t limit(uint8_t i) const { return _limits[i]; }
    uint32_t symbol(uint16_t i) const { return _symbols[i]; }
    uint32_t base(uint8_t i) const { return _bases[i]; }
};

//read the canonical Huffman code lengths for table
void Table::read(BitInputStream &bis, unsigned symbolCount)
{
    for (unsigned i = 0, c = bis.readBits24(5); i <= symbolCount + 1; ++i)
    {
        while (bis.readBits24(1))
            c += bis.readBits24(1) ? -1 : 1;
        _codeLengths[_pos++] = c;
    }

    for (unsigned i = 0; i < symbolCount + 2; ++i)
        _bases[_codeLengths[i] + 1]++;

    for (unsigned i = 1; i < 25; ++i)
        _bases[i] += _bases[i - 1];

    for (unsigned i = 0; i < symbolCount + 2; ++i)
    {
        _minLen = Toolbox::min(_codeLengths[i], _minLen);
        _maxLen = Toolbox::max(_codeLengths[i], _maxLen);
    }

    for (unsigned i = _minLen, code = 0; i <= _maxLen; ++i)
    {
        unsigned base = code;
        code += _bases[i + 1] - _bases[i];
        _bases[i] = base - _bases[i];
        _limits[i] = code - 1;
        code <<= 1;
    }

    for (unsigned i = 0, minLen = _minLen; minLen <= _maxLen; ++minLen)
        for (unsigned symbol = 0; symbol < symbolCount + 2; ++symbol)
            if (_codeLengths[symbol] == minLen)
                _symbols[i++] = symbol;
}

class Tables
{
    Table _tables[6];
    uint8_t *_selectors;
    uint32_t grpIdx, grpPos, curTbl;
public:
    ~Tables() { delete[] _selectors; }
    void read(BitInputStream &bis, uint32_t symbolCount);
    uint32_t nextSymbol(BitInputStream &bis);
};

void Tables::read(BitInputStream &bis, uint32_t symbolCount)
{
    uint8_t nTables = bis.readBits24(3);
    uint16_t nSelectors = bis.readBits24(15);
    _selectors = new uint8_t[nSelectors];
    MoveToFront tableMTF;
    
    for (uint32_t i = 0; i < nSelectors; ++i)
    {
        uint8_t u = 0;

        while (bis.readBits24(1))
            ++u;

        _selectors[i] = tableMTF.indexToFront(u);
    }

    for (uint32_t t = 0; t < nTables; ++t)
        _tables[t].read(bis, symbolCount);

    curTbl = _selectors[0], grpIdx = 0, grpPos = 0;
}

uint32_t Tables::nextSymbol(BitInputStream &bis)
{
    if (grpPos++ % 50 == 0)
        curTbl = _selectors[grpIdx++];

    unsigned i = _tables[curTbl].minLength();
    unsigned codeBits = bis.readBits24(i);

    for (;i <= 23; ++i)
    {
        if (codeBits <= _tables[curTbl].limit(i))
            return _tables[curTbl].symbol(codeBits - _tables[curTbl].base(i));

        codeBits = codeBits << 1 | bis.readBits24(1);
    }

    return 0;
}

class Block
{
    CRC32 _crc;
    uint32_t _dec = 0, _curp = 0, *_merged;
    uint8_t _nextByte();
public:
    uint32_t process(BitInputStream &bi, uint32_t blockSize, ostream &os);
};

uint8_t Block::_nextByte()
{       
    uint8_t ret = _curp & 0xff;
    _curp = _merged[_curp >> 8];
    ++_dec;
    return ret;
}

uint32_t Block::process(BitInputStream &bi, uint32_t blockSize, ostream &os)
{
    uint32_t _blockCRC = bi.readBits32(32);
    assert(bi.readBits24(1) == 0);
    unsigned bwtStartPointer = bi.readBits24(24), symbolCount = 0;
    unsigned bwtByteCounts[256] = {0};
    uint8_t symbolMap[256] = {0};

    for (unsigned i = 0, ranges = bi.readBits24(16); i < 16; ++i)
        if ((ranges & 1 << 15 >> i) != 0)
            for (unsigned j = 0, k = i << 4; j < 16; ++j, ++k)
                if (bi.readBits24(1))
                    symbolMap[symbolCount++] = uint8_t(k);

    Tables tables;
    tables.read(bi, symbolCount);
    uint8_t bwtBlock[blockSize], mtfValue = 0;
    MoveToFront symbolMTF;
    uint32_t _length = 0;

    for (unsigned n = 0, inc = 1;;)
    {
        unsigned nextSymbol = tables.nextSymbol(bi);

        if (nextSymbol == 0)
        {
            n += inc;
            inc <<= 1;
            continue;
        }

        if (nextSymbol == 1)
        {
            n += inc << 1;
            inc <<= 1;
            continue;
        }

        if (n > 0)
        {
            uint8_t nextByte = symbolMap[mtfValue];
            bwtByteCounts[nextByte] += n;
            ++n, inc = 1;

            while (--n >= 1)
                bwtBlock[_length++] = nextByte;
        }

        //end of block
        if (nextSymbol == symbolCount + 1)
            break;

        mtfValue = symbolMTF.indexToFront(nextSymbol - 1);
        uint8_t nextByte = symbolMap[mtfValue];
        bwtByteCounts[nextByte]++;
        bwtBlock[_length++] = nextByte;
    }

    _merged = new uint32_t[_length];
    unsigned characterBase[256] = {0};

    for (unsigned i = 0; i < 255; ++i)
        characterBase[i + 1] = bwtByteCounts[i];

    for (unsigned i = 2; i <= 255; ++i)
        characterBase[i] += characterBase[i - 1];

    for (unsigned i = 0; i < _length; ++i)
    {
        unsigned val = bwtBlock[i] & 0xff;
        _merged[characterBase[val]++] = (i << 8) | val;
    }

    _curp = _merged[bwtStartPointer];
    unsigned repeat = 0, acc = 0;
    int last = -1;

    while (true)
    {
        if (repeat < 1)
        {
            if (_dec == _length)
                break;

            uint8_t nextByte = _nextByte();

            if (nextByte != last)
            {
                last = nextByte, repeat = 1, acc = 1;
                _crc.update(nextByte);
            }
            else if (++acc == 4)
            {
                repeat = _nextByte() + 1, acc = 0;

                for (unsigned i = 0; i < repeat; ++i)
                    _crc.update(nextByte);
            }
            else
            {
                repeat = 1;
                _crc.update(nextByte);
            }
        }

        --repeat;
        os.put(last);
    }

    os.flush();
    delete[] _merged;
    assert(_blockCRC == _crc.crc());
    return _crc.crc();
}

int main(int argc, char **argv)
{
    static constexpr bool quiet = true;
    istream *is = &cin;
    ostream *os = &cout;
    ostream *msg = &cerr;
    ifstream ifs;

    if (quiet)
        msg = &nullstream;

    if (argc == 2)
        ifs.open(argv[1]), is = &ifs;

    BitInputStream bi(*is);
    assert(bi.readBits24(16) == 0x425a);
    bi.readBits24(8);
    uint8_t blockSize = bi.readBits24(8) - '0';
    uint32_t streamCRC = 0;

    while (true)
    {
        uint32_t marker1 = bi.readBits24(24), marker2 = bi.readBits24(24);

        if (marker1 == 0x314159 && marker2 == 0x265359)
        {
            Block b;
            uint32_t blockCRC = b.process(bi, blockSize * 100000, *os);
            streamCRC = (streamCRC << 1 | streamCRC >> 31) ^ blockCRC;
            continue;
        }

        if (marker1 == 0x177245 && marker2 == 0x385090)
        {
            uint32_t crc = bi.readBits32(32);
            assert(crc == streamCRC);
            *msg << "0x";
            Toolbox::hex32(crc, *msg);
            *msg << " 0x";
            Toolbox::hex32(streamCRC, *msg);
            *msg << "\r\n";
            msg->flush();
            break;
        }

        assert(false);
    }
    ifs.close();
    return 0;
}

