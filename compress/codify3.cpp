//This is a comment
//I love comments

#include <cstdint>
#include <cassert>
#include <iostream>

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
    auto os = &std::cout;
    static constexpr unsigned HSIZE = 69001;
    uint16_t codetab[HSIZE];
    std::fill(codetab, codetab + HSIZE, 0);
    Fcode fcode;
    fcode.e.ent = is->get();
    unsigned n_bits = 9, free_ent = 257;
    unsigned extcode = (1 << n_bits) + 1, htab[HSIZE];
    bool stcode = true;

    for (int byte; (byte = is->get()) != -1;)
    {
        if (free_ent >= extcode && fcode.e.ent < 257)
        {
            if (n_bits < 16)
                ++n_bits, extcode = n_bits < 16 ? (1 << n_bits) + 1 : 1 << 16;
            else
            {
                std::fill(codetab, codetab + HSIZE, 0);
                *os << 256 << "\r\n";
                n_bits = 9, stcode = true, free_ent = 257;
                extcode = n_bits < 16 ? (1 << n_bits) + 1 : 1 << n_bits;
            }
        }

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
            *os << fcode.e.ent << "\r\n";
            fc = fcode.code;
            fcode.e.ent = fcode.e.c;

            if (stcode)
                codetab[hp] = free_ent++, htab[hp] = fc;
        }
    }

    *os << fcode.e.ent << "\r\n";
    return 0;
}


