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
#include <algorithm>
#include <iostream>
#include <unistd.h>

#define	IBUFSIZ	BUFSIZ
#define	OBUFSIZ	BUFSIZ
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
    long _bytes_out = 0;
    int boff = 0;
    int _outbits = 0;
    uint8_t outbuf[OBUFSIZ + 2048];
public:
    int outbits() const { return _outbits; }
    long bytes_out() const { return _bytes_out; }
    BitOutputStream(std::ostream &os) : _os(os) { memset(outbuf, 0, sizeof(outbuf)); }

    void flush1()
    {
        _os.write((const char *)outbuf, OBUFSIZ);
        _outbits -= OBUFSIZ << 3;
        memcpy(outbuf, outbuf + OBUFSIZ, (_outbits >> 3) + 1);
        memset(outbuf + (_outbits >> 3) + 1, '\0', OBUFSIZ);
        _bytes_out += OBUFSIZ;
    }

    void flush2()
    {
        _os.write((const char *)outbuf, _outbits + 7 >> 3);
        _bytes_out += _outbits + 7 >> 3;
    }

    void de_bof(uint8_t n_bits)
    {
        const uint8_t nb3 = n_bits << 3;
        boff = _outbits = _outbits - 1 + nb3 - ((_outbits - boff - 1 + nb3) % nb3);
    }

    void de_bof2(uint8_t n_bits)
    {
        boff = -(((OBUFSIZ << 3) - boff) % (n_bits << 3));
    }

    void write(uint16_t code, uint8_t n_bits)
    {
        uint8_t *p = &outbuf[_outbits >> 3];
        long i = long(code) << (_outbits & 7);
        p[0] |= uint8_t(i);
        p[1] |= uint8_t(i >> 8);
        p[2] |= uint8_t(i >> 16);
        _outbits += n_bits;
    }
};

int main(int argc, char **argv)
{
    static constexpr long CHECK_GAP = 10000;
    BitOutputStream bos(std::cout);
    uint8_t inbuf[IBUFSIZ + 64];
    int64_t htab[HSIZE];
    uint16_t codetab[HSIZE];
    long bytes_in = 0, hp, fc, checkpoint = CHECK_GAP;
    int rpos, rlop, stcode = 1, n_bits = 9, ratio = 0;
    uint32_t free_ent = FIRST;
    uint32_t extcode = (1 << n_bits) + 1;
    Fcode fcode;
    std::cout.put(0x1f);
    std::cout.put(0x9d);
    std::cout.put(16 | 0x80);
    fcode.code = 0;
    std::fill(htab, htab + HSIZE, -1);

    for (ssize_t rsize; (rsize = read(0, inbuf, IBUFSIZ)) > 0;)
    {
        bytes_in == 0 ? fcode.e.ent = inbuf[0], rpos = 1 : rpos = 0;
        rlop = 0;

        do
        {
            if (free_ent >= extcode && fcode.e.ent < FIRST)
            {
                if (n_bits < 16)
                    bos.de_bof(n_bits++), extcode = n_bits < 16 ? (1 << n_bits) + 1 : 1 << 16;
                else
                    extcode = (1 << 16) + OBUFSIZ, stcode = 0;
            }

            if (!stcode && bytes_in >= checkpoint && fcode.e.ent < FIRST)
            {
                long rat;
                checkpoint = bytes_in + CHECK_GAP;
                const long foo = bos.bytes_out() + (bos.outbits() >> 3);

                if (bytes_in > 0x007fffff)
                    //shift will overflow
                    rat = foo >> 8 == 0 ? 0x7fffffff : bytes_in / (foo >> 8);
                else
                    //8 fractional bits
                    rat = (bytes_in << 8) / foo;

                if (rat >= ratio)
                    ratio = int(rat);
                else
                {
                    ratio = 0;
                    std::fill(htab, htab + HSIZE, -1);
                    bos.write(256, n_bits);
                    bos.de_bof(n_bits);
                    n_bits = 9, stcode = 1, free_ent = FIRST;
                    extcode = n_bits < 16 ? (1 << n_bits) + 1 : 1 << n_bits;
                }
            }

            if (bos.outbits() >= OBUFSIZ << 3)
                bos.flush1(), bos.de_bof2(n_bits);

            {
                int i2 = std::min(int(rsize - rlop), int(extcode - free_ent));
                i2 = std::min(i2, ((OBUFSIZ + 2048 - 32) * 8 - bos.outbits()) / n_bits);

                if (!stcode)
                    i2 = std::min(i2, int(checkpoint - bytes_in));

                rlop += i2, bytes_in += i2;
            }

loop1:
            while (rpos < rlop)
next2:
            {
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

            if (fcode.e.ent >= FIRST && rpos < rsize)
                goto next2;

            if (rpos > rlop)
                bytes_in += rpos - rlop, rlop = rpos;
        }
        while (rlop < rsize);
    }

	if (bytes_in > 0)
        bos.write(fcode.e.ent, n_bits);

    bos.flush2();
    std::cerr << "Compression: ";
    int q;
    const long num = bytes_in - bos.bytes_out();

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

    std::cerr << q / 100 << "." << q % 100 << "\r\n";
    std::cerr.flush();
    return 0;
}


