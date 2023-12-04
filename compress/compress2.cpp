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
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>

#define	IBUFSIZ	BUFSIZ
#define	OBUFSIZ	BUFSIZ
#define FIRST 257
#define HSIZE 69001

#define	output(b,o,c)	{ uint8_t *p = &(b)[(o)>>3];\
    long i = ((long)(c))<<((o)&0x7);\
    p[0] |= uint8_t(i);\
    p[1] |= uint8_t(i>>8);\
    p[2] |= uint8_t(i>>16);}

union Fcode
{
    long code;
    struct
    {
        uint8_t c;
        uint16_t ent;
    } e;
};

int main(int argc, char **argv)
{
    static constexpr long CHECK_GAP = 10000;
    const int fdout = 1;
    uint8_t inbuf[IBUFSIZ + 64];
    uint8_t outbuf[OBUFSIZ + 2048];
    uint64_t htab[HSIZE];
    uint16_t codetab[HSIZE];
    long bytes_in = 0, bytes_out = 0, hp, fc, checkpoint = CHECK_GAP;
    int rpos, outbits, rlop, stcode = 1, boff, n_bits = 9, ratio = 0;
    uint32_t free_ent = FIRST;
    uint32_t extcode = (1 << n_bits) + 1;
    Fcode fcode;
    memset(outbuf, 0, sizeof(outbuf));
    outbuf[0] = 0x1f;
    outbuf[1] = 0x9d;
    outbuf[2] = char(16 | 0x80);
    boff = outbits = 3 << 3;
    fcode.code = 0;
    memset(htab, -1, sizeof(htab));
    ssize_t rsize;

    while ((rsize = read(0, inbuf, IBUFSIZ)) > 0)
    {
        if (bytes_in == 0)
        {
            fcode.e.ent = inbuf[0];
            rpos = 1;
        }
        else
            rpos = 0;

        rlop = 0;

        do
        {
            if (free_ent >= extcode && fcode.e.ent < FIRST)
            {
                if (n_bits < 16)
                {
                    const uint8_t nb3 = n_bits << 3;
                    boff = outbits = outbits - 1 + nb3 - ((outbits - boff - 1 + nb3) % nb3);
                    ++n_bits;
                    extcode = n_bits < 16 ? (1 << n_bits) + 1 : 1 << 16;
                }
                else
                {
                    extcode = (1 << 16) + OBUFSIZ;
                    stcode = 0;
                }
            }

            if (!stcode && bytes_in >= checkpoint && fcode.e.ent < FIRST)
            {
                long int rat;
                checkpoint = bytes_in + CHECK_GAP;

                if (bytes_in > 0x007fffff)
                {
                    //shift will overflow
                    rat = (bytes_out + (outbits >> 3)) >> 8;
                    rat = rat == 0 ? 0x7fffffff : bytes_in / rat;
                }
                else
                    //8 fractional bits
                    rat = (bytes_in << 8) / (bytes_out + (outbits >> 3));	

                if (rat >= ratio)
                    ratio = int(rat);
                else
                {
                    ratio = 0;
                    memset(htab, -1, sizeof(htab));
                    output(outbuf, outbits, 256);
                    outbits += n_bits;
                    const uint8_t nb3 = n_bits << 3;
                    boff = outbits = outbits - 1 + nb3 - ((outbits - boff - 1 + nb3) % nb3);
                    n_bits = 9, stcode = 1, free_ent = FIRST;
                    extcode = 1 << n_bits;
                    if (n_bits < 16) ++extcode;
                }
            }

            if (outbits >= OBUFSIZ << 3)
            {
                assert(write(fdout, outbuf, OBUFSIZ) == OBUFSIZ);
                outbits -= OBUFSIZ << 3;
                boff = -(((OBUFSIZ << 3) - boff) % (n_bits << 3));
                bytes_out += OBUFSIZ;
                memcpy(outbuf, outbuf + OBUFSIZ, (outbits >> 3) + 1);
                memset(outbuf + (outbits >> 3) + 1, '\0', OBUFSIZ);
            }

            {
                int i = std::min(int(rsize - rlop), int(extcode - free_ent));
                i = std::min(i, int(((sizeof(outbuf) - 32) * 8 - outbits) / n_bits));

                if (!stcode)
                    i = std::min(i, int(checkpoint - bytes_in));

                rlop += i, bytes_in += i;
            }

            while (rpos < rlop)
next2:
            {
                fcode.e.c = inbuf[rpos++];
                uint64_t i;
                fc = fcode.code;
                hp = long(fcode.e.c) <<  8 ^ long(fcode.e.ent);
    
                if ((i = htab[hp]) == fc)
                {
                    fcode.e.ent = codetab[hp];
    
                    if (rpos >= rlop)
                        break;
    
                    continue;
                }

                if (i != -1)
                {
                    //secondary hash (after G. Knott)
                    long disp = HSIZE - hp - 1;	

                    while (i != -1)
                    {
                        if ((hp -= disp) < 0)
                            hp += HSIZE;

                        if ((i = htab[hp]) == fc)
                        {
                            fcode.e.ent = codetab[hp];

                            if (rpos >= rlop)
                                goto endlop;
    
                            goto next2;
                        }
                    }
                }

                output(outbuf, outbits, fcode.e.ent);
                outbits += n_bits;

	    		{
                    const long fc = fcode.code;
                    fcode.e.ent = fcode.e.c;

                    if (stcode)
                    {
                        codetab[hp] = (uint16_t)free_ent++;
                        htab[hp] = fc;
                    }
                }
            }
endlop:
            if (fcode.e.ent >= FIRST && rpos < rsize)
                goto next2;

            if (rpos > rlop)
            {
                bytes_in += rpos-rlop;
                rlop = rpos;
            }
        }
        while (rlop < rsize);
    }

    assert(rsize >= 0);

	if (bytes_in > 0)
    {
		output(outbuf, outbits, fcode.e.ent);
        outbits += n_bits;
    }

    assert(write(fdout, outbuf, outbits + 7 >> 3) == outbits + 7 >> 3);
	bytes_out += outbits + 7 >> 3;
    fprintf(stderr, "Compression: ");
    int q;
    long num = bytes_in - bytes_out;

    if (bytes_in > 0)
    {
        if (num > 214748L)
            q = int(num / (bytes_in / 10000L));
        else
            q = int(10000 * num / bytes_in);
    }
    else q = 10000;

    if (q < 0)
        putc('-', stderr), q = -q;

    fprintf(stderr, "%d.%02d%%", q / 100, q % 100);
    fprintf(stderr, "\n");
    return 0;
}


