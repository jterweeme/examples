//This is a comment
//I love comments

#include <cstdint>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

#define	IBUFSIZ	8192
#define	OBUFSIZ	8192
#define FIRST 257
#define HSIZE 69001

using std::cout;

union Fcode
{
    long code;
    struct
    {
        uint8_t c;
        uint16_t ent;
    } e;
};

static uint8_t outbuf[OBUFSIZ + 2048];
static int outbits;
static constexpr long CHECK_GAP = 10000;
static uint8_t inbuf[IBUFSIZ + 64];
static uint64_t htab[HSIZE];
static uint16_t codetab[HSIZE];
static long bytes_in = 0, bytes_out = 0, hp, checkpoint = CHECK_GAP;
static int rpos, rlop, stcode = 1, boff = 0, ratio = 0, n_bits = 9;
static uint32_t free_ent = FIRST;
static uint32_t extcode = 513;
static Fcode fcode;

static void output(uint16_t c)
{
    cout << c << "\r\n";
    outbits += n_bits;
}

int main(int argc, char **argv)
{
    memset(outbuf, 0, sizeof(outbuf));
    outbits = boff = 3 << 3;
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
                    const unsigned nb3 = n_bits << 3;
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
                long rat;
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
                    output(256);
                    const unsigned nb3 = n_bits << 3;
                    boff = outbits = outbits - 1 + nb3 - ((outbits - boff - 1 + nb3) % nb3);
                    n_bits = 9, stcode = 1, free_ent = FIRST;
                    extcode = 1 << n_bits;
                    if (n_bits < 16) ++extcode;
                }
            }

            if (outbits >= OBUFSIZ << 3)
            {
                outbits -= OBUFSIZ << 3;
                boff = -(((OBUFSIZ << 3) - boff) % (n_bits << 3));
                bytes_out += OBUFSIZ;
            }

            int i = std::min(int(rsize - rlop), int(extcode - free_ent));
            i = std::min(i, int(((sizeof(outbuf) - 32) * 8 - outbits) / n_bits));

            if (!stcode)
                i = std::min(i, int(checkpoint - bytes_in));

            rlop += i, bytes_in += i;
            bool flag = false;
            bool flag2 = true;

            while (true)
            {
                while (true)
                {
                    if (flag)
                        fcode.e.ent = codetab[hp];

                    if (rpos >= rlop && flag2)
                        break;

                    flag = false;
                    flag2 = true;
                    fcode.e.c = inbuf[rpos++];
                    long i;
                    long fc = fcode.code;
                    hp = long(fcode.e.c) <<  8 ^ long(fcode.e.ent);

                    if ((i = htab[hp]) == fc)
                    {
                        flag = true;
                        continue;
                    }

                    long disp = HSIZE - hp - 1;

                    while (i != -1)
                    {
                        if ((hp -= disp) < 0)
                            hp += HSIZE;

                        if ((i = htab[hp]) == fc)
                        {
                            flag = true;
                            break;
                        }
                    }

                    if (flag)
                        continue;
                
                    output(fcode.e.ent);
				    fc = fcode.code;
    				fcode.e.ent = fcode.e.c;

	    			if (stcode)
		    		{
			    		codetab[hp] = (uint16_t)free_ent++;
				    	htab[hp] = fc;
    				}
                }

                if (fcode.e.ent >= FIRST && rpos < rsize)
                {
                    flag2 = false;
                    continue;
                }

                break;
            }

			if (rpos > rlop)
			{
				bytes_in += rpos - rlop;
				rlop = rpos;
			}
		}
		while (rlop < rsize);
	}

	if (bytes_in > 0)
		output(fcode.e.ent);

    return 0;
}


