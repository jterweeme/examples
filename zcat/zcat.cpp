#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <numeric>

#define HSIZE (1<<17)

uint8_t inbuf[BUFSIZ + 64];
int rsize;
int insize;
uint32_t bitmask;
uint32_t posbits;
uint32_t inbits;
uint8_t *p;
uint8_t n_bits;

uint32_t getCode(uint8_t n_bits)
{
    p = inbuf + (posbits >> 3);

    uint32_t code = ((uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16)
                            >> (posbits & 0x7)) & bitmask;

    posbits += n_bits;
    return code;
}

void clear()
{
    posbits = (posbits - 1) + ((n_bits<<3) - (posbits - 1 + (n_bits<<3)) % (n_bits<<3));
    n_bits = 9;
    bitmask = (1 << n_bits) - 1;
}

void bufread()
{
    int o = posbits >> 3;
    int e = o <= insize ? insize - o : 0;
    std::rotate(inbuf, inbuf + o, inbuf + o + e);
    insize = e;
    posbits = 0;

    if (insize < int(sizeof(inbuf) - BUFSIZ))
    {
        rsize = fread(inbuf + insize, 1, BUFSIZ, stdin);
        assert(rsize >= 0);
        insize += rsize;
    }

    inbits = rsize > 0 ? insize - insize % n_bits << 3 : (insize << 3) - (n_bits - 1);
}

int main()
{
    rsize = fread(inbuf, 1, BUFSIZ, stdin);
    insize = rsize;
    assert(insize >= 3);
    assert(inbuf[0] == 0x1f);
    assert(inbuf[1] == 0x9d);
    const uint8_t maxbits = inbuf[2] & 0x1f;
    const uint8_t block_mode = inbuf[2] & 0x80;
    assert(maxbits <= 16);
    n_bits = 9;
    bitmask = (1 << n_bits) - 1;
    uint32_t maxcode = n_bits == maxbits ? 1 << maxbits : (1 << n_bits) - 1;
    int32_t oldcode = -1;
    uint8_t finchar = 0;
    posbits = 3<<3;
    uint32_t free_ent = block_mode ? 257 : 256;
    uint16_t codetab[HSIZE] = {0};
    uint8_t htab[HSIZE];
    std::iota(htab, htab + 256, 0);
    bufread();

    while (true)
    {
        if (inbits <= posbits)
        {
            break;
        }

        if (free_ent > maxcode)
        {
            posbits = posbits - 1 + ((n_bits<<3) - (posbits - 1 + (n_bits<<3)) % (n_bits<<3));
            ++n_bits;
            maxcode = n_bits == maxbits ? 1 << maxbits : (1<<n_bits) - 1;
            bitmask = (1 << n_bits) - 1;
            bufread();
            continue;
        }

        uint32_t code = getCode(n_bits);

        if (oldcode == -1)
        {
            assert(code < 256);
            finchar = oldcode = code;
            std::cout.put(finchar);
            continue;
        }

        //clear
        if (code == 256 && block_mode)
        {
            memset(codetab, 0, 256);
            free_ent = 256;
            clear();
            maxcode = n_bits == maxbits ? 1 << maxbits : (1 << n_bits) - 1;
            bufread();
            continue;
        }

        uint32_t incode = code;
        uint8_t *stackp = htab + HSIZE - 1;

        //Special case for KwKwK string.
        if (code >= free_ent)   
        {
            assert(code <= free_ent);
            *--stackp = finchar;
            code = oldcode;
        }

        while (code >= 256)
        {
            //Generate output characters in reverse order
            *--stackp = htab[code];
            code = codetab[code];
        }

        *--stackp = finchar = htab[code];

        //And put them out in forward order
        int i = htab + HSIZE - 1 - stackp;
        std::cout.write((char *)(stackp), i);

        //Generate the new entry.
        if ((code = free_ent) < uint32_t(1 << maxbits))
        {
            codetab[code] = uint16_t(oldcode);
            htab[code] = finchar;
            free_ent = code + 1;
        }

        //remember previous code
        oldcode = incode;

        if (inbits <= posbits && rsize > 0)
        {
            bufread();
            continue;
        }
    }

    std::cout.flush();
    return 0;
}


