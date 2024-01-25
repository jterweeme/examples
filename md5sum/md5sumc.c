//Usage: ./md5sumc < file

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

static uint32_t _w[16], _k[64], _h[4];

static uint32_t _r[64] = {
 7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
 5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
 4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
 6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21};

static void calc()
{
    uint32_t a = _h[0], b = _h[1], c = _h[2], d = _h[3];

    for (unsigned i = 0, f, g; i < 64; ++i)
    {
        if (i < 16)
            f = b & c | ~b & d, g = i;
        else if (i < 32)
            f = d & b | ~d & c, g = (5 * i + 1) % 16;
        else if (i < 48)
            f = b ^ c ^ d, g = (3 * i + 5) % 16;
        else
            f = c ^ (b | ~d), g = (7 * i) % 16;
        
        uint32_t temp = d;
        d = c;
        c = b;
        uint32_t x = a + f + _k[i] + _w[g];
        b += x << _r[i] | x >> 32 - _r[i];
        a = temp;
    }

    _h[0] += a, _h[1] += b, _h[2] += c, _h[3] += d;
}

int main()
{
    for (unsigned i = 0; i < 64; ++i)
        _k[i] = (uint32_t)(fabs(sin(i + 1)) * (double)(1UL << 32));

    _h[0] = 0x67452301, _h[1] = 0xefcdab89, _h[2] = 0x98badcfe, _h[3] = 0x10325476;
    unsigned sz = 0, gc = 0;

    while ((gc = fread((char *)_w, 1, 64, stdin)) == 64)
        calc(), sz += gc;

    memset(((char *)_w) + gc, 0, 64 - gc);
    sz += gc;
    ((char *)_w)[gc] = 0x80;

    if (gc < 56) {
        _w[14] = sz * 8, _w[15] = sz >> 29;
    } else if (gc < 64) {
        calc();
        memset(_w, 0, 64);
        _w[14] = sz * 8, _w[15] = sz >> 29;
    }

    calc();

    uint8_t *a = (uint8_t *)_h;
    for (unsigned i = 0; i < 16; ++i)
        printf("%02x", a[i]);

    printf("\r\n");
    return 0;
}


