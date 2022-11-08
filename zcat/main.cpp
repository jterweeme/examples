#include <cstring>
#include <iostream>
#include <algorithm>

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

static constexpr uint8_t
BLOCK_MODE = 0x80, BITS = 16, INIT_BITS = 9, HBITS = 17,
MAGIC_1 = 0x1f, MAGIC_2 = 0x9d, BIT_MASK = 0x1f;

static constexpr uint32_t HSIZE = 1<<HBITS;
static constexpr uint16_t FIRST = 257, CLEAR = 256;

static void input(uint8_t *b, int &o, long &c, int n, int m)
{
    c = (*(long *)(&b[o >> 3]) >> (o & 0x7)) & m;
    o += n;
}

#define	tab_suffixof(i) ((uint8_t *)(g_htab))[i]

static int decompress(std::istream &is, std::ostream &os)
{
    long g_htab[HSIZE];
    uint16_t g_codetab[HSIZE];
    uint8_t inbuf[BUFSIZ + 64], outbuf[BUFSIZ+2048];
    int inbits, bitmask;
    is.read((char *)(inbuf), BUFSIZ);
    int insize, rsize;
    rsize = insize = is.gcount();
    int maxbits = inbuf[2] & BIT_MASK;
    int g_block_mode = inbuf[2] & BLOCK_MODE;
    long code, incode, free_ent, maxcode;
    long maxmaxcode = 1<<maxbits;
    long bytes_in = insize;
    int n_bits = INIT_BITS;
    maxcode = bitmask = (1<<n_bits) - 1;
    long oldcode = -1;
    int finchar = 0, outpos = 0, posbits = 3<<3;
    free_ent = g_block_mode ? FIRST : 256;
    memset(g_codetab, 0, 256);

    for (code = 255; code >= 0; --code)
        //g_htab[code] = uint8_t(code);
        tab_suffixof(code) = uint8_t(code);
resetbuf:
    typedef uint8_t char_type;
    typedef long code_int;
    typedef long count_int;
    typedef long cmp_code_int;
    int o = posbits >> 3;
    int e = o <= insize ? insize - o : 0;

    for (int i = 0 ; i < e ; ++i)
        inbuf[i] = inbuf[i+o];

    insize = e;
    posbits = 0;

    if (insize < sizeof(inbuf) - BUFSIZ)
    {
        is.read((char *)(inbuf + insize), BUFSIZ);
        rsize = is.gcount();

        if (rsize < 0)
            throw "read error";

        insize += rsize;
    }

    inbits = rsize > 0 ? insize - insize % n_bits << 3 : (insize<<3)-(n_bits-1);

    while (inbits > posbits)
    {
        if (free_ent > maxcode)
        {
            posbits = ((posbits-1) + ((n_bits<<3) - (posbits-1+(n_bits<<3))%(n_bits<<3)));
            ++n_bits;
            maxcode = n_bits == maxbits ? maxmaxcode : (1 << n_bits) - 1;
            bitmask = (1<<n_bits)-1;
            goto resetbuf;
        }

        input(inbuf, posbits, code, n_bits, bitmask);

        if (oldcode == -1)
        {
            if (code >= 256)
                throw "corrupt input";

            outbuf[outpos++] = uint8_t(finchar = int(oldcode = code));
            continue;
        }

        if (code == CLEAR && g_block_mode)
        {
            memset(g_codetab, 0, 256);
            free_ent = FIRST - 1;
            posbits = (posbits-1) + ((n_bits<<3) - (posbits - 1 + (n_bits<<3))%(n_bits<<3));
            n_bits = INIT_BITS;
            maxcode = (1<<n_bits) - 1;
            bitmask = (1<<n_bits) - 1;
            goto resetbuf;
        }

        incode = code;
        char_type *stackp = (char_type *)(g_htab + HSIZE - 1);

        if (code >= free_ent)
        {
            if (code > free_ent)
            {
                char_type *p;
                posbits -= n_bits;
                p = &inbuf[posbits>>3];

                fprintf(stderr, "insize:%d posbits:%d inbuf:%02X %02X %02X %02X %02X (%d)\n",
                        insize, posbits, p[-1],p[0],p[1],p[2],p[3], (posbits&07));

                fprintf(stderr, "uncompress: corrupt input\n");
                return -1;
            }

            *--stackp = char_type(finchar);
            code = oldcode;
        }

        while ((cmp_code_int)code >= (cmp_code_int)256)
        {
            *--stackp = tab_suffixof(code);
            code = g_codetab[code];
        }

        *--stackp = char_type(finchar = tab_suffixof(code));

        {
            int i = (char_type *)(g_htab + HSIZE - 1) - stackp;

            if (outpos + i >= BUFSIZ)
            {
                do
                {
                    i = std::min(i, BUFSIZ - outpos);

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

    return decompress(std::cin, std::cout);
}


