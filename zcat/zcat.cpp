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
    uint8_t n_bits = 9;
    int32_t free_ent = block_mode ? 257 : 256, code, oldcode, incode;
    uint16_t codetab[1 << maxbits];
    std::fill(codetab, codetab + 256, 0);
    bis.cnt = 0; //counter moet op nul om later padding te berekenen
    code = oldcode = bis.readBits(n_bits);
    assert(code >= 0 && code < 256);
    char finchar = oldcode, htab[1 << maxbits];
    os->put(finchar);
    std::iota(htab, htab + 256, 0);
    std::vector<char> stack;

    while ((code = incode = bis.readBits(n_bits)) != -1)
    {
        if (code == 256 && block_mode)
        {
            //padding?!
            assert(n_bits == 13 || n_bits == 15 || n_bits == 16);
            for (const uint8_t nb3 = n_bits << 3; (bis.cnt - 1U + nb3) % nb3 != nb3 - 1U;)
                bis.readBits(n_bits);

            std::fill(codetab, codetab + 256, 0), free_ent = 256, n_bits = 9;
            continue;
        }

        assert(code <= free_ent);

        if (code == free_ent)
            stack.push_back(finchar), code = oldcode;

        while (code >= 256)
            stack.push_back(htab[code]), code = codetab[code];

        os->put(finchar = htab[code]);

        while (stack.size())
            os->put(stack.back()), stack.pop_back();
    
        if ((code = free_ent) < 1 << maxbits)
            codetab[code] = uint16_t(oldcode), htab[code] = finchar, free_ent = code + 1;
    
        oldcode = incode;

        if (free_ent > (n_bits == maxbits ? 1 << maxbits : (1 << n_bits) - 1))
            ++n_bits;
    }

    os->flush();
    ifs.close();
    return 0;
}


