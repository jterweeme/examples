//This is a comment
//I love comments

//zcatpp (zcat c++)

//#define FAST

#include <cassert>
#include <cstdint>
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
    unsigned _bits = 0, _window = 0;
public:
    unsigned cnt = 0;
    BitStream(istream &is) : _is(is) { }

    int readBits(unsigned n)
    {
        for (; _bits < n; _bits += 8)
        {
            int c = _is.get();
            if (c == -1) return -1;
            _window |= c << _bits;
        }

        int ret = _window & (1 << n) - 1;
        _window >>= n, _bits -= n, cnt += n;
        return ret;
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

    BitStream bis(*is);
    assert(bis.readBits(16) == 0x9d1f);
    const unsigned maxbits = bis.readBits(7);
    assert(maxbits >= 9 && maxbits <= 16);
    assert(bis.readBits(1) == 1); //block mode is hardcoded 1 in ncompress
    bis.cnt = 0; //reset counter needed for cumbersome padding formula
    unsigned cnt = 0, nbits = 9;

    for (int code; (code = bis.readBits(nbits)) != -1;)
    {
        //read 256 9-bit codes, 512 10-bit codes, 1024 11-bit codes, etc
        if (++cnt == 1U << nbits - 1U && nbits != maxbits)
            ++nbits, cnt = 0;

        if (code == 256)
        {
            //other max. bits not working yet :S
            assert(maxbits == 13 || maxbits == 15 || maxbits == 16);

            //cumbersome padding formula
            for (const unsigned nb3 = nbits << 3; (bis.cnt - 1U + nb3) % nb3 != nb3 - 1U;)
                bis.readBits(nbits);

            cnt = 0, nbits = 9;
        }

        cout << code << "\n";
    }

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
    assert(c != -1 && c & 80);
    const unsigned maxbits = c & 0x7f;
    assert(maxbits >= 9 && maxbits <= 16);
    cerr << maxbits << "\r\n";

    

    //9*8 = 8*9 = 72
    //256*9 = 288*8

    //10*4 = 8*5 = 40
    //512*10 = 640*8

    //11*8 = 8*11 88
    //1024*11=1408*8

    //12*2 = 8*3 = 24
    //2048*12=3072*8

    //13*8 = 8*13 = 104
    //4096*13=6656*8

    //14*4 = 8*7 = 56
    //8192*14=14336*8

    //15*8 = 8*15 = 120
    //16384*15=30720*8

    //16*1 = 8*2 = 16
    return 0;
}
#endif


