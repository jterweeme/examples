//This is a comment
//I love comments

//zcatpp (zcat c++)

#include "generator.h"
#include <cassert>
#include <cstdint>
#include <coroutine>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>

using std::vector;

namespace fast
{
class istream
{
private:
    uint32_t _cap, _head = 0, _tail = 0;
    uint8_t *_buf;
    ssize_t _gcount = -1;
protected:
    int _fd;
public:
    ~istream() { delete[] _buf; }
    ssize_t gcount() const { return _gcount; }

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

    void read(char *buf, unsigned n)
    {
        _gcount = 0;

        for (int c; n-- && (c = get()) != -1;)
            buf[_gcount++] = c;
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
    uint32_t _cap, _pos = 0;
    char *_buf;
public:
    ostream(int fd, uint32_t capacity) : _fd(fd), _cap(capacity), _buf(new char[capacity]) { }
    ~ostream() { delete[] _buf; }
    void put(char c) { if (_pos > _cap) flush(); _buf[_pos++] = c; }
    void flush() { ::write(_fd, _buf, _pos), _pos = 0; }
};

static istream cin(0, 8192);
static ostream cout(1, 8192);
static ostream cerr(1, 8192);
}

using fast::ostream;
using fast::istream;
using fast::ifstream;
using fast::cin;
using fast::cout;
using fast::cerr;

class ByteStack
{
    vector<char> _stack;
public:
    void push(char c) { _stack.push_back(c); }
    char top() const { return _stack.back(); }
    void pop_all(ostream &os) { for (; _stack.size(); _stack.pop_back()) os.put(top()); }
};

class Dictionary
{
    unsigned _cap;
    uint16_t *_codes;
    char *_bytes;
    unsigned _pos = 0;
public:
    Dictionary(unsigned cap)
      : _cap(cap), _codes(new uint16_t[cap - 256]), _bytes(new char[cap - 256]) { }

    void lookup(ByteStack &s, uint16_t code)
    {
        for (; code >= 256U; code = _codes[code - 256])
            s.push(_bytes[code - 256]);

        s.push(code);
    }

    void store(unsigned code, char c)
    {
        if (_pos + 256 < _cap)
            _codes[_pos] = code, _bytes[_pos] = c, ++_pos;
    }

    ~Dictionary() { delete[] _codes; delete[] _bytes; }
    auto size() const { return _pos + 256; }
    void clear() { _pos = 0; }
};

static Generator<unsigned>
codes(istream &is, unsigned bitdepth)
{
    char buf[20];
start_block:
    for (unsigned nbits = 9; nbits <= bitdepth; ++nbits)
    {
        for (unsigned i = 0; i < 1U << nbits - 1 || nbits == bitdepth;)
        {
            is.read(buf, nbits);
            unsigned ncodes = is.gcount() * 8 / nbits;

            if (ncodes == 0)
                co_return;
            
            for (unsigned bits = 0, j = 0; ncodes--; bits += nbits, ++i, ++j)
            {
                unsigned *window = (unsigned *)(buf + bits / 8);
                unsigned code = *window >> j * (nbits - 8) % 8 & (1 << nbits) - 1;
                co_yield code;

                if (code == 256)
                    goto start_block;
            }
        }
    }
}

static void
lzw(unsigned dictcap, ostream &os, Generator<unsigned> codes)
{
    Dictionary dict(dictcap);
    ByteStack stack;
    unsigned oldcode = 0;
    char finchar = 0;

    while (codes)
    {
        unsigned newcode, c;
        newcode = c = codes();
        assert(c <= dict.size());
        
        if (c == 256)
        {
            dict.clear();
            continue;
        }

        if (c == dict.size())
            stack.push(finchar), c = oldcode;

        dict.lookup(stack, c);
        dict.store(oldcode, finchar = stack.top());
        oldcode = newcode;
        stack.pop_all(os);
    }
}

int
main(int argc, char **argv)
{
    istream *is = &cin;
    ostream * const os = &cout;
    ifstream ifs;

    if (argc > 1)
        ifs.open(argv[1]), is = &ifs;

    assert(is->get() == 0x1f && is->get() == 0x9d); //magic
    int c = is->get();
    assert(c >= 0 && c & 0x80);   //block mode bit is hardcoded in ncompress
    const unsigned bitdepth = c & 0x7f;
    ::lzw(1 << bitdepth, *os, codes(*is, bitdepth));
    os->flush();
    ifs.close();
    return 0;
}


