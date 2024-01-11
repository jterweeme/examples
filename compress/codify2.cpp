//This is a comment
//I love comments

#include <cstdint>
#include <cstring>
#include <cassert>
#include <iostream>
#include <vector>

using std::istream;
using std::cin;
using std::fill;

class BitOutputStream
{
    std::ostream &_os;
    unsigned _window = 0, _bits = 0;
public:
    uint64_t cnt = 0;
    BitOutputStream(std::ostream &os) : _os(os) { }

    void write(uint16_t code, unsigned n_bits)
    {
        std::cout << code << "\r\n";
        cnt += n_bits;
    }
};

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

uint16_t codetab[HSIZE];
unsigned htab[HSIZE];
unsigned free_ent = 257;

uint16_t find(unsigned &hp, unsigned fc)
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

int main(int argc, char **argv)
{
    istream *is = &cin;
    Dictionary dict;
    dict.clear();
    std::fill(codetab, codetab + HSIZE, 0);
    BitOutputStream bos(std::cout);
    bos.cnt = 0;
    Fcode fcode;
    fcode.e.ent = is->get();
    unsigned n_bits = 9, checkpoint = CHECK_GAP;
    unsigned ratio = 0, extcode = (1 << n_bits) + 1;
    bool stcode = true;
    uint64_t bytes_in = 1;

    for (int byte; (byte = is->get()) != -1;)
    {
        if (free_ent >= extcode && fcode.e.ent < 257)
        {
            if (n_bits < 16)
                ++n_bits, extcode = n_bits < 16 ? (1 << n_bits) + 1 : 1 << 16;
            else
                stcode = false;
        }

        if (!stcode && bytes_in >= checkpoint && fcode.e.ent < 257)
        {
            checkpoint = bytes_in + CHECK_GAP;
            unsigned rat = (bytes_in << 8) / (bos.cnt >> 3);

            if (rat >= ratio)
                ratio = rat;
            else
            {
                ratio = 0;
                dict.clear();
                std::fill(codetab, codetab + HSIZE, 0);
                bos.write(256, n_bits);
                n_bits = 9, stcode = true, free_ent = 257;
                extcode = n_bits < 16 ? (1 << n_bits) + 1 : 1 << n_bits;
            }
        }

        ++bytes_in;
        fcode.e.c = byte;
        //unsigned fc = fcode.code;
        unsigned hp = fcode.e.c << 8 ^ fcode.e.ent;
        uint16_t x = find(hp, fcode.code);
#if 0
        bool hfound = false;

        while (codetab[hp])
        {
            if (htab[hp] == fc)
            {
                fcode.e.ent = codetab[hp];
                hfound = true;
                break;
            }

            if ((hp += hp + 1) >= HSIZE)
                hp -= HSIZE;
        }
#endif
        //if (!hfound)
        if (!x)
        {
            bos.write(fcode.e.ent, n_bits);
            unsigned fc = fcode.code;
            fcode.e.ent = fcode.e.c;

            if (stcode)
                //dict.store(hp, fc);
                codetab[hp] = free_ent++, htab[hp] = fc;
        }
        else
        {
            fcode.e.ent = codetab[hp];
        }
    }

    bos.write(fcode.e.ent, n_bits);
    return 0;
}


