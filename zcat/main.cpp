#include <cstring>
#include <iostream>

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

typedef uint8_t char_type;
typedef long int code_int;

#ifdef SIGNED_COMPARE_SLOW
typedef unsigned long int count_int;
typedef unsigned short int count_short;
typedef unsigned long int cmp_code_int;
#else
typedef long int count_int;
typedef long int cmp_code_int;
#endif

static constexpr uint8_t BLOCK_MODE = 0x80;
static constexpr uint8_t BITS = 16;
static constexpr uint8_t INIT_BITS = 9;
static constexpr uint8_t HBITS = 17;
static constexpr uint32_t HSIZE = 1<<HBITS;
static constexpr uint16_t FIRST = 257;
static constexpr uint16_t CLEAR = 256;
static constexpr uint8_t MAGIC_1 = 0x1f;
static constexpr uint8_t MAGIC_2 = 0x9d;
static constexpr uint8_t BIT_MASK = 0x1f;


#ifndef BYTEORDER
//#define BYTEORDER 0000
#define BYTEORDER 4321
#endif

#if BYTEORDER == 4321
#define	input(b,o,c,n,m){ (c) = (*(long *)(&(b)[(o)>>3])>>((o)&0x7))&(m); (o) += (n); }
#else
static void input(char_type *b, int &o, code_int &c, int n, int m)
{
    char_type *p = &(b)[o >> 3];
    c = (((long(p[0])) | (long(p[1]) << 8) | (long(p[2]) << 16)) >> (o & 0x7)) & m;
    o += n;
}
#endif

#define	tab_suffixof(i) ((char_type *)(g_htab))[i]

