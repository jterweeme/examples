#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <numeric>
#include <vector>
#include <iostream>

#define HSIZE (1<<17)

int main(int argc, char **argv)
{
    FILE *in = argc > 1 ? fopen(argv[1], "r") : stdin;
    assert(fgetc(in) == 0x1f);
    assert(fgetc(in) == 0x9d);
    uint8_t buf1 = fgetc(in);
    const uint8_t maxbits = buf1 & 0x1f;
    const uint8_t block_mode = buf1 & 0x80;
    assert(maxbits <= 16);
    std::vector<uint8_t> inbuf;
    uint8_t n_bits = 9;
    uint32_t bitmask = (1 << n_bits) - 1;
    int32_t oldcode = -1;
    uint8_t finchar = 0;
    uint32_t free_ent = block_mode ? 257 : 256;
    uint16_t codetab[HSIZE];
    memset(codetab, 0, 256);
    uint8_t htab[HSIZE];
    std::iota(htab, htab + 256, 0);

    for (int c; (c = fgetc(in)) != -1;)
        inbuf.push_back(c);

    for (uint32_t posbits = 0; (inbuf.size() << 3) - (n_bits - 1) > posbits;)
    {
        uint32_t maxcode = n_bits == maxbits ? 1 << maxbits : (1 << n_bits) - 1;
    
        if (free_ent > maxcode)
        {
            ++n_bits;
            bitmask = (1 << n_bits) - 1;
            continue;
        }
    
        uint8_t *p = inbuf.data() + (posbits >> 3);
    
        uint32_t code = (((uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16))
                    >> (posbits & 0x7)) & bitmask;
    
        posbits += n_bits;
    
        if (oldcode == -1)
        {
            assert(code < 256);
            oldcode = code;
            finchar = oldcode;
            fwrite(&finchar, 1, 1, stdout);
            continue;
        }
    
        if (code == 256 && block_mode)
        {
            memset(codetab, 0, 256);
            free_ent = 256;
            posbits = (posbits - 1) + ((n_bits<<3) - (posbits - 1 + (n_bits<<3)) % (n_bits<<3));
            n_bits = 9;
            bitmask = (1 << n_bits) - 1;
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
        fwrite(stackp, 1, htab + HSIZE - 1 - stackp, stdout);
    
        if ((code = free_ent) < uint32_t(1 << maxbits))
        {
            codetab[code] = (uint16_t)oldcode;
            htab[code] = finchar;
            free_ent = code + 1;
        }
    
        oldcode = incode;
    }

    fflush(stdout);
    fclose(in);
    return 0;
}


