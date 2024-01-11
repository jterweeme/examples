//This is a comment
//I love comments

#include "generator.h"
#include "mystl.h"
#include <cassert>
#include <iostream>

using mystl::cin;
using std::cout;
using mystl::istream;
using std::ostream;
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

static constexpr unsigned HSIZE = 69001;

class Dictionary
{
    uint16_t codetab[HSIZE];
    unsigned htab[HSIZE];
public:
    unsigned free_ent = 257;
    void clear() { fill(codetab, codetab + HSIZE, 0), free_ent = 257; }
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

static Generator<unsigned> codify(istream &is)
{
    Dictionary dict;
    dict.clear();
    Fcode fcode;
    fcode.e.ent = is.get();
    unsigned n_bits = 9;
    unsigned extcode = (1 << n_bits) + 1;

    for (int byte; (byte = is.get()) != -1;)
    {
        if (dict.free_ent >= extcode && fcode.e.ent < 257)
        {
            if (n_bits < 16)
                ++n_bits, extcode = n_bits < 16 ? (1 << n_bits) + 1 : 1 << 16;
            else
            {
                co_yield 256;
                dict.clear();
                n_bits = 9;
                extcode = 1 << n_bits;
            }
        }

        fcode.e.c = byte;
        unsigned hp = fcode.e.c << 8 ^ fcode.e.ent;
        uint16_t x = dict.find(hp, fcode.code);

        if (!x)
        {
            co_yield fcode.e.ent;
            unsigned fc = fcode.code;
            fcode.e.ent = fcode.e.c;
            dict.store(hp, fc);
        }
        else
        {
            fcode.e.ent = x;
        }
    }

    co_yield fcode.e.ent;
}


int main(int argc, char **argv)
{
    for (auto code = codify(cin); code;)
        cout << code() << "\r\n";

    return 0;
}


