#include <cassert>
#include <cstdint>
#include <algorithm>
#include <numeric>
#include <vector>
#include <iostream>
#include <fstream>

#define HSIZE (1<<17)

class BitStream
{
    std::istream &_is;
    uint32_t _bits = 0;
    uint32_t _window = 0;
    uint32_t _cnt = 0;
public:
    BitStream(std::istream &is) : _is(is) { }
    uint32_t cnt() const { return _cnt; }
    void cnt(uint32_t val) { _cnt = val; }

    int32_t readBits(uint8_t n)
    {
        for (; _bits < n; _bits += 8)
        {
            int c = _is.get();

            if (c == -1)
                return -1;

            _window = _window | c << _bits;
        }

        int32_t ret = _window & (1 << n) - 1;
        _window = _window >> n, _bits -= n, _cnt += n;
        return ret;
    }
};

int main(int argc, char **argv)
{
    std::ostream *os = &std::cout;
    std::istream *is = &std::cin;
    std::ifstream ifs;

    if (argc > 1)
    {
        ifs.open(argv[1]);
        is = &ifs;
    }

    BitStream bis(*is);
    assert(bis.readBits(8) == 0x1f);
    assert(bis.readBits(8) == 0x9d);
    const uint8_t maxbits = bis.readBits(5);
    assert(maxbits <= 16);
    bis.readBits(2);
    const bool block_mode = bis.readBits(1) ? true : false;
    uint8_t finchar = 0, n_bits = 9, htab[HSIZE];
    int32_t oldcode = -1, free_ent = block_mode ? 257 : 256;
    uint16_t codetab[HSIZE];
    std::fill(codetab, codetab + 256, 0);
    std::iota(htab, htab + 256, 0);
    bis.cnt(0);

    while (true)
    {
        const auto maxcode = n_bits == maxbits ? 1 << maxbits : (1 << n_bits) - 1;
    
        if (free_ent > maxcode)
            ++n_bits;

        int32_t code = bis.readBits(n_bits);

        if (code == -1)
            break;
    
        if (oldcode == -1)
        {
            assert(code < 256);
            oldcode = code;
            finchar = oldcode;
            os->put(finchar);
            continue;
        }
    
        //end block
        if (code == 256 && block_mode)
        {
            std::fill(codetab, codetab + 256, 0), free_ent = 256;

            const auto padding = (bis.cnt() - 1) + ((n_bits<<3)
                        - (bis.cnt() - 1 + (n_bits<<3)) % (n_bits<<3));
            
            while (padding - bis.cnt() >= 16)
                bis.readBits(16);

            n_bits = 9;
            continue;
        }
    
        uint32_t incode = code;
        uint8_t *stackp = htab + HSIZE - 1;
        assert(code <= free_ent);
    
        if (code >= free_ent)   
            *--stackp = finchar, code = oldcode;
    
        while (code >= 256)
            *--stackp = htab[code], code = codetab[code];
    
        *--stackp = finchar = htab[code];
        os->write((char *)(stackp), htab + HSIZE - 1 - stackp);
    
        if ((code = free_ent) < 1 << maxbits)
            codetab[code] = uint16_t(oldcode), htab[code] = finchar, free_ent = code + 1;
    
        oldcode = incode;
    }

    os->flush();
    ifs.close();
    return 0;
}


