//This is a comment
//I love comments

#include "mystd.h"
#include <fstream>
#include <iostream>
#include <cstdint>
#include <cassert>
#include <cmath>

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

using mystd::ifstream;
using mystd::istream;
using mystd::ostream;
using mystd::cin;
using mystd::cout;
using mystd::cerr;
using mystd::endl;
using std::fabs;
using std::sin;

static constexpr char nibble(uint8_t n)
{ return n <= 9 ? '0' + char(n) : 'a' + char(n - 10); }

struct Hash
{
    uint32_t _h[4];
    uint32_t operator [] (unsigned i) const { return _h[i]; }

    Hash(uint32_t h0, uint32_t h1, uint32_t h2, uint32_t h3)
    { _h[0] = h0, _h[1] = h1, _h[2] = h2, _h[3] = h3; }

    Hash& operator += (const Hash &h)
    { _h[0] += h[0], _h[1] += h[1], _h[2] += h[2], _h[3] += h[3]; return *this; }
};

class Chunk
{
    uint32_t _w[16];
    uint32_t _k[64];
    static const uint32_t _r[64];
public:
    Chunk()
    { for (unsigned i = 0; i < 64; ++i) _k[i] = uint32_t(fabs(sin(i + 1)) * double(1UL << 32)); }

    void fillTail(uint32_t size) { _w[14] = size * 8, _w[15] = size >> 29; }
    Hash calc(const Hash &hash) const;
    void clear() { for (int i = 0; i < 16; ++i) _w[i] = 0; }
    void stopBit(unsigned gc) { ((char *)_w)[gc] = 0x80; }
    auto read(istream &is) { clear(); is.read((char *)_w, 64); return is.gcount(); }
};

const uint32_t Chunk::_r[64] = {
 7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
 5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
 4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
 6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21};

Hash Chunk::calc(const Hash &h) const
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
        uint32_t x = a + f + _k[i] + _w[g];
        b += x << _r[i] | x >> 32 - _r[i];
        a = temp;
    }

    return Hash(a, b, c, d);
}

static Hash stream(istream &is)
{
    Hash hash(0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476);
    Chunk chunk;
    unsigned sz = 0, gc = 0;

    while ((gc = chunk.read(is)) == 64)
        hash += chunk.calc(hash), sz += gc;

    sz += gc;
    chunk.stopBit(gc);

    if (is.gcount() < 56) {
        chunk.fillTail(sz);
    } else if (is.gcount() < 64) {
        hash += chunk.calc(hash);
        chunk.clear();
        chunk.fillTail(sz);
    }

    return hash += chunk.calc(hash);
}

int main(int argc, char **argv)
{
#ifdef WIN32
    _setmode(_fileno(stdin), _O_BINARY);
#endif
    istream *is = &cin;
    ifstream ifs;

    if (argc > 1)
        ifs.open(argv[1]), is = &ifs;

    Hash hash = ::stream(*is);

    uint8_t *a = (uint8_t *)hash._h;
    for (unsigned i = 0; i < 16; ++i)
        cout << nibble(a[i] >> 4) << nibble(a[i] & 0xf);

    cout << endl;
    return 0;
}


