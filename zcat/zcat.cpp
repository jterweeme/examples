//This is a comment
//I love comments

#include <cassert>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>

class BitStream
{
    std::istream &_is;
    unsigned _bits = 0, _window = 0;
public:
    unsigned cnt = 0;
    BitStream(std::istream &is) : _is(is) { }

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
    std::ostream &_os;
public:
    unsigned n_bits() const { return _n_bits; }
    void reset() { _n_bits = 9; _dict.clear(); }

    LZW(unsigned maxbits, bool block_mode, char first, std::ostream &os)
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
        {
            auto foo = _dict[c - 256];
            stack.push_back(foo.second);
            c = foo.first;
        }

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
    std::ostream * const os = &std::cout;
    std::istream *is = &std::cin;
    std::ifstream ifs;

    if (argc > 1)
        ifs.open(argv[1]), is = &ifs;

    BitStream bis(*is);
    assert(bis.readBits(16) == 0x9d1f);
    const unsigned maxbits = bis.readBits(5);
    assert(maxbits <= 16);
    bis.readBits(2);
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
    ifs.close();
    return 0;
}


