#include <cassert>
#include <cstdint>
#include <numeric>
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
            _window = _window | c << _bits;
        }

        int ret = _window & (1 << n) - 1;
        _window = _window >> n, _bits -= n, cnt += n;
        return ret;
    }
};

class Stack
{
    std::vector<char> _stack;
public:
    void push(char c) { _stack.push_back(c); }

    void print(std::ostream &os)
    {
        while (_stack.size())
            os.put(_stack.back()), _stack.pop_back();
    }
};

class LZW
{
    const unsigned _maxbits;
    const bool _block_mode;
    unsigned _oldcode, _free_ent, *_codetab, _n_bits = 9;
    char _finchar, *_htab;
    Stack _stack;
public:
    void reset()
    {
        _free_ent = 256;
        _n_bits = 9;
    }
    unsigned n_bits() const { return _n_bits; }
    ~LZW() { delete[] _codetab; delete[] _htab; }
    LZW(unsigned maxbits, bool block_mode, char first)
      :
        _maxbits(maxbits),
        _block_mode(block_mode),
        _oldcode(first),
        _free_ent(block_mode ? 257 : 256),
        _codetab(new unsigned[1 << maxbits]),
        _finchar(first),
        _htab(new char[1 << maxbits])
    {
        std::iota(_htab, _htab + 256, 0);
    }

    inline void code(const unsigned in, std::ostream &os)
    {
        assert(in >= 0 && in <= 65535 && in <= _free_ent);
        unsigned c = in;

        if (c == _free_ent)
        {
            _stack.push(_finchar);
            c = _oldcode;
        }

        while (c >= 256U)
        {
            _stack.push(_htab[c]);
            c = _codetab[c - 256];
        }

        os.put(_finchar = _htab[c]);
        _stack.print(os);

        if (_free_ent < 1U << _maxbits)
        {
            _codetab[_free_ent - 256] = _oldcode;
            _htab[_free_ent++] = _finchar;
        }

        if (_free_ent > (_n_bits == _maxbits ? 1U << _maxbits : (1U << _n_bits) - 1U))
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
    LZW lzw(maxbits, block_mode, first);

    for (int code; (code = bis.readBits(lzw.n_bits())) != -1;)
    {
        if (code == 256 && block_mode)
        {
            //padding?!
            assert(lzw.n_bits() == 13 || lzw.n_bits() == 15 || lzw.n_bits() == 16);
            for (const unsigned nb3 = lzw.n_bits() << 3; (bis.cnt - 1U + nb3) % nb3 != nb3 - 1U;)
                bis.readBits(lzw.n_bits());

            lzw.reset();
        }
        else
        {        
            lzw.code(code, *os);
        }
    }

    os->flush();
    ifs.close();
    return 0;
}


