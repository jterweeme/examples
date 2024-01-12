//This is a comment
//I love comments

#include "generator.h"
#include "mystl.h"

using mystl::istream;
using mystl::ifstream;
using mystl::ostream;
using mystl::cin;
using mystl::cout;
using std::fill;

class Dictionary
{
    static constexpr unsigned HSIZE = 69001;
    uint16_t codetab[HSIZE];
    unsigned htab[HSIZE];
public:
    unsigned free_ent;
    void clear() { fill(codetab, codetab + HSIZE, 0), free_ent = 257; }
    Dictionary() { clear(); }

    uint16_t find(unsigned c, unsigned ent, bool stcode)
    {
        unsigned hp = c << 8 ^ ent;

        while (codetab[hp])
        {
            if (htab[hp] == (c << 16 | ent))
                return codetab[hp];

            if ((hp += hp + 1) >= HSIZE)
                hp -= HSIZE;
        }

        if (stcode)
        {
            codetab[hp] = free_ent++;
            htab[hp] = c << 16 | ent;
        }

        return 0;
    }
};

static Generator<unsigned> codify(istream &is)
{
    static constexpr unsigned CHECK_GAP = 10000;
    Dictionary dict;
    uint64_t cnt = 0;
    unsigned ent = is.get();
    unsigned n_bits = 9, checkpoint = CHECK_GAP;
    unsigned ratio = 0, extcode = (1 << n_bits) + 1;
    bool stcode = true;
    uint64_t bytes_in = 1;

    for (int byte; (byte = is.get()) != -1;)
    {
        if (dict.free_ent >= extcode && ent < 257)
        {
            if (n_bits < 16)
                ++n_bits, extcode = n_bits < 16 ? (1 << n_bits) + 1 : 1 << 16;
            else
                stcode = false;
        }

        if (!stcode && bytes_in >= checkpoint && ent < 257)
        {
            checkpoint = bytes_in + CHECK_GAP;
            unsigned rat = (bytes_in << 8) / (cnt >> 3);

            if (rat >= ratio)
                ratio = rat;
            else
            {
                co_yield 256;
                ratio = 0;
                dict.clear();
                cnt += n_bits;
                n_bits = 9, stcode = true;
                extcode = n_bits < 16 ? (1 << n_bits) + 1 : 1 << n_bits;
            }
        }

        ++bytes_in;
        uint16_t x = dict.find(byte, ent, stcode);

        if (x)
            ent = x;
        else
        {
            co_yield ent;
            cnt += n_bits;
            ent = byte;
        }
    }

    co_yield ent;
    cnt += n_bits;
}

static void press(Generator<unsigned> codes, ostream &os, unsigned bitdepth)
{
    unsigned cnt = 0, nbits = 9;
    char buf[20] = {0};

    while (codes)
    {
        unsigned code = codes();
        unsigned *window = (unsigned *)(buf + nbits * (cnt % 8) / 8);
        *window |= code << (cnt % 8) * (nbits - 8) % 8;
        ++cnt;

        if (cnt % 8 == 0 || code == 256)
        {
            os.write(buf, nbits);
            fill(buf, buf + sizeof(buf), 0);
        }

        if (code == 256)
            nbits = 9, cnt = 0;

        if (nbits != bitdepth && cnt == 1U << nbits - 1)
            ++nbits, cnt = 0;
    }

    auto dv = div((cnt % 8) * nbits, 8);
    os.write(buf, dv.quot + (dv.rem ? 1 : 0));
    os.flush();
}

int main(int argc, char **argv)
{
    static constexpr unsigned bitdepth = 16;
    istream *is = &cin;
    ostream *os = &cout;
    ifstream ifs;

    if (argc > 1)
        ifs.open(argv[1]), is = &ifs;

    os->put(0x1f);
    os->put(0x9d);
    os->put(bitdepth | 0x80);
    press(codify(*is), *os, bitdepth);
    return 0;
}



