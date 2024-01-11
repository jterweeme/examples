//This is a comment
//I love comments

#include <cstdint>
#include <cstring>
#include <cassert>
#include <iostream>
#include <fstream>

using std::istream;
using std::ifstream;
using std::cin;
using std::cout;
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
        _window |= code << _bits, cnt += n_bits, _bits += n_bits;
        while (_bits >= 8) flush();
    }

    void flush()
    {
        if (_bits)
        {
            const unsigned bits = std::min(_bits, 8U);
            _os.put(_window & 0xff);
            _window = _window >> bits, _bits -= bits;
        }
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
    istream *is = &std::cin;
    ifstream ifs;

    if (argc > 1)
        ifs.open(argv[1]), is = &ifs;

    Dictionary dict;
    BitOutputStream bos(cout);
    bos.write(0x9d1f, 16);  //magic
    bos.write(16, 7);       //max. 16 bits (hardcoded)
    bos.write(1, 1);        //block mode
    bos.cnt = 0;
    Fcode fcode;
    fcode.e.ent = is->get();
    unsigned n_bits = 9, checkpoint = CHECK_GAP;
    unsigned ratio = 0, extcode = (1 << n_bits) + 1;
    bool stcode = true;
    unsigned bytes_in = 1;

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
            unsigned rat = (bytes_in << 8) / (bos.cnt >> 3);

            if (rat >= ratio)
                ratio = rat;
            else
            {
                ratio = 0;
                dict.clear();
                bos.write(256, n_bits);

                for (unsigned nb3 = n_bits << 3; (bos.cnt - 1U + nb3) % nb3 != nb3 - 1U;)
                    bos.write(0, 16);

                n_bits = 9, stcode = true;
                extcode = n_bits < 16 ? (1 << n_bits) + 1 : 1 << n_bits;
            }
        }

        ++bytes_in;
        fcode.e.c = byte;
        unsigned hp = fcode.e.c << 8 ^ fcode.e.ent;
        uint16_t x = dict.find(hp, fcode.code);

        if (x)
            fcode.e.ent = x;
        else
        {
            bos.write(fcode.e.ent, n_bits);
            unsigned fc = fcode.code;
            fcode.e.ent = fcode.e.c;

            if (stcode)
                dict.store(hp, fc);
        }
    }

    bos.write(fcode.e.ent, n_bits);
    bos.flush();
    return 0;
}


