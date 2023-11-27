#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define HSIZE (1<<17)
#define IBUFSIZ BUFSIZ
#define ELBOWROOM 64

int main(int argc, char **argv)
{
    FILE *in = stdin;

    if (argc > 1)
        in = fopen(argv[1], "r");

    uint8_t *inbuf = malloc(IBUFSIZ + ELBOWROOM);
    size_t rsize = fread(inbuf, 1, IBUFSIZ, in);
    int insize = rsize;
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

    for (uint16_t i = 0; i < 256; ++i)
        htab[i] = i;
resetbuf:
    int o = posbits >> 3;
    int e = o <= insize ? insize - o : 0;

    for (int i = 0 ; i < e ; ++i)
        inbuf[i] = inbuf[i + o];

    insize = e;
    posbits = 0;

    if (insize < ELBOWROOM)
    {
        rsize = fread(inbuf + insize, 1, IBUFSIZ, in);
        assert(rsize >= 0);
        insize += rsize;
    }

    int inbits = rsize > 0 ? insize - insize % n_bits << 3 : (insize << 3) - (n_bits - 1);

    while (inbits > posbits)
    {
        if (free_ent > maxcode)
        {
            posbits = posbits - 1 + ((n_bits<<3) - (posbits - 1 + (n_bits<<3)) % (n_bits<<3));
            ++n_bits;
            maxcode = n_bits == maxbits ? 1 << maxbits : (1 << n_bits) - 1;
            bitmask = (1 << n_bits) - 1;
            goto resetbuf;
        }

        uint8_t *p = inbuf + (posbits >> 3);
        uint32_t code = ((uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16));
        code = code >> (posbits & 0x7) & bitmask;
        posbits += n_bits;

        if (oldcode == -1)
        {
            assert(code < 256);
            oldcode = code;
            finchar = oldcode;
            fwrite(&finchar, 1, 1, stdout);
            continue;
        }

        //clear
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
        assert(code <= free_ent);

        //Special case for KwKwK string.
        if (code == free_ent)   
            *--stackp = finchar, code = oldcode;

        //Generate output characters in reverse order
        while (code >= 256)
            *--stackp = htab[code], code = codetab[code];

        *--stackp = finchar = htab[code];

        //And put them out in forward order
        uint32_t i = htab + HSIZE - 1 - stackp;
        fwrite(stackp, 1, i, stdout);

        //Generate the new entry.
        if ((code = free_ent) < 1 << maxbits)
            codetab[code] = (uint16_t)oldcode, htab[code] = finchar, free_ent = code + 1;

        //remember previous code
        oldcode = incode;
    }

    if (rsize > 0)
        goto resetbuf;

    fflush(stdout);
    fclose(in);
    free(inbuf);
    return 0;
}


