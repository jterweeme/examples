//This is a comment
//I love comments


#include <cassert>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>

namespace fast
{

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
    int _fd;
    uint32_t _cap;
    uint32_t _pos = 0;
    char *_buf;
public:
    ostream(int fd, uint32_t capacity) : _fd(fd), _cap(capacity), _buf(new char[capacity]) { }
    ~ostream() { delete[] _buf; }
    inline void put(char c) { if (_pos > _cap) flush(); _buf[_pos++] = c; }
    void flush() { ::write(_fd, _buf, _pos), _pos = 0; }
    inline ostream& operator<<(const char *s) { while (*s) put(*s++); return *this; }

    inline ostream& operator<<(unsigned n)
    {
        if (n == 0) { put('0'); return *this; }
        char buf[100];
        unsigned i = 0;
        for (; n; n = n / 10)
            buf[i++] = n % 10 + '0';
        while (i)
            put(buf[--i]);
        return *this;
    }
};

static istream cin(0, 8192);
static ostream cout(1, 8192);
static ostream cerr(2, 8192);
}

#if 0
using fast::istream;
using fast::ostream;
using fast::ifstream;
using fast::cin;
using fast::cout;
using fast::cerr;
#else
using std::istream;
using std::ostream;
using std::ifstream;
using std::cin;
using std::cout;
using std::cerr;
#endif

class BitStream
{
    istream &_is;
    unsigned _bits = 0, _window = 0, _cnt = 0;
public:
    BitStream(istream &is) : _is(is) { }
    unsigned cnt() const { return _cnt; }

    int readBits(unsigned n)
    {
        for (; _bits < n; _bits += 8)
        {
            int c = _is.get();
            if (c == -1) return -1;
            _window |= c << _bits;
        }

        int ret = _window & (1 << n) - 1;
        _window >>= n, _bits -= n, _cnt += n;
        return ret;
    }
};

class Codes
{
    BitStream _bis;
    const unsigned _maxbits;
    unsigned _cnt = 0, _nbits = 9;
public:
    Codes(istream &is, unsigned maxbits) : _bis(is), _maxbits(maxbits)
    {
        assert(maxbits >= 9 && maxbits <= 16);
    }

    int extract()
    {
        int code = _bis.readBits(_nbits);

        //read 256 9-bit codes, 512 10-bit codes, 1024 11-bit codes, etc
        if (++_cnt == 1U << _nbits - 1U && _nbits != _maxbits)
            ++_nbits, _cnt = 0;

        if (code == 256)
        {
            //other max. bits not working yet :S
            assert(_maxbits == 13 || _maxbits == 15 || _maxbits == 16);

            //cumbersome padding formula
            for (const unsigned nb3 = _nbits << 3; (_bis.cnt() - 1U + nb3) % nb3 != nb3 - 1U;)
                _bis.readBits(_nbits);

            _cnt = 0, _nbits = 9;
        }

        return code;
    }
};

#if 0
int main(int argc, char **argv)
{
    istream *is = &cin;
    ostream *os = &cout;
    ifstream ifs;

    if (argc > 1)
        ifs.open(argv[1]), is = &ifs;

    assert(is->get() == 0x1f);
    assert(is->get() == 0x9d);
    int c = is->get();
    assert(c >= 0 && c & 0x80);   //block mode bit is hardcoded in ncompress
    Codes codes(*is, c & 0x7f);

    for (int code; (code = codes.extract()) != -1;)
        *os << code << "\r\n";

    os->flush();
    ifs.close();
    return 0;
}
#else
int main(int argc, char **argv)
{
    istream *is = &cin;
    ostream *os = &cout;
    ifstream ifs;
    
    if (argc > 1)
        ifs.open(argv[1]), is = &ifs;

    assert(is->get() == 0x1f);
    assert(is->get() == 0x9d);
    int c = is->get();
    assert(c != -1 && c & 0x80);
    const unsigned maxbits = c & 0x7f;
    assert(maxbits >= 9 && maxbits <= 16);
    unsigned nbits = 9;

    uint8_t buf[512];

    unsigned ncodes2 = 0;

    while (true)
    {
        is->read((char *)buf, 32 * nbits);

        unsigned ncodes = is->gcount() * 8 / nbits;

        if (ncodes <= 0)
            break;

        cerr << ncodes << " " << nbits << "-bit codes\r\n";
        unsigned bits = 0;

        for (unsigned i = 0; ncodes--; ++i)
        {
            unsigned byte = bits / 8;
            unsigned shift = (i * (nbits - 8)) % 8;
            unsigned *window = (unsigned *)(buf + byte);
            unsigned code = *window >> shift & (1 << nbits) - 1;
            *os << code << "\r\n";
            bits += nbits;
            ++ncodes2;
        }

        if (ncodes2 == 1U << nbits - 1U)
            ++nbits,ncodes2 = 0;
    }

    return 0;
}
#endif


