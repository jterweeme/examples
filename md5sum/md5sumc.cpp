#include <fstream>
#include <cstring>
#include <iostream>
#include <cstdint>
#include <cassert>

using std::istream;
using std::fill;
using std::cin;

const uint32_t _k[64] = {
0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee ,
0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501 ,
0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be ,
0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821 ,
0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa ,
0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8 ,
0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed ,
0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a ,
0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c ,
0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70 ,
0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05 ,
0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665 ,
0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039 ,
0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1 ,
0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1 ,
0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391 };

const uint32_t _r[64] = { 7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
                          5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
                          4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
                          6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21};

union Buf
{
    uint8_t b[64];
    uint32_t w[16];
};

Buf buf;
uint32_t h[4];

void calc()
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
        uint32_t x = a + f + _k[i] + buf.w[g];
        b += x << _r[i] | x >> 32 - _r[i];
        a = temp;
    }

    h[0] += a, h[1] += b, h[2] += c, h[3] += d;
}

static void stream(istream &is)
{
    h[0] = 0x67452301, h[1] = 0xefcdab89, h[2] = 0x98badcfe, h[3] = 0x10325476;

    for (unsigned i = 0; is; ++i)
    {
        fill(buf.b, buf.b + 64, 0);
        is.read((char *)buf.b, 64);

        if (is.gcount() < 64)
            buf.b[is.gcount()] = 0x80;

        unsigned sz = i * 64 + is.gcount();

        if (is.gcount() < 56)
        {
            buf.w[14] = sz * 8;
            buf.w[15] = sz >> 29;
        }
        else if (is.gcount() < 64)
        {
            calc();
            fill(buf.b, buf.b + 64, 0);
            buf.w[14] = sz * 8;
            buf.w[15] = sz >> 29;
        }

        calc();
    }

    uint8_t *a = (uint8_t *)h;
    for (unsigned i = 0; i < 16; ++i)
        printf("%02x", a[i]);

    printf("\r\n");
    fflush(stdout);
}

int main(int argc, char **argv)
{
    stream(cin);
    return 0;
}


