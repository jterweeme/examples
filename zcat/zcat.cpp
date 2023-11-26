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
    uint8_t _byte = 0;
public:
    BitStream(std::istream &is) : _is(is) { }

    int32_t readBit()
    {
        if (_bits == 0)
        {
            int c = _is.get();

            if (c == -1)
                return -1;

            _byte = uint8_t(c);
            _bits = 8;
        }

        --_bits;
        int32_t ret = _byte & 1;
        _byte = _byte >> 1;
        return ret;
    }

    int32_t readBits(uint8_t n)
    {
        int32_t ret = 0;

        for (uint8_t i = 0; i < n; ++i)
        {
            int32_t bit = readBit();
            
            if (bit == -1)
                return -1;

            ret = ret | bit << i;
        }

        return ret;
    }
};

int main(int argc, char **argv)
{
    std::ifstream ifs;
    std::istream *is = &std::cin;

    if (argc > 1)
    {
        ifs.open(argv[1]);
        is = &ifs;
    }

    std::ostream *os = &std::cout;
    assert(is->get() == 0x1f);
    assert(is->get() == 0x9d);
    uint8_t buf1 = is->get();
    const uint8_t maxbits = buf1 & 0x1f;
    const uint8_t block_mode = buf1 & 0x80;
    assert(maxbits <= 16);
    int32_t oldcode = -1;
    uint8_t finchar = 0;
    uint32_t posbits = 0;
    int32_t free_ent = block_mode ? 257 : 256;
    uint16_t codetab[HSIZE];
    uint8_t htab[HSIZE];
    uint8_t n_bits = 9;
    std::fill(codetab, codetab + 256, 0);
    std::iota(htab, htab + 256, 0);
    BitStream bis(*is);

    while (true)
    {
        const auto maxcode = n_bits == maxbits ? 1 << maxbits : (1 << n_bits) - 1;
    
        if (free_ent > maxcode)
            ++n_bits;

        int32_t code = bis.readBits(n_bits);
        posbits += n_bits;

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
    
        if (code == 256 && block_mode)
        {
            std::fill(codetab, codetab + 256, 0);
            free_ent = 256;

            //padding
            {
                const auto losbits = (posbits - 1) + ((n_bits<<3)
                        - (posbits - 1 + (n_bits<<3)) % (n_bits<<3));
            
                while (losbits - posbits >= 16)
                {
                    bis.readBits(16);
                    posbits += 16;
                }
            }

            n_bits = 9;
            continue;
        }
    
        uint32_t incode = code;
        uint8_t *stackp = htab + HSIZE - 1;
    
        if (code >= free_ent)   
        {
            assert(code <= free_ent);
            *--stackp = finchar;
            code = oldcode;
        }
    
        while (code >= 256)
        {
            *--stackp = htab[code];
            code = codetab[code];
        }
    
        *--stackp = finchar = htab[code];
        os->write((char *)(stackp), htab + HSIZE - 1 - stackp);
    
        if ((code = free_ent) < 1 << maxbits)
        {
            codetab[code] = uint16_t(oldcode);
            htab[code] = finchar;
            free_ent = code + 1;
        }
    
        oldcode = incode;
    }

    os->flush();
    ifs.close();
    return 0;
}


