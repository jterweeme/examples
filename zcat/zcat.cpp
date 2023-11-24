#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <numeric>
#include <vector>
#include <iostream>
#include <fstream>

#define HSIZE (1<<17)

uint32_t posbits = 0;



class BitStream
{
    std::vector<uint8_t> inbuf;
public:
    BitStream(std::istream &is)
    {
        for (int c; (c = is.get()) != -1;)
            inbuf.push_back(c);
    }

    int32_t getCode(uint8_t n)
    {
        if ((inbuf.size() << 3) - (n - 1) <= posbits)
            return -1;
    
        uint8_t *p = inbuf.data() + (posbits >> 3);
    
        uint32_t code = (((uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16))
                    >> (posbits & 0x7)) & (1 << n) - 1;
    
        return code;
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
    int32_t free_ent = block_mode ? 257 : 256;
    uint16_t codetab[HSIZE];
    uint8_t htab[HSIZE];
    uint8_t n_bits = 9;
    std::fill(codetab, codetab + 256, 0);
    std::iota(htab, htab + 256, 0);
    BitStream bis(*is);

    while (true)
    {
        int32_t maxcode = n_bits == maxbits ? 1 << maxbits : (1 << n_bits) - 1;
    
        if (free_ent > maxcode)
            ++n_bits;

        int32_t code = bis.getCode(n_bits);
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
            memset(codetab, 0, 256);
            free_ent = 256;
            posbits = (posbits - 1) + ((n_bits<<3) - (posbits - 1 + (n_bits<<3)) % (n_bits<<3));
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
    
        if ((code = free_ent) < (1 << maxbits))
        {
            codetab[code] = (uint16_t)oldcode;
            htab[code] = finchar;
            free_ent = code + 1;
        }
    
        oldcode = incode;
    }

    os->flush();
    ifs.close();
    return 0;
}


