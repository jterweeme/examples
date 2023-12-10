#include <cassert>
#include <cstdint>
#include <numeric>
#include <iostream>
#include <fstream>
#include <vector>

class BitStream
{
    std::istream &_is;
    uint32_t _bits = 0, _window = 0;
public:
    uint32_t cnt = 0;
    BitStream(std::istream &is) : _is(is) { }

    int32_t readBits(uint8_t n)
    {
        for (; _bits < n; _bits += 8)
        {
            int c = _is.get();
            if (c == -1) return -1;
            _window = _window | c << _bits;
        }

        int32_t ret = _window & (1 << n) - 1;
        _window = _window >> n, _bits -= n, cnt += n;
        return ret;
    }
};

class LZW
{
    std::ostream &_os;
    const uint8_t _maxbits;
    const bool _block_mode;
    uint16_t *_codetab;
    char *_htab;
    int32_t _oldcode;
    int32_t _free_ent;
    char _finchar;
public:
    LZW(std::ostream &os, uint8_t maxbits, bool block_mode, char first)
      :
        _os(os),
        _maxbits(maxbits),
        _block_mode(block_mode),
        _codetab(new uint16_t[1 << maxbits]),
        _htab(new char[1 << maxbits]),
        _oldcode(first),
        _free_ent(block_mode ? 257 : 256)
    {
        std::fill(_codetab, _codetab + 256, 0);
        std::iota(_htab, _htab + 256, 0);
        os.put(_finchar = first);
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

    void code(uint16_t c, uint8_t &n_bits)
    {
        assert(c <= _free_ent);
        uint16_t incode = c;
        std::vector<char> stack;

        if (c == _free_ent)
            stack.push_back(_finchar), c = _oldcode;

        while (c >= 256)
            stack.push_back(_htab[c]), c = _codetab[c];

        _os.put(_finchar = _htab[c]);

        while (stack.size())
            _os.put(stack.back()), stack.pop_back();

        if ((c = _free_ent) < 1 << _maxbits)
            _codetab[c] = uint16_t(_oldcode), _htab[c] = _finchar, _free_ent = c + 1;

        if (_free_ent > (n_bits == _maxbits ? 1 << _maxbits : (1 << n_bits) - 1))
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
    const uint8_t maxbits = bis.readBits(5);
    assert(maxbits <= 16);
    bis.readBits(2);
    const bool block_mode = bis.readBits(1) ? true : false;
    bis.cnt = 0; //counter moet op nul om later padding te berekenen

    uint8_t n_bits = 9;
    int32_t code = bis.readBits(n_bits);
    assert(code >= 0 && code < 256);
    LZW lzw(*os, maxbits, block_mode, code);

    while ((code = bis.readBits(n_bits)) != -1)
    {
        if (code == 256 && block_mode)
        {
            //padding?!
            assert(n_bits == 13 || n_bits == 15 || n_bits == 16);
            for (const uint8_t nb3 = n_bits << 3; (bis.cnt - 1U + nb3) % nb3 != nb3 - 1U;)
                bis.readBits(n_bits);

            lzw.reset();
            n_bits = 9;
            continue;
        }
        
        lzw.code(code, n_bits);
    }

    os->flush();
    ifs.close();
    return 0;
}


