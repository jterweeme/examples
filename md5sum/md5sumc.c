//Usage: ./md5sumc < file

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

static uint32_t _k[64], _r[64] = {
 7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
 5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
 4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
 6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21};

static void calc(uint32_t *h, uint32_t *w)
{
    uint32_t a = h[0], b = h[1], c = h[2], d = h[3];

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
        uint32_t x = a + f + _k[i] + w[g];
        b += x << _r[i] | x >> 32 - _r[i];
        a = temp;
    }

    h[0] += a, h[1] += b, h[2] += c, h[3] += d;
}

int main()
{
    for (unsigned i = 0; i < 64; ++i)
        _k[i] = (uint32_t)(fabs(sin(i + 1)) * (double)(1UL << 32));

    union Buf { char b[64]; uint32_t dw[16]; } buf;
    uint32_t h[] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476 };
    unsigned sz = 0, gc = 0;

    while ((gc = fread(buf.b, 1, 64, stdin)) == 64)
        calc(h, buf.dw), sz += gc;

    sz += gc;
    memset(buf.b + gc, 0, 64 - gc);
    buf.b[gc] = 0x80;

    if (gc >= 56)
        calc(h, buf.dw), memset(buf.b, 0, 64);

    buf.dw[14] = sz << 3, buf.dw[15] = sz >> 29;
    calc(h, buf.dw);

    uint8_t *a = (uint8_t *)h;
    for (unsigned i = 0; i < 16; ++i)
        printf("%02x", a[i]);

    printf("\r\n");
    return 0;
}


