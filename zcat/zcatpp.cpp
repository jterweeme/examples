//This is a comment
//I love comments

//zcatpp (zcat c++)

#define FAST 1

#include <cassert>
#include <cstdint>
#include <vector>
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
    void put(char c) { if (_pos > _cap) flush(); _buf[_pos++] = c; }
    void flush() { ::write(_fd, _buf, _pos), _pos = 0; }
};

static istream cin(0, 8192);
static ostream cout(1, 8192);
#else
using std::istream;
using std::ostream;
using std::ifstream;
using std::cin;
using std::cout;
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

class PrintStack
{
    std::vector<char> _stack;
public:
    void push_back(char c) { _stack.push_back(c); }
    void print(ostream &os) { for (; _stack.size(); _stack.pop_back()) os.put(_stack.back()); }
};

class LZW
{
    const unsigned _maxbits;
    unsigned _oldcode = 0;
    char _finchar;
    std::vector<std::pair<unsigned, char>> _dict;
    ostream &_os;
    PrintStack _stack;
public:
    void clear_dict() { _dict.clear(); }

    LZW(unsigned maxbits, ostream &os)
      : _maxbits(maxbits), _os(os) { }

    inline void code(const unsigned in)
    {
        assert(in <= _dict.size() + 256);
        auto c = in;

        if (c == _dict.size() + 256)
            _stack.push_back(_finchar), c = _oldcode;

        for (; c >= 256U; c = _dict[c - 256].first)
            _stack.push_back(_dict[c - 256].second);

        _os.put(_finchar = c);

        if (_dict.size() + 256 < 1U << _maxbits)
            _dict.push_back(std::pair<unsigned, char>(_oldcode, _finchar));

        _oldcode = in;
        _stack.print(_os);
    }
};

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
    assert(maxbits <= 16);
    const bool block_mode = bis.readBits(1) ? true : false;
    bis.cnt = 0; //counter moet op nul om later padding te berekenen
    unsigned cnt = 0, nbits = 9;
    LZW lzw(maxbits, *os);

    for (int code; (code = bis.readBits(nbits)) != -1;)
    {
        //read 256 9-bit codes, 512 10-bit codes, 1024 11-bit codes, etc
        if (++cnt == 1U << nbits - 1U && nbits != maxbits)
            ++nbits, cnt = 0;

        if (code == 256 && block_mode)
        {
            //other max. bits not working yet :S
            assert(maxbits == 13 || maxbits == 15 || maxbits == 16);

            //cumbersome padding formula
            for (const unsigned nb3 = nbits << 3; (bis.cnt - 1U + nb3) % nb3 != nb3 - 1U;)
                bis.readBits(nbits);

            lzw.clear_dict(), cnt = 0, nbits = 9;
        }
        else
        {        
            lzw.code(code);
        }
    }

    os->flush();
    ifs.close();
    return 0;
}


