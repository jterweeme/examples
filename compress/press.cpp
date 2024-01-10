#include "generator.h"
#include <iostream>
#include <cstdint>
#include <cassert>
#include <unistd.h>
#include <fcntl.h>

namespace my
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
    istream &read(char *s, size_t n) { assert(false); }
    size_t gcount() const { assert(false); return 0; }

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
};

static istream cin(0, 8192);
static ostream cout(1, 8192);
static ostream cerr(2, 8192);
}

using std::istream;
using std::ostream;
using std::cin;
using std::cout;
using std::cerr;
using std::fill;

static Generator<unsigned> codes(istream &is)
{
    unsigned n = 0;
    bool flag = false;

    for (int c; (c = is.get()) != -1;)
    {
        if (isdigit(c))
        {
            flag = true;
            n = n * 10 + c - '0';
        }
        else
        {
            if (flag)
                co_yield n;

            flag = false;
            n = 0;
        }
    }
}

int main()
{
    ostream *os = &cout;
    os->put(0x1f);
    os->put(0x9d);
    os->put(0x90);
    unsigned cnt = 0, nbits = 9;
    const unsigned bitdepth = 16;
    char buf[20] = {0};
    
    for (auto codes = ::codes(cin); codes;)
    {
        unsigned code = codes();
        unsigned *window = (unsigned *)(buf + nbits * (cnt % 8) / 8);
        *window |= code << (cnt % 8) * (nbits - 8) % 8;
        ++cnt;

        if (cnt % 8 == 0 || code == 256)
        {
            os->write(buf, nbits);
            fill(buf, buf + sizeof(buf), 0);
        }

        if (code == 256)
            nbits = 9, cnt = 0;
        
        if (nbits != bitdepth && cnt == 1U << nbits - 1)
            ++nbits, cnt = 0;
    }

    auto dv = std::div((cnt % 8) * nbits, 8);
    os->write(buf, dv.quot + (dv.rem ? 1 : 0));
    os->flush();
    return 0;
}


