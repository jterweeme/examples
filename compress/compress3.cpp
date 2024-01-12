//This is a comment
//I love comments

#include "generator.h"
#include "mystl.h"
#include <iostream>
#include <fstream>

using mystl::cin;
using std::cout;
using std::cerr;
using mystl::ifstream;
using mystl::istream;
using std::ostream;
using std::fill;
using std::div;

class Toolbox
{
public:
    template <class T> static T min(T a, T b) { return b < a ? b : a; }
    template <class T> static T max(T a, T b) { return a < b ? b : a; }

    static char nibble(uint8_t n)
    { return n <= 9 ? '0' + char(n) : 'a' + char(n - 10); }

    static void hex32(unsigned dw, ostream &os)
    { for (unsigned i = 0; i <= 28; i += 4) os.put(nibble(dw >> 28 - i & 0xf)); }
};

class Dictionary
{
    static constexpr unsigned HSIZE = 69001;
    uint16_t codetab[HSIZE];
    unsigned htab[HSIZE];
public:
    unsigned free_ent;
    void clear() { fill(codetab, codetab + HSIZE, 0), free_ent = 257; }
    Dictionary() { clear(); }

    uint16_t find(unsigned c, unsigned ent)
    {
        unsigned hp = c << 8 ^ ent;

        while (codetab[hp])
        {
            if (htab[hp] == (c << 16 | ent))
                return codetab[hp];
    
            if ((hp += hp + 1) >= HSIZE)
                hp -= HSIZE;
        }
    
        codetab[hp] = free_ent++;
        htab[hp] = c << 16 | ent;
        return 0;
    }
};

static Generator<unsigned> test(istream &is)
{
    Dictionary dict;
    unsigned ent = is.get();
    typedef Toolbox T;
    unsigned a = 0;

    for (int byte; (byte = is.get()) != -1;)
    {
        unsigned x = dict.find(byte, ent);
        cerr << a++ << " ";
        T::hex32(x, cerr);
        cerr << "\r\n";

        if (x)
            ent = x;
        else
        {
            co_yield ent;
            ent = byte;
        }
    }

    co_yield ent;
}

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
            if (n_bits < 16)
                ++n_bits, extcode = n_bits < 16 ? (1 << n_bits) + 1 : 1 << 16;
            else
            {
                co_yield 256;
                dict.clear();
                n_bits = 9;
                extcode = 513;
            }
        }

        uint16_t x = dict.find(byte, ent);

        if (x)
            ent = x;
        else
        {
            co_yield ent;
            ent = byte;
        }
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


