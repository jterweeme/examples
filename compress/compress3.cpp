//This is a comment
//I love comments

#include "generator.h"
#include "mystl.h"

using mystl::cin;
using mystl::cout;
using mystl::ifstream;
using mystl::istream;
using mystl::ostream;
using std::fill;
using std::div;

union Fcode
{
    uint32_t code = 0;
    struct
    {
        uint16_t c;
        uint16_t ent;
    } e;
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
    unsigned extcode = 513;

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

        if (x)
            fcode.e.ent = x;
        else
        {
            co_yield fcode.e.ent;
            unsigned fc = fcode.code;
            fcode.e.ent = fcode.e.c;
            dict.store(hp, fc);
        }
    }

    co_yield fcode.e.ent;
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