static int decompress()
{
    count_int g_htab[HSIZE];
    uint16_t g_codetab[HSIZE];
    int g_block_mode = BLOCK_MODE;
    char_type inbuf[BUFSIZ + 64];
    char_type outbuf[BUFSIZ+2048];
    int maxbits = INIT_BITS;
    int exit_code = -1;
    char_type *stackp;
    code_int code;
    int finchar;
    code_int oldcode;
    code_int incode;
    int inbits;
    int posbits;
    int outpos;
    int bitmask;
    code_int free_ent;
    code_int maxcode;
    int rsize;

    long bytes_in;
    bytes_in = 0;
    int insize = 0;

    //rsize = read(0, inbuf, BUFSIZ);

    std::cin.read((char *)(inbuf), BUFSIZ);
    rsize = std::cin.gcount();

    insize += rsize;

    if (insize < 3 || inbuf[0] != MAGIC_1 || inbuf[1] != MAGIC_2)
    {
        if (rsize < 0)
            throw "read error";

        if (insize > 0)
        {
            fprintf(stderr, "%s: not in compressed format\n", "stdin");
            exit_code = 1;
        }

        return exit_code;
    }

    maxbits = inbuf[2] & BIT_MASK;
    g_block_mode = inbuf[2] & BLOCK_MODE;

    if (maxbits > BITS)
    {
        fprintf(stderr, "%s: compressed with %d bits, can only handle %d bits\n", "stdin", maxbits, BITS);
        exit_code = 4;
        return exit_code;
    }

    code_int maxmaxcode = 1<<maxbits;
    bytes_in = insize;
    int n_bits = INIT_BITS;
    maxcode = (1<<n_bits) - 1;
    bitmask = (1<<n_bits) - 1;
    oldcode = -1;
    finchar = 0;
    outpos = 0;
    posbits = 3<<3;
    free_ent = g_block_mode ? FIRST : 256;
    memset(g_codetab, 0, 256);

    for (code = 255; code >= 0; --code)
        tab_suffixof(code) = (char_type)code;

resetbuf:
    {
        int o = posbits >> 3;
        int e = o <= insize ? insize - o : 0;

        for (int i = 0 ; i < e ; ++i)
            inbuf[i] = inbuf[i+o];

        insize = e;
        posbits = 0;
    }

    if (insize < sizeof(inbuf) - BUFSIZ)
    {
        //rsize = read(0, inbuf + insize, BUFSIZ);

        std::cin.read((char *)(inbuf + insize), BUFSIZ);
        rsize = std::cin.gcount();

        if (rsize < 0)
            throw "read error";

        insize += rsize;
    }

    inbits = ((rsize > 0) ? (insize - insize%n_bits)<<3 : (insize<<3)-(n_bits-1));

    while (inbits > posbits)
    {
        if (free_ent > maxcode)
        {
            posbits = ((posbits-1) + ((n_bits<<3) - (posbits-1+(n_bits<<3))%(n_bits<<3)));
            ++n_bits;

            if (n_bits == maxbits)
                maxcode = maxmaxcode;
            else
                maxcode = (1<<n_bits) - 1;

            bitmask = (1<<n_bits)-1;
            goto resetbuf;
        }

        input(inbuf, posbits, code, n_bits, bitmask);

        if (oldcode == -1)
        {
            if (code >= 256)
            {
                fprintf(stderr, "oldcode:-1 code:%i\n", (int)(code));
                fprintf(stderr, "uncompress: corrupt input\n");
                return -1;
            }

            outbuf[outpos++] = (char_type)(finchar = (int)(oldcode = code));
            continue;
        }

        if (code == CLEAR && g_block_mode)
        {
            memset(g_codetab, 0, 256);
            free_ent = FIRST - 1;
            posbits = ((posbits-1) + ((n_bits<<3) - (posbits - 1 + (n_bits<<3))%(n_bits<<3)));
            n_bits = INIT_BITS;
            maxcode = (1<<n_bits) - 1;
            bitmask = (1<<n_bits) - 1;
            goto resetbuf;
        }

        incode = code;
        stackp = (char_type *)(g_htab + HSIZE - 1);

        if (code >= free_ent)
        {
            if (code > free_ent)
            {
                char_type *p;
                posbits -= n_bits;
                p = &inbuf[posbits>>3];
                fprintf(stderr, "insize:%d posbits:%d inbuf:%02X %02X %02X %02X %02X (%d)\n", insize, posbits, p[-1],p[0],p[1],p[2],p[3], (posbits&07));
                fprintf(stderr, "uncompress: corrupt input\n");
                return -1;
            }

            *--stackp = (char_type)finchar;
            code = oldcode;
        }

        while ((cmp_code_int)code >= (cmp_code_int)256)
        {
            *--stackp = tab_suffixof(code);
            code = g_codetab[code];
        }

        *--stackp = (char_type)(finchar = tab_suffixof(code));

        {
            int i = (char_type *)(g_htab + HSIZE - 1) - stackp;

            if (outpos + i >= BUFSIZ)
            {
                do
                {
                    if (i > BUFSIZ - outpos)
                        i = BUFSIZ-outpos;

                    if (i > 0)
                    {
                        memcpy(outbuf+outpos, stackp, i);
                        outpos += i;
                    }

                    if (outpos >= BUFSIZ)
                    {
                        std::cout.write((const char *)(outbuf), outpos);
                        outpos = 0;
                    }

                    stackp += i;
                    i = (char_type *)(g_htab + HSIZE - 1) - stackp;
                }
                while (i > 0);
            }
            else
            {
                memcpy(outbuf + outpos, stackp, i);
                outpos += i;
            }
        }

        if ((code = free_ent) < maxmaxcode)
        {
            g_codetab[code] = uint16_t(oldcode);
            ((char_type *)(g_htab))[code] = char_type(finchar);
            free_ent = code + 1;
        }

        //remember previous code
        oldcode = incode;
    }

    bytes_in += rsize;

    if (rsize > 0)
        goto resetbuf;

    if (outpos > 0)
        std::cout.write((const char *)(outbuf), outpos);

    return 0;
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

#ifdef WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    return decompress();
}


