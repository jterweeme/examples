#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <numeric>

#define HSIZE (1<<17)

typedef int32_t code_int;
typedef uint32_t count_int;
typedef uint16_t count_short;
typedef uint32_t cmp_code_int;
typedef uint8_t char_type;

class BitInputStream
{   
    std::istream &_input;
    uint8_t _currentByte = 0;
    uint8_t _nBitsRemaining = 0;
public:
    BitInputStream(std::istream &in) : _input(in) { }
    uint8_t getBitPosition() const { return (8 - _nBitsRemaining) % 8; }
    uint8_t readBit();
    void align() { while (getBitPosition() != 0) readBit(); }
    uint32_t readUint(uint32_t numBits);
    ssize_t read(uint8_t *buf, size_t n);
};

ssize_t BitInputStream::read(uint8_t *buf, size_t n)
{
    for (size_t i = 0; i < n; ++i)
        buf[i] = readUint(8) & 0xff;

    return -1;
}

//read single bit
uint8_t BitInputStream::readBit()
{
    if (_nBitsRemaining == 0)
    {
        _currentByte = _input.get();
        _nBitsRemaining = 8;
    }
    --_nBitsRemaining;
    return (_currentByte >> 7 - _nBitsRemaining) & 1;
}

//read multiple bits
uint32_t BitInputStream::readUint(uint32_t numBits)
{
    uint32_t result = 0;
    for (uint32_t i = 0; i < numBits; ++i)
    {
        uint32_t c = readBit();
        result |= c << i;
    }
    return result;
}

int main()
{
    BitInputStream bis(std::cin);
    uint8_t inbuf[BUFSIZ + 64];  //Input buffer
    int rsize = read(0, inbuf, BUFSIZ);
    int insize = rsize;
    assert(insize >= 3);
    assert(inbuf[0] == 0x1f);
    assert(inbuf[1] == 0x9d);
    const uint8_t maxbits = inbuf[2] & 0x1f;
    const uint8_t block_mode = inbuf[2] & 0x80;
    assert(maxbits <= 16);
    const uint32_t maxmaxcode = 1 << maxbits;
    uint8_t n_bits = 9;
    uint32_t bitmask = (1 << n_bits) - 1;
    uint32_t maxcode = n_bits == maxbits ? maxmaxcode : (1 << n_bits) - 1;
    int32_t oldcode = -1;
    uint8_t finchar = 0;
    uint32_t posbits = 3<<3;
    uint32_t free_ent = block_mode ? 257 : 256;
    uint16_t codetab[HSIZE] = {0};
    uint8_t htab[HSIZE];
    std::iota(htab, htab + 256, 0);
resetbuf:
    int o = posbits >> 3;
    int e = o <= insize ? insize - o : 0;

    for (int i = 0 ; i < e ; ++i)
        inbuf[i] = inbuf[i + o];

    insize = e;
    posbits = 0;

    if (insize < int(sizeof(inbuf) - BUFSIZ))
    {
        rsize = read(0, inbuf + insize, BUFSIZ);
        assert(rsize >= 0);
        insize += rsize;
    }

    uint32_t inbits = rsize > 0 ? insize - insize % n_bits << 3 : (insize << 3) - (n_bits - 1);

    while (inbits > posbits)
    {
        if (free_ent > maxcode)
        {
            posbits = posbits - 1 + ((n_bits<<3) - (posbits - 1 + (n_bits<<3)) % (n_bits<<3));
            ++n_bits;
            maxcode = n_bits == maxbits ? maxmaxcode : (1<<n_bits) - 1;
            bitmask = (1 << n_bits) - 1;
            goto resetbuf;
        }

        char_type *p = inbuf + (posbits >> 3);

        uint32_t code = ((uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16)
                            >> (posbits & 0x7)) & bitmask;

        posbits += n_bits;

        if (oldcode == -1)
        {
            assert(code < 256);
            finchar = oldcode = code;
            std::cout.put(finchar);
            continue;
        }

        //clear
        if (code == 256 && block_mode)
        {
            memset(codetab, 0, 256);
            free_ent = 256;
            posbits = (posbits - 1) + ((n_bits<<3) - (posbits - 1 + (n_bits<<3)) % (n_bits<<3));
            n_bits = 9;
            bitmask = (1 << n_bits) - 1;
            maxcode = n_bits == maxbits ? maxmaxcode : (1 << n_bits) - 1;
            goto resetbuf;
        }

        code_int incode = code;
        //char_type *stackp = htab + HSIZE * 4 - 4;
        char_type *stackp = htab + HSIZE - 1;

        //Special case for KwKwK string.
        if (code >= free_ent)   
        {
            assert(code <= free_ent);
            *--stackp = finchar;
            code = oldcode;
        }

        while (code >= 256)
        {
            //Generate output characters in reverse order
            *--stackp = htab[code];
            code = codetab[code];
        }

        *--stackp = finchar = htab[code];

        //And put them out in forward order
        int i = htab + HSIZE - 1 - stackp;
        std::cout.write((char *)(stackp), i);

        //Generate the new entry.
        if ((code = free_ent) < maxmaxcode) 
        {
            codetab[code] = uint16_t(oldcode);
            htab[code] = finchar;
            free_ent = code + 1;
        }

        //remember previous code
        oldcode = incode;
    }

    if (rsize > 0)
        goto resetbuf;

    std::cout.flush();
    return 0;
}


