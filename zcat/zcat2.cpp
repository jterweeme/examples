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

class LZW
{
    const unsigned _maxbits;
    const bool _block_mode;
    unsigned *_codetab;
    char *_htab;
    unsigned _oldcode;
    unsigned _free_ent;
    char _finchar;
public:
    LZW(unsigned maxbits, bool block_mode, char first)
      :
        _maxbits(maxbits),
        _block_mode(block_mode),
        _codetab(new unsigned[1 << maxbits]),
        _htab(new char[1 << maxbits]),
        _oldcode(first),
        _free_ent(block_mode ? 257 : 256)
    {
        std::iota(_htab, _htab + 256, 0);
    }

    ~LZW()
    {
        delete[] _codetab;
        delete[] _htab;
    }

    void reset()
    {
        std::fill(_codetab, _codetab + 256, 0);
        _free_ent = 256;
    }

    inline void code(const unsigned incode, unsigned &n_bits, std::ostream &os)
    {
        assert(incode >= 0 && incode <= 65535);
        assert(incode <= _free_ent);
        unsigned c = incode;
        std::vector<char> stack;

        if (c == _free_ent)
        {
            stack.push_back(_finchar);
            c = _oldcode;
        }

        while (c >= 256)
        {
            stack.push_back(_htab[c]);
            c = _codetab[c];
        }

        os.put(_finchar = _htab[c]);

        while (stack.size())
            os.put(stack.back()), stack.pop_back();

        c = _free_ent;

        if (c < 1U << _maxbits)
        {
            assert(c < 65536);
            _codetab[c] = _oldcode;
            _htab[c] = _finchar;
            _free_ent = c + 1U;
        }

        assert(_free_ent < 65537);

        if (_free_ent > (n_bits == _maxbits ? 1U << _maxbits : (1U << n_bits) - 1U))
            ++n_bits;

        _oldcode = incode;
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

    unsigned n_bits = 9;
    int first = bis.readBits(n_bits);
    assert(first >= 0 && first < 256);
    os->put(first);
    LZW lzw(maxbits, block_mode, first);

    for (int code; (code = bis.readBits(n_bits)) != -1;)
    {
        if (code == 256 && block_mode)
        {
            //padding?!
            assert(n_bits == 13 || n_bits == 15 || n_bits == 16);
            for (const unsigned nb3 = n_bits << 3; (bis.cnt - 1U + nb3) % nb3 != nb3 - 1U;)
                bis.readBits(n_bits);

            lzw.reset();
            n_bits = 9;
        }
        else
        {        
            lzw.code(code, n_bits, *os);
        }
    }

    os->flush();
    ifs.close();
    return 0;
}


