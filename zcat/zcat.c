#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define FIRST   257      /* first free entry*/
#define HSIZE (1<<17)

typedef int32_t code_int;
typedef uint32_t count_int;
typedef uint16_t count_short;
typedef uint32_t cmp_code_int;
typedef uint8_t char_type;
typedef uint8_t *pchar_type;

union unie
{
    count_int ci[HSIZE];
    char_type b[HSIZE * 4];
};

int main()
{
    char_type inbuf[BUFSIZ+64];  //Input buffer
    char_type outbuf[BUFSIZ+2048];  //Output buffer
    int rsize = read(0, inbuf, BUFSIZ);
    int insize = rsize;
    assert(insize >= 3);
    assert(inbuf[0] == 0x1f);
    assert(inbuf[1] == 0x9d);
    int maxbits = inbuf[2] & 0x1f;
    int block_mode = inbuf[2] & 0x80;
    assert(maxbits <= 16);
    code_int maxmaxcode = 1 << maxbits;
    long bytes_in = insize;
    int n_bits = 9;
    int bitmask = (1 << n_bits) - 1;
    code_int maxcode = n_bits == maxbits ? maxmaxcode : (1 << n_bits) - 1;
    code_int oldcode = -1;
    int finchar = 0;
    int outpos = 0;
    int posbits = 3<<3;
    code_int free_ent = block_mode ? FIRST : 256;
    uint16_t codetab[HSIZE];
    memset(codetab, 0, 256);
    union unie htab;

    for (uint16_t i = 0; i < 256; ++i)
        htab.b[i] = i;
resetbuf:
    int o = posbits >> 3;
    int e = o <= insize ? insize - o : 0;

    for (int i = 0 ; i < e ; ++i)
        inbuf[i] = inbuf[i + o];

    insize = e;
    posbits = 0;

    if (insize < sizeof(inbuf) - BUFSIZ)
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

        code_int code = (((long)(p[0]) | ((long)(p[1]) << 8) | ((long)(p[2]) << 16))
                    >> (posbits & 0x7)) & bitmask;

        posbits += n_bits;

        if (oldcode == -1)
        {
            assert(code < 256);
            outbuf[outpos++] = (char_type)(finchar = (int)(oldcode = code));
            continue;
        }

        //clear
        if (code == 256 && block_mode)
        {
            memset(codetab, 0, 256);
            free_ent = FIRST - 1;
            posbits = (posbits - 1) + ((n_bits<<3) - (posbits - 1 + (n_bits<<3)) % (n_bits<<3));
            n_bits = 9;
            bitmask = (1 << n_bits) - 1;
            maxcode = n_bits == maxbits ? maxmaxcode : (1 << n_bits) - 1;
            goto resetbuf;
        }

        code_int incode = code;
        char_type *stackp = (pchar_type)(&(htab.ci[HSIZE - 1]));

        //Special case for KwKwK string.
        if (code >= free_ent)   
        {
            assert(code <= free_ent);
            *--stackp = (char_type)finchar;
            code = oldcode;
        }

        while ((cmp_code_int)(code) >= (cmp_code_int)(256))
        {
            //Generate output characters in reverse order
            *--stackp = ((pchar_type)(htab.ci))[code];
            code = codetab[code];
        }

        *--stackp = (char_type)(finchar = ((pchar_type)(htab.ci))[code]);

        //And put them out in forward order
        {
            int i = ((pchar_type)(&(htab.ci[HSIZE-1]))) - stackp;

            if (outpos + i >= BUFSIZ)
            {
                do
                {
                    if (i > BUFSIZ-outpos)
                        i = BUFSIZ-outpos;

                    if (i > 0)
                    {
                        memcpy(outbuf + outpos, stackp, i);
                        outpos += i;
                    }

                    if (outpos >= BUFSIZ)
                    {
                        assert(write(1, outbuf, outpos) == outpos);
                        outpos = 0;
                    }
                    stackp += i;
                    i = ((pchar_type)(&(htab.ci[HSIZE-1]))) - stackp;
                }
                while (i > 0);
            }
            else
            {
                memcpy(outbuf + outpos, stackp, i);
                outpos += i;
            }
        }

        //Generate the new entry.
        if ((code = free_ent) < maxmaxcode) 
        {
            codetab[code] = (uint16_t)(oldcode);
            ((pchar_type)(htab.ci))[code] = (char_type)finchar;
            free_ent = code + 1;
        }

        //remember previous code
        oldcode = incode;
    }

    bytes_in += rsize;

    if (rsize > 0)
        goto resetbuf;

    if (outpos > 0 && write(1, outbuf, outpos) != outpos)
        assert(0);

    return 0;
}


