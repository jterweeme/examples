#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <numeric>

#define HSIZE (1<<17)
static constexpr uint32_t IBUFSIZ = 100 << 20;
static constexpr uint32_t ELBOWROOM = 64;

int main(int argc, char **argv)
{
    FILE *in = stdin;

    if (argc > 1)
        in = fopen(argv[1], "r");

    uint8_t *inbuf = new uint8_t[IBUFSIZ + ELBOWROOM];
    size_t rsize = fread(inbuf, 1, IBUFSIZ, in);
    assert(rsize < IBUFSIZ);
    assert(rsize >= 3);
    assert(inbuf[0] == 0x1f);
    assert(inbuf[1] == 0x9d);
    const uint8_t maxbits = inbuf[2] & 0x1f;
    const uint8_t block_mode = inbuf[2] & 0x80;
    assert(maxbits <= 16);
    uint8_t n_bits = 9;
    uint32_t bitmask = (1 << n_bits) - 1;
    uint32_t maxcode = n_bits == maxbits ? 1 << maxbits : (1 << n_bits) - 1;
    int32_t oldcode = -1;
    uint8_t finchar = 0;
    uint32_t posbits = 3<<3;
    uint32_t free_ent = block_mode ? 257 : 256;
    uint16_t codetab[HSIZE];
    memset(codetab, 0, 256);
    uint8_t htab[HSIZE];
    std::iota(htab, htab + 256, 0);
    int insize = rsize;
resetbuf:
    int o = posbits >> 3;
    int e = o <= insize ? insize - o : 0;
    std::rotate(inbuf, inbuf + o, inbuf + e + o);
    insize = e;
    posbits = 0;
    rsize = insize < ELBOWROOM ? 0 : rsize;
    int inbits = rsize > 0 ? insize - insize % n_bits << 3 : (insize << 3) - (n_bits - 1);
loop:
    if (free_ent > maxcode)
    {
        posbits = posbits - 1 + ((n_bits<<3) - (posbits - 1 + (n_bits<<3)) % (n_bits<<3));
        ++n_bits;
        maxcode = n_bits == maxbits ? 1 << maxbits : (1 << n_bits) - 1;
        bitmask = (1 << n_bits) - 1;
        goto resetbuf;
    }

    uint8_t *p = inbuf + (posbits >> 3);

    uint32_t code = (((uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16))
                >> (posbits & 0x7)) & bitmask;

    posbits += n_bits;

    if (oldcode == -1)
    {
        assert(code < 256);
        oldcode = code;
        finchar = oldcode;
        fwrite(&finchar, 1, 1, stdout);
        goto loop;
    }

    if (code == 256 && block_mode)
    {
        memset(codetab, 0, 256);
        free_ent = 256;
        posbits = (posbits - 1) + ((n_bits<<3) - (posbits - 1 + (n_bits<<3)) % (n_bits<<3));
        n_bits = 9;
        bitmask = (1 << n_bits) - 1;
        maxcode = n_bits == maxbits ? 1 << maxbits : (1 << n_bits) - 1;
        goto resetbuf;
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
    uint32_t i = htab + HSIZE - 1 - stackp;
    fwrite(stackp, 1, i, stdout);

    if ((code = free_ent) < uint32_t(1 << maxbits))
    {
        codetab[code] = (uint16_t)oldcode;
        htab[code] = finchar;
        free_ent = code + 1;
    }

    oldcode = incode;

    if (inbits > posbits)
        goto loop;

    if (rsize > 0)
        goto resetbuf;

    fflush(stdout);
    fclose(in);
    delete[] inbuf;
    return 0;
}


