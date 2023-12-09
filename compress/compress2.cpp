/* (N)compress.c - File compression ala IEEE Computer, Mar 1992.
 *
 * Authors:
 *   Spencer W. Thomas   (decvax!harpo!utah-cs!utah-gr!thomas)
 *   Jim McKie           (decvax!mcvax!jim)
 *   Steve Davies        (decvax!vax135!petsd!peora!srd)
 *   Ken Turkowski       (decvax!decwrl!turtlevax!ken)
 *   James A. Woods      (decvax!ihnp4!ames!jaw)
 *   Joe Orost           (decvax!vax135!petsd!joe)
 *   Dave Mack           (csu@alembic.acs.com)
 *   Peter Jannesen, Network Communication Systems
 *                       (peter@ncs.nl)
 *   Mike Frysinger      (vapier@gmail.com)
 */

#include <cstdint>
#include <cstring>
#include <cassert>
#include <iostream>

#define	IBUFSIZ	BUFSIZ
#define OBUFSIZ BUFSIZ
#define FIRST 257
#define HSIZE 69001

union Fcode
{
    long code;
    struct
    {
        uint8_t c;
        uint16_t ent;
    } e;
};

class BitOutputStream
{
    std::ostream &_os;
    uint32_t _window = 0;
    uint32_t _bits = 0;
public:
    uint64_t _cnt = 0;
    uint64_t cnt() const { return _cnt; }
    BitOutputStream(std::ostream &os) : _os(os) { }

    void write(uint16_t code, uint8_t n_bits)
    {
        _window |= code << _bits, _cnt += n_bits, _bits += n_bits;
        while (_bits >= 8) flush();
    }

    void flush()
    {
        if (_bits)
        {
            const uint32_t bits = std::min(_bits, 8U);
            _os.put(_window & 0xff);
            _window = _window >> bits, _bits -= bits;
        }
    }
};

int main(int argc, char **argv)
{
    static constexpr long CHECK_GAP = 10000;
    uint8_t inbuf[IBUFSIZ + 64];
    int64_t htab[HSIZE];
    uint16_t codetab[HSIZE];
    long bytes_in = 0, hp, fc, checkpoint = CHECK_GAP;
    int rpos, stcode = 1, n_bits = 9, ratio = 0;
    uint32_t free_ent = FIRST;
    uint32_t extcode = (1 << n_bits) + 1;
    Fcode fcode;
    BitOutputStream bos(std::cout);
    bos.write(0x9d1f, 16);  //magic
    bos.write(16, 5);       //max. 16 bits (hardcoded)
    bos.write(0, 2);        //reserved
    bos.write(1, 1);        //block mode
    bos._cnt = 0;
    fcode.code = 0;
    std::fill(htab, htab + HSIZE, -1);

    while (true)
    {
        std::cin.read((char *)inbuf, IBUFSIZ);

        if (std::cin.gcount() <= 0)
            break;

        bytes_in == 0 ? fcode.e.ent = inbuf[0], rpos = 1 : rpos = 0;
        int rlop = 0;

        do
        {
            if (free_ent >= extcode && fcode.e.ent < FIRST)
            {
                if (n_bits < 16)
                    ++n_bits, extcode = n_bits < 16 ? (1 << n_bits) + 1 : 1 << 16;
                else
                    extcode = (1 << 16) + OBUFSIZ, stcode = 0;
            }

            if (!stcode && bytes_in >= checkpoint && fcode.e.ent < FIRST)
            {
                long rat;
                checkpoint = bytes_in + CHECK_GAP;

                if (bytes_in > 0x007fffff)
                    //shift will overflow
                    rat = bos.cnt() >> 11 == 0 ? 0x7fffffff : bytes_in / (bos.cnt() >> 11);
                else
                    //8 fractional bits
                    rat = (bytes_in << 8) / (bos.cnt() >> 3);

                if (rat >= ratio)
                    ratio = int(rat);
                else
                {
                    ratio = 0;
                    std::fill(htab, htab + HSIZE, -1);
                    bos.write(256, n_bits);
                    const uint8_t nb3 = n_bits << 3;

                    while (nb3 - (bos.cnt() - 1 + nb3) % nb3 - 1 > 0)
                        bos.write(0, 16);

                    n_bits = 9, stcode = 1, free_ent = FIRST;
                    extcode = n_bits < 16 ? (1 << n_bits) + 1 : 1 << n_bits;
                }
            }

            ++rlop, ++bytes_in;
            bool flag = false;
loop1:
            while (rpos < rlop || flag)
            {
                flag = false;
                fcode.e.c = inbuf[rpos++];
                fc = fcode.code;
                hp = long(fcode.e.c) <<  8 ^ long(fcode.e.ent);
    
                if (htab[hp] == fc)
                {
                    fcode.e.ent = codetab[hp];
                    continue;
                }
                
                //secondary hash (after G. Knott)
                while (htab[hp] != -1)
                {
                    if ((hp -= HSIZE - hp - 1) < 0)
                        hp += HSIZE;

                    if (htab[hp] == fc)
                    {
                        fcode.e.ent = codetab[hp];
                        goto loop1;
                    }
                }
                bos.write(fcode.e.ent, n_bits);
                fc = fcode.code;
                fcode.e.ent = fcode.e.c;

                if (stcode)
                    codetab[hp] = uint16_t(free_ent++), htab[hp] = fc;
            }

            if (fcode.e.ent >= FIRST && rpos < std::cin.gcount())
            {
                flag = true;
                goto loop1;
            }

            if (rpos > rlop)
                bytes_in += rpos - rlop, rlop = rpos;
        }
        while (rlop < std::cin.gcount());
    }

	if (bytes_in > 0)
        bos.write(fcode.e.ent, n_bits);

    bos.flush();
    std::cerr << "Compression: ";
    int q;
    const long num = bytes_in - bos.cnt() / 8;

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


