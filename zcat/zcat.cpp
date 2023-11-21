#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <iostream>

#define HSIZE (1<<17)

typedef int32_t code_int;
typedef uint32_t count_int;
typedef uint16_t count_short;
typedef uint32_t cmp_code_int;
typedef uint8_t char_type;

int main()
{
    char_type inbuf[BUFSIZ + 64];  //Input buffer
    int rsize = read(0, inbuf, BUFSIZ);
    int insize = rsize;
    assert(insize >= 3);
    assert(inbuf[0] == 0x1f);
    assert(inbuf[1] == 0x9d);
    uint32_t maxbits = inbuf[2] & 0x1f;
    uint8_t block_mode = inbuf[2] & 0x80;
    assert(maxbits <= 16);
    code_int maxmaxcode = 1 << maxbits;
    long bytes_in = insize;
    uint32_t n_bits = 9;
    uint32_t bitmask = (1 << n_bits) - 1;
    code_int maxcode = n_bits == maxbits ? maxmaxcode : (1 << n_bits) - 1;
    code_int oldcode = -1;
    char_type finchar = 0;
    int posbits = 3<<3;
    code_int free_ent = block_mode ? 257 : 256;
    uint16_t codetab[HSIZE];
    memset(codetab, 0, 256);
    char_type htab[HSIZE * 4];

    for (uint16_t i = 0; i < 256; ++i)
        htab[i] = i;
resetbuf:
    int o = posbits >> 3;
    int e = o <= insize ? insize - o : 0;

    for (int i = 0 ; i < e ; ++i)
        inbuf[i] = inbuf[i + o];

    insize = e;
    posbits = 0;

    if (insize < int(sizeof(inbuf) - BUFSIZ))
    {
        rsize = read(0, inbuf + insize, BUFSIZ);
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
            maxcode = n_bits == maxbits ? maxmaxcode : (1<<n_bits) - 1;
            bitmask = (1 << n_bits) - 1;
            goto resetbuf;
        }

        char_type *p = inbuf + (posbits >> 3);

        code_int code = ((long(p[0]) | (long(p[1]) << 8) | (long(p[2]) << 16))
                    >> (posbits & 0x7)) & bitmask;

        posbits += n_bits;

        if (oldcode == -1)
        {
            assert(code < 256);
            oldcode = code;
            finchar = oldcode;
            std::cout.put(finchar);
            //write(1, &finchar, 1);
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
            maxcode = n_bits == maxbits ? maxmaxcode : (1 << n_bits) - 1;
            goto resetbuf;
        }

        code_int incode = code;
        char_type *stackp = htab + HSIZE * 4 - 4;

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
        int i = htab + HSIZE * 4 - 4 - stackp;
        std::cout.write((char *)(stackp), i);

        //Generate the new entry.
        if ((code = free_ent) < maxmaxcode) 
        {
            codetab[code] = uint16_t(oldcode);
            htab[code] = finchar;
            free_ent = code + 1;
        }

        //remember previous code
        oldcode = incode;
    }

    bytes_in += rsize;

    if (rsize > 0)
        goto resetbuf;

    std::cout.flush();
    return 0;
}


