#include <cassert>
#include <cstdint>
#include <algorithm>
#include <numeric>
#include <vector>
#include <iostream>
#include <fstream>

#define HSIZE (1<<17)

class BitStream
{
    std::vector<uint8_t> inbuf;
    uint32_t _posbits = 0;
public:
    BitStream(std::istream &is)
    {
        for (int c; (c = is.get()) != -1;)
            inbuf.push_back(c);
    }

    void sync(uint32_t n)
    {
        _posbits = n;
    }

    int32_t readBits(uint8_t n)
    {
        if ((inbuf.size() << 3) - (n - 1) <= _posbits)
            return -1;
    
        uint8_t *p = inbuf.data() + (_posbits >> 3);
        uint32_t code = uint32_t(p[0]) | uint32_t(p[1]) << 8  | uint32_t(p[2]) << 16;
        code = code >> (_posbits & 0x7);
        code = code & (1 << n) - 1;
        _posbits += n;
        return code;
    }
};

int main(int argc, char **argv)
{
    std::ifstream ifs;
    std::istream *is = &std::cin;

    if (argc > 1)
    {
        ifs.open(argv[1]);
        is = &ifs;
    }

    std::ostream *os = &std::cout;
    assert(is->get() == 0x1f);
    assert(is->get() == 0x9d);
    uint8_t buf1 = is->get();
    const uint8_t maxbits = buf1 & 0x1f;
    const uint8_t block_mode = buf1 & 0x80;
    assert(maxbits <= 16);
    int32_t oldcode = -1;
    uint8_t finchar = 0;
    uint32_t posbits = 0, poosbits = 0;
    int32_t free_ent = block_mode ? 257 : 256;
    uint16_t codetab[HSIZE];
    uint8_t htab[HSIZE];
    uint8_t n_bits = 9;
    std::fill(codetab, codetab + 256, 0);
    std::iota(htab, htab + 256, 0);
    BitStream bis(*is);

    while (true)
    {
        int32_t maxcode = n_bits == maxbits ? 1 << maxbits : (1 << n_bits) - 1;
    
        if (free_ent > maxcode)
            ++n_bits;

        const uint32_t div = (posbits - poosbits) / 16;

        for (uint32_t i = 0; i < div; ++i)
        {
            bis.readBits(16);
            poosbits += 16;
        }

        int32_t code = bis.readBits(n_bits);
        posbits += n_bits;
        poosbits += n_bits;

        if (code == -1)
            break;
    
        if (oldcode == -1)
        {
            assert(code < 256);
            oldcode = code;
            finchar = oldcode;
            os->put(finchar);
            continue;
        }
    
        if (code == 256 && block_mode)
        {
            std::fill(codetab, codetab + 256, 0);
            free_ent = 256;
            posbits = (posbits - 1) + ((n_bits<<3) - (posbits - 1 + (n_bits<<3)) % (n_bits<<3));
            n_bits = 9;
            continue;
        }
    
        uint32_t incode = code;
        uint8_t *stackp = htab + HSIZE - 1;
    
        if (code >= free_ent)   
        {
            assert(code <= free_ent);
            *--stackp = finchar;
            code = oldcode;
        }
    
        while (code >= 256)
        {
            *--stackp = htab[code];
            code = codetab[code];
        }
    
        *--stackp = finchar = htab[code];
        os->write((char *)(stackp), htab + HSIZE - 1 - stackp);
    
        if ((code = free_ent) < (1 << maxbits))
        {
            codetab[code] = (uint16_t)oldcode;
            htab[code] = finchar;
            free_ent = code + 1;
        }
    
        oldcode = incode;
    }

    os->flush();
    ifs.close();
    return 0;
}


