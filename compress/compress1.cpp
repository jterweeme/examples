//This is a comment
//I love comments

#include "generator.h"
#include "mystl.h"
#include <cstring>

using mystl::cin;
using mystl::cout;
using std::min;
using mystl::ifstream;
using mystl::istream;
using mystl::ostream;
using std::fill;

union Fcode
{
    long code;
    struct
    {
        uint8_t c;
        uint16_t ent;
    } e;
};

class Dictionary
{
    static constexpr unsigned HSIZE = 69001;
    uint16_t codetab[HSIZE];
    int64_t htab[HSIZE];
public:
    unsigned free_ent = 257;
    void clear() { memset(htab, -1, sizeof(htab)), free_ent = 257; }
    void store(unsigned hp, unsigned fc) { codetab[hp] = free_ent++, htab[hp] = fc; }

    uint16_t find(long &hp, long fc)
    {
        long disp = HSIZE - hp - 1;

        while (htab[hp] != -1)
        {
            if (htab[hp] == fc)
                return codetab[hp];

            if ((hp -= disp) < 0)
                hp += HSIZE;
        }

        return 0;
    }
};

static Generator<unsigned> codify(istream &is)
{
    static constexpr long CHECK_GAP = 10000;
    static constexpr unsigned IBUFSIZ = 8192;
    static constexpr int OBUFSIZ = 8192;
    uint8_t outbuf[OBUFSIZ + 2048];
    int outbits;
    uint8_t inbuf[IBUFSIZ + 64];
    long bytes_in = 0, bytes_out = 0, checkpoint = CHECK_GAP;
    int n_bits = 9;
    int rpos, rlop, stcode = 1, boff = 0, ratio = 0;
    uint32_t extcode = 513;
    long hp;
    Fcode fcode;
    Dictionary dict;
    dict.clear();
    memset(outbuf, 0, sizeof(outbuf));
    outbits = boff = 3 << 3;
    fcode.code = 0;

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
            if (dict.free_ent >= extcode && fcode.e.ent < 257)
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

            if (!stcode && bytes_in >= checkpoint && fcode.e.ent < 257)
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
                    co_yield 256;
                    ratio = 0;
                    dict.clear();
                    outbits += n_bits;
                    const unsigned nb3 = n_bits << 3;
                    boff = outbits = outbits - 1 + nb3 - ((outbits - boff - 1 + nb3) % nb3);
                    n_bits = 9, stcode = 1, dict.free_ent = 257;
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

            int i = min(int(rsize - rlop), int(extcode - dict.free_ent));
            i = min(i, int(((sizeof(outbuf) - 32) * 8 - outbits) / n_bits));

            if (!stcode)
                i = min(i, int(checkpoint - bytes_in));

            rlop += i, bytes_in += i;
            bool flag2 = true;

            while (true)
            {
                while (true)
                {
                    if (rpos >= rlop && flag2)
                        break;

                    flag2 = true;
                    fcode.e.c = inbuf[rpos++];
                    long fc = fcode.code;
                    hp = long(fcode.e.c) <<  8 ^ long(fcode.e.ent);
                    uint16_t x = dict.find(hp, fc);

                    if (x)
                    {
                        fcode.e.ent = x;
                        continue;
                    }

                    co_yield fcode.e.ent;
                    outbits += n_bits;
				    fc = fcode.code;
    				fcode.e.ent = fcode.e.c;

	    			if (stcode)
                        dict.store(hp, fc);
                }

                if (fcode.e.ent >= 257 && rpos < rsize)
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
    {
        co_yield fcode.e.ent;
        outbits += n_bits;
    }
}

static void press(Generator<unsigned> codes, ostream &os, unsigned bitdepth)
{
    unsigned cnt = 0, nbits = 9;
    char buf[20] = {0};

    while (codes)
    {
        unsigned code = codes();
        unsigned *window = (unsigned *)(buf + nbits * (cnt % 8) / 8);
        *window |= code << (cnt % 8) * (nbits - 8) % 8;
        ++cnt;

        if (cnt % 8 == 0 || code == 256)
        {
            os.write(buf, nbits);
            fill(buf, buf + sizeof(buf), 0);
        }

        if (code == 256)
            nbits = 9, cnt = 0;

        if (nbits != bitdepth && cnt == 1U << nbits - 1)
            ++nbits, cnt = 0;
    }

    auto dv = div((cnt % 8) * nbits, 8);
    os.write(buf, dv.quot + (dv.rem ? 1 : 0));
    os.flush();
}

int main(int argc, char **argv)
{
    static constexpr unsigned bitdepth = 16;
    istream *is = &cin;
    ostream *os = &cout;
    ifstream ifs;

    if (argc > 1)
        ifs.open(argv[1]), is = &ifs;

    os->put(0x1f);
    os->put(0x9d);
    os->put(bitdepth | 0x80);
    press(codify(*is), *os, bitdepth);
    return 0;
}


