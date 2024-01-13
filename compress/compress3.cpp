//This is a comment
//I love comments

#include "generator.h"
#include "mystl.h"
#include <iostream>
#include <fstream>

using mystl::cin;
using mystl::cout;
using mystl::cerr;
using mystl::ifstream;
using mystl::istream;
using mystl::ostream;
using std::fill;
using std::div;

class Dictionary
{
    static constexpr unsigned HSIZE = 69001;
    uint16_t codetab[HSIZE];
    uint32_t htab[HSIZE];
public:
    unsigned free_ent;
    void clear() { fill(htab, htab + HSIZE, 0xffffffff), free_ent = 257; }
    Dictionary() { clear(); }

    uint16_t find(unsigned c, unsigned ent)
    {
        unsigned hp = c << 8 ^ ent;
        unsigned disp = HSIZE - hp - 1;

        while (htab[hp] != 0xffffffff)
        {
            if (htab[hp] == (c << 16 | ent))
                return codetab[hp];

            hp = hp < disp ? hp + HSIZE - disp : hp - disp;
        }

        codetab[hp] = free_ent++;
        htab[hp] = c << 16 | ent;
        return 0;
    }
};

static Generator<unsigned> codify(istream &is)
{
    Dictionary dict;
    unsigned ent = is.get();
    unsigned n_bits = 9;
    unsigned extcode = 513;

    for (int byte; (byte = is.get()) != -1;)
    {
        if (dict.free_ent >= extcode && ent < 257)
        {
            if (++n_bits > 16)
            {
                co_yield 256;
                dict.clear();
                n_bits = 9;
            }

            extcode = 1 << n_bits;

            if (n_bits < 16)
                ++extcode;
        }

        uint16_t x = dict.find(byte, ent);

        if (x == 0)
            co_yield ent;

        ent = x ? x : byte;
    }

    co_yield ent;
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

        if (++cnt % 8 == 0 || code == 256)
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


