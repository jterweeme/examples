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
class InStream
{
    int _fd;
    uint32_t _cap;
    uint32_t _head = 0, _tail = 0;
    uint8_t *_buf;
public:
    ~InStream() { delete[] _buf; }

    InStream(int fd, uint32_t capacity)
      :
        _fd(fd), _cap(capacity), _buf(new uint8_t[capacity])
    { }

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

class OutStream
{
    int _fd;
    uint32_t _cap;
    uint32_t _pos = 0;
    char *_buf;
public:
    OutStream(int fd, uint32_t capacity) : _fd(fd), _cap(capacity), _buf(new char[capacity]) { }
    ~OutStream() { delete[] _buf; }
    void put(char c) { if (_pos > _cap) flush(); _buf[_pos++] = c; }
    void flush() { ::write(_fd, _buf, _pos), _pos = 0; }
};
#else
typedef std::istream InStream;
typedef std::ostream OutStream;
#endif

class BitStream
{
    InStream &_is;
    unsigned _bits = 0, _window = 0;
public:
    unsigned cnt = 0;
    BitStream(InStream &is) : _is(is) { }

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

class LZW
{
    const unsigned _maxbits;
    unsigned _oldcode, _n_bits = 9;
    char _finchar;
    std::vector<std::pair<unsigned, char>> _dict;
    OutStream &_os;
public:
    unsigned n_bits() const { return _n_bits; }
    void reset() { _n_bits = 9; _dict.clear(); }

    LZW(unsigned maxbits, bool block_mode, char first, OutStream &os)
      : _maxbits(maxbits), _oldcode(first), _finchar(first), _os(os)
    {
        //why did they design it this way?
        if (block_mode)
            _dict.push_back(std::pair<unsigned, char>(0, 0));
    }

    inline void code(const unsigned in)
    {
        assert(in >= 0 && in <= 65535 && in <= _dict.size() + 256);
        auto c = in;
        std::vector<char> stack;

        if (c == _dict.size() + 256)
            stack.push_back(_finchar), c = _oldcode;

        while (c >= 256U)
            stack.push_back(_dict[c - 256].second), c = _dict[c - 256].first;

        _os.put(_finchar = c);

        while (stack.size())
            _os.put(stack.back()), stack.pop_back();

        if (_dict.size() + 256 < 1U << _maxbits)
            _dict.push_back(std::pair<unsigned, char>(_oldcode, _finchar));

        if (_n_bits < _maxbits && _dict.size() + 256 > (1U << _n_bits) - 1)
            ++_n_bits;

        _oldcode = in;
    }
};

int main(int argc, char **argv)
{
#if FAST
    OutStream ous(1, 8192);
    auto const os = &ous;
    int fd = argc > 1 ? ::open(argv[1], O_RDONLY) : 0; //stdin
    assert(fd != -1);
    InStream ins(fd, 8192);
    InStream *is = &ins;
#else
    auto const os = &std::cout;
    auto is = &std::cin;
    std::ifstream ifs;

    if (argc > 1)
        ifs.open(argv[1]), is = &ifs;
#endif
    BitStream bis(*is);
    assert(bis.readBits(16) == 0x9d1f);
    const unsigned maxbits = bis.readBits(7);
    assert(maxbits <= 16);
    const bool block_mode = bis.readBits(1) ? true : false;
    bis.cnt = 0; //counter moet op nul om later padding te berekenen

    int first = bis.readBits(9);
    assert(first >= 0 && first < 256);
    os->put(first);
    LZW lzw(maxbits, block_mode, first, *os);

    for (int code; (code = bis.readBits(lzw.n_bits())) != -1;)
    {
        if (code == 256 && block_mode)
        {
            //other max. bits not working yet :S
            assert(maxbits == 13 || maxbits == 15 || maxbits == 16);

            //padding?!
            for (const unsigned nb3 = lzw.n_bits() << 3; (bis.cnt - 1U + nb3) % nb3 != nb3 - 1U;)
                bis.readBits(lzw.n_bits());

            lzw.reset();
        }
        else
        {        
            lzw.code(code);
        }
    }

    os->flush();
#ifdef FAST
    ::close(fd);
#else
    ifs.close();
#endif
    return 0;
}


