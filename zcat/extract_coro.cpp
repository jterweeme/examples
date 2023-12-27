//This is a comment
//I love comments

#define FAST

#include <cassert>
#include <cstdint>
#include <boost/coroutine2/all.hpp>

using boost::coroutines2::coroutine;
using boost::coroutines2::fixedsize_stack;

#ifdef FAST
#include <unistd.h>
#include <fcntl.h>

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
#else
#include <iostream>
#include <fstream>

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

static istream *is = &cin;

static void codes(coroutine<unsigned>::push_type &yield)
{
    BitStream bis(*is);
    unsigned cnt = 0, nbits = 9, maxbits = 16;
    cerr << "debug bericht\r\n";

    for (int code; (code = bis.readBits(nbits)) != -1;)
    {
        if (++cnt == 1U << nbits - 1 && nbits != maxbits)
            ++nbits, cnt = 0;

        if (code == 256)
        {
            for (const unsigned nb3 = nbits << 3; (bis.cnt() - 1U + nb3) % nb3 != nb3 - 1U;)
                bis.readBits(nbits);

            cnt = 0, nbits = 9;
        }

        yield(code);
    }
}

int main(int argc, char **argv)
{
    ostream *os = &cout;
    ifstream ifs;

    if (argc > 1)
        ifs.open(argv[1]), is = &ifs;

    assert(is->get() == 0x1f);
    assert(is->get() == 0x9d);
    int c = is->get();
    assert(c >= 0 && c & 0x80);   //block mode bit is hardcoded in ncompress
    coroutine<unsigned>::pull_type codes(fixedsize_stack(), ::codes);

    for (auto code : codes)
        *os << code << "\r\n";

    os->flush();
    ifs.close();
    return 0;
}



