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
#define OBUFSIZ BUFSIZ
#define FIRST 257
#define HSIZE 69001
#define NIEUW

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
    uint64_t _outbits = 0;
    uint64_t _cnt = 0;
    uint8_t _outbuf[OBUFSIZ + 2048];
    uint32_t _window = 0;
    uint32_t _bits = 0;
public:
    uint64_t cnt() const { return _cnt; }
    BitOutputStream(std::ostream &os) : _os(os) { memset(_outbuf, 0, sizeof(_outbuf)); }
#ifndef NIEUW
    uint64_t outbits() const { return _outbits; }

    void flush1()
    {
        _os.write((const char *)_outbuf, OBUFSIZ);
        _outbits -= OBUFSIZ << 3;
        memcpy(_outbuf, _outbuf + OBUFSIZ, _outbits / 8 + 1);
        memset(_outbuf + _outbits / 8 + 1, 0, OBUFSIZ);
    }

    void flush2()
    {
        _os.write((const char *)_outbuf, _outbits + 7 >> 3);
    }

    void write(uint16_t code, uint8_t n_bits)
    {
        uint8_t *p = _outbuf + (_outbits >> 3);
        long i = long(code) << (_outbits & 7);
        p[0] |= uint8_t(i);
        p[1] |= uint8_t(i >> 8);
        p[2] |= uint8_t(i >> 16);
        _outbits += n_bits, _cnt += n_bits;
    }
#else
    void write(uint16_t code, uint8_t n_bits)
    {
        _window |= code << _bits;
        _bits += n_bits, _cnt += n_bits;
        
        while (_bits >= 8)
        {
            _os.put(_window & 0xff);
            _window = _window >> 8, _bits -= 8;
        }
    }

    void flush2()
    {
        if (_bits > 0)
            _os.put(_window & 0xff);
    }
#endif
};

int main(int argc, char **argv)
{
    static constexpr long CHECK_GAP = 10000;
    BitOutputStream bos(std::cout);
    uint8_t inbuf[IBUFSIZ + 64];
    int64_t htab[HSIZE];
    uint16_t codetab[HSIZE];
    long bytes_in = 0, hp, fc, checkpoint = CHECK_GAP;
    int rpos, stcode = 1, n_bits = 9, ratio = 0;
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
                    uint32_t foo = nb3 - (bos.cnt() - 1 + nb3) % nb3 - 1;

                    while (foo > 0)
                        bos.write(0, 16), foo -= 16;

                    n_bits = 9, stcode = 1, free_ent = FIRST;
                    extcode = n_bits < 16 ? (1 << n_bits) + 1 : 1 << n_bits;
                }
            }
#ifndef NIEUW
            if (bos.outbits() >= OBUFSIZ << 3)
                bos.flush1();
#endif
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

            if (fcode.e.ent >= FIRST && rpos < rsize)
            {
                flag = true;
                goto loop1;
            }

            if (rpos > rlop)
                bytes_in += rpos - rlop, rlop = rpos;
        }
        while (rlop < rsize);
    }

	if (bytes_in > 0)
        bos.write(fcode.e.ent, n_bits);
#ifndef NIEUW
    bos.flush2();
#endif
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

    std::cerr << q / 100 << "." << q % 100 << "\r\n";
    std::cerr.flush();
    return 0;
}


