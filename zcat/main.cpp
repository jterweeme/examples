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

static int decompress(std::istream &is, std::ostream &os)
{
    uint8_t g_htab[HSIZE];
    uint16_t g_codetab[HSIZE];
    uint8_t inbuf[BUFSIZ + 64], outbuf[BUFSIZ+2048];
    is.read((char *)(inbuf), BUFSIZ);
    int insize, rsize;
    rsize = insize = is.gcount();
    int maxbits = inbuf[2] & BIT_MASK;
    int g_block_mode = inbuf[2] & BLOCK_MODE;
    long maxmaxcode = 1<<maxbits;
    long bytes_in = insize;
    int n_bits = INIT_BITS;
    long maxcode, bitmask;
    maxcode = bitmask = (1<<n_bits) - 1;
    long oldcode = -1;
    int finchar = 0, posbits = 3<<3;
    long free_ent = g_block_mode ? FIRST : 256;
    memset(g_codetab, 0, 256);

    for (long code = 255; code >= 0; --code)
        g_htab[code] = uint8_t(code);
resetbuf:

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

    int inbits = rsize > 0 ? insize - insize % n_bits << 3 : (insize<<3)-(n_bits-1);

    while (inbits > posbits)
    {
        if (free_ent > maxcode)
        {
            posbits = (posbits-1) + ((n_bits<<3) - (posbits-1+(n_bits<<3))%(n_bits<<3));
            ++n_bits;
            maxcode = n_bits == maxbits ? maxmaxcode : (1 << n_bits) - 1;
            bitmask = (1<<n_bits)-1;
            goto resetbuf;
        }

        long code = (*(long *)(&inbuf[posbits >> 3]) >> (posbits & 0x7)) & bitmask;
        posbits += n_bits;

        if (oldcode == -1)
        {
            if (code >= 256)
                throw "corrupt input";

            os.put(uint8_t(finchar = int(oldcode = code)));
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

        long incode = code;
        uint8_t *stackp = g_htab + HSIZE - 1;

        if (code >= free_ent)
        {
            if (code > free_ent)
                throw "corrupt input";

            *--stackp = uint8_t(finchar);
            code = oldcode;
        }

        while ((long)code >= (long)256)
        {
            *--stackp = g_htab[code];
            code = g_codetab[code];
        }

        *--stackp = uint8_t(finchar = g_htab[code]);

        int i = 0;

        do
        {
            i = std::min(i, BUFSIZ);
            os.write((const char *)(stackp), i);
            stackp += i;
            i = (g_htab + HSIZE - 1) - stackp;
        }
        while (i > 0);

        if ((code = free_ent) < maxmaxcode)
        {
            g_codetab[code] = uint16_t(oldcode);
            g_htab[code] = uint8_t(finchar);
            free_ent = code + 1;
        }

        oldcode = incode;
    }

    bytes_in += rsize;

    if (rsize > 0)
        goto resetbuf;

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


