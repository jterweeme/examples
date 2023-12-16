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
#include <vector>

class BitOutputStream
{
    std::ostream &_os;
    uint32_t _window = 0;
    uint32_t _bits = 0;
public:
    uint64_t cnt = 0;
    BitOutputStream(std::ostream &os) : _os(os) { }

    void write(uint16_t code, uint8_t n_bits)
    {
        _window |= code << _bits, cnt += n_bits, _bits += n_bits;
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

union Fcode
{
    long code = 0;
    struct
    {
        uint8_t c;
        uint16_t ent;
    } e;
};

static constexpr int IBUFSIZ = BUFSIZ;

class InBuf
{
    uint8_t inbuf[IBUFSIZ + 64];
    int bytes_read;
    int _rpos = -1;
public:
    int rpos() const { return _rpos; }
    uint8_t next() { return inbuf[_rpos++]; }
    bool hasleft() { return _rpos < bytes_read; }

    bool read()
    {
        _rpos = 0;
        std::cin.read((char *)inbuf, IBUFSIZ);
        bytes_read = std::cin.gcount();
        return bytes_read <= 0 ? false : true;
    }
};

static bool vlag(bool &f)
{
    bool ret = f;
    f = false;
    return ret;
}

int main(int argc, char **argv)
{
    static constexpr long CHECK_GAP = 10000;
    static constexpr uint32_t HSIZE = 69001;
    int64_t htab[HSIZE];
    uint16_t codetab[HSIZE];
    long checkpoint = CHECK_GAP;
    bool stcode = true;
    uint32_t free_ent = 257;

    BitOutputStream bos(std::cout);
    bos.write(0x9d1f, 16);  //magic
    bos.write(16, 5);       //max. 16 bits (hardcoded)
    bos.write(0, 2);        //reserved
    bos.write(1, 1);        //block mode
    bos.cnt = 0;
    std::fill(htab, htab + HSIZE, -1);
    Fcode fcode;
    fcode.e.ent = std::cin.get();
    long bytes_in = 1;
    InBuf inbuf;
    int n_bits = 9, ratio = 0;
    uint32_t extcode = (1 << n_bits) + 1;
    bool rflag = inbuf.read();

    for (int rlop = 0; rflag;)
    {
        if (inbuf.hasleft() == false)
        {
            rflag = inbuf.read();
            rlop = 0;
            continue;
        }

        if (free_ent >= extcode && fcode.e.ent < 257)
        {
            if (n_bits < 16)
                ++n_bits, extcode = n_bits < 16 ? (1 << n_bits) + 1 : 1 << 16;
            else
                stcode = false;
        }

        if (!stcode && bytes_in >= checkpoint && fcode.e.ent < 257)
        {
            long rat;
            checkpoint = bytes_in + CHECK_GAP;

            if (bytes_in > 0x007fffff)
                //shift will overflow
                rat = bos.cnt >> 11 == 0 ? 0x7fffffff : bytes_in / (bos.cnt >> 11);
            else
                //8 fractional bits
                rat = (bytes_in << 8) / (bos.cnt >> 3);

            if (rat >= ratio)
                ratio = int(rat);
            else
            {
                ratio = 0;
                std::fill(htab, htab + HSIZE, -1);
                bos.write(256, n_bits);

                for (uint8_t nb3 = n_bits << 3; (bos.cnt - 1U + nb3) % nb3 != nb3 - 1U;)
                    bos.write(0, 16);

                n_bits = 9, stcode = true, free_ent = 257;
                extcode = n_bits < 16 ? (1 << n_bits) + 1 : 1 << n_bits;
            }
        }

        ++rlop, ++bytes_in;
        bool flag = false;

        while (inbuf.rpos() < rlop || vlag(flag))
        {
            fcode.e.c = inbuf.next();
            long fc = fcode.code;
            long hp = long(fcode.e.c) <<  8 ^ long(fcode.e.ent);
    
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
                    flag = true;
                    break;
                }
            }

            if (vlag(flag))
                continue;

            bos.write(fcode.e.ent, n_bits);
            fc = fcode.code;
            fcode.e.ent = fcode.e.c;

            if (stcode)
                codetab[hp] = uint16_t(free_ent++), htab[hp] = fc;
        }

        if (fcode.e.ent >= 257 && inbuf.hasleft())
        {
            flag = true;
            continue;
        }

        if (inbuf.rpos() > rlop)
        {
            bytes_in += inbuf.rpos() - rlop;
            rlop = inbuf.rpos();
        }
    }

    bos.write(fcode.e.ent, n_bits);
    bos.flush();
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


