#include <cassert>
#include <cstdint>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <fstream>

#define HSIZE (1<<17)

class BitStream
{
    std::istream &_is;
    uint32_t _bits = 0;
    uint32_t _window = 0;
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

    void ignoreBits(uint32_t n)
    {
        uint32_t x = std::min(n, _bits);
        _window = _window >> x, _bits -= x, n -= x;
        auto dv = std::div(n, 8);
        _is.ignore(dv.quot), readBits(dv.rem), cnt += n + x;
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
    uint16_t codetab[HSIZE];
    std::fill(codetab, codetab + 256, 0);
    bis.cnt = 0;
    code = oldcode = bis.readBits(n_bits);
    assert(code >= 0 && code < 256);
    char finchar = oldcode, htab[HSIZE];
    os->put(finchar);
    std::iota(htab, htab + 256, 0);

    while ((code = incode = bis.readBits(n_bits)) != -1)
    {
        if (code == 256 && block_mode)
        {
            bis.ignoreBits((n_bits<<3) - (bis.cnt - 1 + (n_bits<<3)) % (n_bits<<3) - 1);
            std::fill(codetab, codetab + 256, 0), free_ent = 256, n_bits = 9;
            continue;
        }
    
        char *stackp = htab + HSIZE - 1;
        assert(code <= free_ent);
    
        if (code == free_ent)   
            *--stackp = finchar, code = oldcode;
    
        while (code >= 256)
            *--stackp = htab[code], code = codetab[code];
    
        *--stackp = finchar = htab[code];
        os->write(stackp, htab + HSIZE - 1 - stackp);
    
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


