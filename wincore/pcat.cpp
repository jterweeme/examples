#include "mystd.h"
#include <cstdint>
#include <cassert>

#ifdef WIN32
#include <io.h>
#endif

using mystd::ifstream;
using mystd::istream;
using mystd::ostream;
using mystd::byteswap;
using mystd::cin;
using mystd::cerr;
using mystd::cout;
using mystd::endl;

static void unpack(istream &is, ostream &os, ostream &msg)
{
    assert(is.get() == 0x1f);
    assert(is.get() == 0x1e);
    uint32_t origsize;
    is.read((char *)(&origsize), 4);
#if __BYTE_ORDER == __LITTLE_ENDIAN
    origsize = byteswap(origsize);
#endif
    uint8_t maxlev = uint8_t(is.get());
    msg << "Length: " << origsize << ", Levels: " << unsigned(maxlev) << endl;
    uint16_t intnodes[maxlev];

    for (uint8_t i = 0; i < maxlev; ++i)
        intnodes[i] = uint16_t(is.get());

    char *tree[maxlev];
    char characters[256];
    char *xeof = characters;

    for (uint8_t i = 0; i < maxlev; ++i)
    {
        tree[i] = xeof;

        for (int c = intnodes[i]; c > 0; --c)
        {
            assert(xeof < characters + 256);
            *xeof++ = char(is.get());
        }
    }

    *xeof++ = char(is.get());
    intnodes[maxlev - 1] += 2;
    uint32_t nchildren = 0;

    for (uint8_t i = maxlev; i >= 1; --i)
    {
        int c = intnodes[i - 1];
        intnodes[i - 1] = nchildren /= 2;
        nchildren += c;
    }

    for (uint32_t lev = 1, i = 0; true;)
    {
        int c = is.get();

        for (uint8_t bit = 0; bit < 8; ++bit)
        {
            i *= 2;

            if (c & 0200)
                ++i;

            c <<= 1;
            int j = i - intnodes[lev - 1];

            if (j < 0)
            {
                ++lev;
                continue;
            }

            const char *p = tree[lev - 1] + j;

            if (p == xeof)
            {
                assert(origsize == 0);
                return;
            }

            os.put(*p);
            --origsize;
            lev = 1;
            i = 0;
        }
    }
}

int main(int argc, char **argv)
{
#ifdef WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    istream *is = &cin;
    ifstream ifs;

    if (argc > 1)
        ifs.open(argv[1]), is = &ifs;

    unpack(*is, cout, cerr);
    ifs.close();
    return 0;
}


