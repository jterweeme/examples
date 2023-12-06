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

#if 0
#define FAST
#define	HSIZE (1<<17)
#define	HMASK (HSIZE-1)
#else
#define HSIZE 69001
#endif

#define	output(b,o,c)	{ uint8_t *p = &(b)[(o)>>3];\
    long i = ((long)(c))<<((o)&0x7);\
    p[0] |= uint8_t(i);\
    p[1] |= uint8_t(i>>8);\
    p[2] |= uint8_t(i>>16);}

//Special secudary hash table.
static const int primetab[256] =
{
   	 1013, -1061, 1109, -1181, 1231, -1291, 1361, -1429,
   	 1481, -1531, 1583, -1627, 1699, -1759, 1831, -1889,
   	 1973, -2017, 2083, -2137, 2213, -2273, 2339, -2383,
   	 2441, -2531, 2593, -2663, 2707, -2753, 2819, -2887,
   	 2957, -3023, 3089, -3181, 3251, -3313, 3361, -3449,
   	 3511, -3557, 3617, -3677, 3739, -3821, 3881, -3931,
   	 4013, -4079, 4139, -4219, 4271, -4349, 4423, -4493,
   	 4561, -4639, 4691, -4783, 4831, -4931, 4973, -5023,
   	 5101, -5179, 5261, -5333, 5413, -5471, 5521, -5591,
   	 5659, -5737, 5807, -5857, 5923, -6029, 6089, -6151,
   	 6221, -6287, 6343, -6397, 6491, -6571, 6659, -6709,
   	 6791, -6857, 6917, -6983, 7043, -7129, 7213, -7297,
   	 7369, -7477, 7529, -7577, 7643, -7703, 7789, -7873,
   	 7933, -8017, 8093, -8171, 8237, -8297, 8387, -8461,
   	 8543, -8627, 8689, -8741, 8819, -8867, 8963, -9029,
   	 9109, -9181, 9241, -9323, 9397, -9439, 9511, -9613,
   	 9677, -9743, 9811, -9871, 9941,-10061,10111,-10177,
    10259,-10321,10399,-10477,10567,-10639,10711,-10789,
    10867,-10949,11047,-11113,11173,-11261,11329,-11423,
   	11491,-11587,11681,-11777,11827,-11903,11959,-12041,
   	12109,-12197,12263,-12343,12413,-12487,12541,-12611,
   	12671,-12757,12829,-12917,12979,-13043,13127,-13187,
   	13291,-13367,13451,-13523,13619,-13691,13751,-13829,
   	13901,-13967,14057,-14153,14249,-14341,14419,-14489,
   	14557,-14633,14717,-14767,14831,-14897,14983,-15083,
   	15149,-15233,15289,-15359,15427,-15497,15583,-15649,
   	15733,-15791,15881,-15937,16057,-16097,16189,-16267,
   	16363,-16447,16529,-16619,16691,-16763,16879,-16937,
   	17021,-17093,17183,-17257,17341,-17401,17477,-17551,
   	17623,-17713,17791,-17891,17957,-18041,18097,-18169,
   	18233,-18307,18379,-18451,18523,-18637,18731,-18803,
   	18919,-19031,19121,-19211,19273,-19381,19429,-19477
};

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

            goto next;
hfound:
            fcode.e.ent = codetab[hp];
next:
            if (rpos >= rlop)
                goto endlop;
next2:
            fcode.e.c = inbuf[rpos++];
#ifndef FAST
            long i;
            fc = fcode.code;
            hp = long(fcode.e.c) <<  8 ^ long(fcode.e.ent);

            if ((i = htab[hp]) == fc)
                goto hfound;

            if (i != -1)
            {
                //secondary hash (after G. Knott)
                long disp = HSIZE - hp - 1;	

                do
                {
                    if ((hp -= disp) < 0)
                        hp += HSIZE;

                    if ((i = htab[hp]) == fc)
                        goto hfound;
                }
                while (i != -1);
            }
#else
            long i;
            fc = fcode.code;
            hp = long(fcode.e.c) << 17 - 8 ^ long(fcode.e.ent);

            if ((i = htab[hp]) == fc)
                goto hfound;

            while (i != -1)
            {
                const long p = primetab[fcode.e.c];
                hp = hp + p & HMASK;
                if ((i = htab[hp]) == fc)
                    goto hfound;
                if (i == -1)
                    break;
                hp = hp + p & HMASK;
                if ((i = htab[hp]) == fc)
                    goto hfound;
	    		if (i == -1)
                    break;
                hp = hp + p & HMASK;
                if ((i = htab[hp]) == fc)
                    goto hfound;
            }
#endif
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

			goto next;
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


