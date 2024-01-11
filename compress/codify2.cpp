//This is a comment
//I love comments

#include "mystl.h"
#include <cstdint>
#include <cassert>
#include <iostream>

using mystl::istream;
using mystl::cin;
using std::cout;
using std::fill;

union Fcode
{
    uint32_t code = 0;
    struct
    {
        uint16_t c;
        uint16_t ent;
    } e;
};

static constexpr unsigned CHECK_GAP = 10000, HSIZE = 69001;

class Dictionary
{
    uint16_t codetab[HSIZE];
    unsigned htab[HSIZE];
public:
    unsigned free_ent;
    void clear() { fill(codetab, codetab + HSIZE, 0), free_ent = 257; }
    Dictionary() { clear(); }
    void store(unsigned hp, unsigned fc) { codetab[hp] = free_ent++, htab[hp] = fc; }

    uint16_t find(unsigned &hp, unsigned fc) const
    {
        while (codetab[hp])
        {
            if (htab[hp] == fc)
                return codetab[hp];

            if ((hp += hp + 1) >= HSIZE)
                hp -= HSIZE;
        }

        return 0;
    }
};

int main(int argc, char **argv)
{
    istream *is = &cin;
    Dictionary dict;
    uint64_t cnt = 0;
    Fcode fcode;
    fcode.e.ent = is->get();
    unsigned n_bits = 9, checkpoint = CHECK_GAP;
    unsigned ratio = 0, extcode = (1 << n_bits) + 1;
    bool stcode = true;
    uint64_t bytes_in = 1;

    for (int byte; (byte = is->get()) != -1;)
    {
        if (dict.free_ent >= extcode && fcode.e.ent < 257)
        {
            if (n_bits < 16)
                ++n_bits, extcode = n_bits < 16 ? (1 << n_bits) + 1 : 1 << 16;
            else
                stcode = false;
        }

        if (!stcode && bytes_in >= checkpoint && fcode.e.ent < 257)
        {
            checkpoint = bytes_in + CHECK_GAP;
            unsigned rat = (bytes_in << 8) / (cnt >> 3);

            if (rat >= ratio)
                ratio = rat;
            else
            {
                ratio = 0;
                dict.clear();
                cout << 256 << "\r\n";
                cnt += n_bits;
                n_bits = 9, stcode = true;
                extcode = n_bits < 16 ? (1 << n_bits) + 1 : 1 << n_bits;
            }
        }

        ++bytes_in;
        fcode.e.c = byte;
        unsigned hp = fcode.e.c << 8 ^ fcode.e.ent;
        uint16_t x = dict.find(hp, fcode.code);

        if (!x)
        {
            cout << fcode.e.ent << "\r\n";
            cnt += n_bits;
            unsigned fc = fcode.code;
            fcode.e.ent = fcode.e.c;

            if (stcode)
                dict.store(hp, fc);
        }
        else
        {
            fcode.e.ent = x;
        }
    }

    cout << fcode.e.ent << "\r\n";
    cnt += n_bits;
    return 0;
}


