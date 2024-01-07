//This is a comment
//I love comments

#include <cstdint>
#include <cstring>
#include <cassert>
#include <iostream>
#include <vector>

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

int main(int argc, char **argv)
{
    auto is = &std::cin;
    static constexpr unsigned CHECK_GAP = 10000, HSIZE = 69001;
    uint16_t codetab[HSIZE];
    std::fill(codetab, codetab + HSIZE, 0);
    BitOutputStream bos(std::cout);
    bos.cnt = 0;
    Fcode fcode;
    fcode.e.ent = is->get();
    unsigned n_bits = 9, checkpoint = CHECK_GAP, free_ent = 257;
    unsigned ratio = 0, extcode = (1 << n_bits) + 1, htab[HSIZE];
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
                std::fill(codetab, codetab + HSIZE, 0);
                bos.write(256, n_bits);
                n_bits = 9, stcode = true, free_ent = 257;
                extcode = n_bits < 16 ? (1 << n_bits) + 1 : 1 << n_bits;
            }
        }

        ++bytes_in;
        fcode.e.c = byte;
        unsigned fc = fcode.code;
        unsigned hp = fcode.e.c << 8 ^ fcode.e.ent;
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

        if (!hfound)
        {
            bos.write(fcode.e.ent, n_bits);
            fc = fcode.code;
            fcode.e.ent = fcode.e.c;

            if (stcode)
                codetab[hp] = free_ent++, htab[hp] = fc;
        }
    }

    bos.write(fcode.e.ent, n_bits);
    return 0;
}


