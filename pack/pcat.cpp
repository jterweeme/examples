#include <stdexcept>
#include <iostream>
#include <vector>
#include <fstream>
#include <cstdint>
#include <cassert>

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

using std::ifstream;
using std::istream;
using std::ostream;
using std::cin;
using std::cerr;
using std::cout;

class Toolbox
{
public:
    static uint32_t swapEndian(uint32_t x);
    static uint32_t be32tohost(uint32_t num);
};

uint32_t Toolbox::be32tohost(uint32_t num)
{
    return swapEndian(num);
}

uint32_t Toolbox::swapEndian(uint32_t n)
{
    return n >> 24 & 0xff | n << 8 & 0xff0000 | n >> 8 & 0xff00 | n << 24 & 0xff000000;
}

static void dumpIntnodes(ostream &os, uint16_t *intnodes, uint8_t n)
{
    os << "Intnodes:";

    for (uint8_t i = 0; i < n; ++i)
        os << " " << intnodes[i];

    os << "\r\n";
    os.flush();
}

static void unpack(istream &is, ostream &os, ostream &msg, bool verbose = false)
{
    uint16_t magic;
    is.read((char *)(&magic), 2);
    uint32_t origsize = 0;
    is.read((char *)(&origsize), 4);
    origsize = Toolbox::be32tohost(origsize);
    uint8_t maxlev = uint8_t(is.get());

    if (verbose)
    {
        cerr << "Length: " << origsize << ", Levels: " << uint16_t(maxlev) << "\r\n";
        cerr.flush();
    }

    uint16_t intnodes[maxlev];

    for (uint8_t i = 0; i < maxlev; ++i)
        intnodes[i] = uint16_t(is.get());

    dumpIntnodes(cerr, intnodes, maxlev);
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

    dumpIntnodes(cerr, intnodes, maxlev);
    intnodes[maxlev - 1] += 2;
    dumpIntnodes(cerr, intnodes, maxlev);
    uint32_t nchildren = 0;

    for (uint8_t i = maxlev; i >= 1; --i)
    {
        int c = intnodes[i - 1];
        intnodes[i - 1] = nchildren /= 2;
        nchildren += c;
    }

    dumpIntnodes(cerr, intnodes, maxlev);
    uint32_t lev = 1, i = 0;

    while (true)
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
    ifstream ifs(argv[1]);
    unpack(ifs, cout, cerr, true);
    ifs.close();
    return 0;
}


