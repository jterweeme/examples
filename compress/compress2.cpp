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
    unsigned code = 0;
    struct
    {
        uint8_t c;
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
    bos.write(0x9d1f, 16);  //magic
    bos.write(16, 7);       //max. 16 bits (hardcoded)
    bos.write(1, 1);        //block mode
    bos.cnt = 0;
    Fcode fcode;
    fcode.e.ent = is->get();
    unsigned bytes_in = 1, n_bits = 9, checkpoint = CHECK_GAP, free_ent = 257;
    unsigned ratio = 0, extcode = (1 << n_bits) + 1, htab[HSIZE];
    bool stcode = true;

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
            unsigned rat;
            checkpoint = bytes_in + CHECK_GAP;

            if (bytes_in > 0x007fffff)
                //shift will overflow
                rat = bos.cnt >> 11 == 0 ? 0x7fffffff : bytes_in / (bos.cnt >> 11);
            else
                //8 fractional bits
                rat = (bytes_in << 8) / (bos.cnt >> 3);

            if (rat >= ratio)
                ratio = rat;
            else
            {
                ratio = 0;
                std::fill(codetab, codetab + HSIZE, 0);
                bos.write(256, n_bits);

                for (unsigned nb3 = n_bits << 3; (bos.cnt - 1U + nb3) % nb3 != nb3 - 1U;)
                    bos.write(0, 16);

                n_bits = 9, stcode = true, free_ent = 257;
                extcode = n_bits < 16 ? (1 << n_bits) + 1 : 1 << n_bits;
            }
        }

        ++bytes_in;
        fcode.e.c = byte;
        unsigned fc = fcode.code;
        unsigned hp = fcode.e.c << 8 ^ fcode.e.ent;
 
        bool hfound = false;

        //secondary hash (after G. Knott)
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
    bos.flush();

    //print message
    std::cerr << "Compression: ";
    int q;
    const long num = bytes_in - bos.cnt / 8;

    if (bytes_in > 0)
    {
        if (num > 214748L)
            q = int(num / (bytes_in / 10000L));
        else
            q = int(10000 * num / bytes_in);
    }
    else q = 10000;

    if (q < 0)
        std::cerr.put('-'), q = -q;

    std::cerr << q / 100 << "." << q % 100 << "%\r\n";
    std::cerr.flush();
    return 0;
}


