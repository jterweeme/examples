#include <fstream>
#include <cstring>
#include <iostream>
#include <cstdint>
#include <cassert>

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

#if __cplusplus >= 201103L
#define CPP11
#endif

#ifdef CPP11
#define CONSTEXPR constexpr
#else
#define CONSTEXPR const
#endif

using std::ifstream;
using std::istream;
using std::ostream;
using std::string;
using std::cin;
using std::cout;
using std::cerr;
using std::wcout;

class Toolbox
{
public:
    static char nibble(uint8_t n)
    { return n <= 9 ? '0' + char(n) : 'a' + char(n - 10); }

    static void hex32(unsigned dw, ostream &os)
    { for (unsigned i = 0; i <= 28; i += 4) os.put(nibble(dw >> 28 - i & 0xf)); }

    static uint32_t swapEndian(uint32_t n)
    { return n >> 24 & 0xff | n << 8 & 0xff0000 | n >> 8 & 0xff00 | n << 24 & 0xff000000; }

    static uint32_t be32tohost(uint32_t n)
    { return swapEndian(n); } //NIET PORTABLE!
};

class Hash
{
    uint32_t _h[4];
public:
    Hash() { _h[0] = 0x67452301, _h[1] = 0xefcdab89, _h[2] = 0x98badcfe, _h[3] = 0x10325476; }
    uint32_t h(unsigned i) const { return _h[i]; }

    Hash(uint32_t h0, uint32_t h1, uint32_t h2, uint32_t h3)
    { _h[0] = h0, _h[1] = h1, _h[2] = h2, _h[3] = h3; }

    void dump(ostream &os) const
    { for (unsigned i = 0; i < 4; ++i) Toolbox::hex32(Toolbox::be32tohost(h(i)), os); }

    void add(const Hash &h)
    { _h[0] += h.h(0), _h[1] += h.h(1), _h[2] += h.h(2), _h[3] += h.h(3); }
};

class Chunk
{
    uint32_t _w[16];
    static const uint32_t _k[64];
    static const uint32_t _r[64];
public:
    Hash calc(const Hash &hash);
    void fillTail(uint32_t size) { _w[14] = size * 8, _w[15] = size >> 29; }
    void clear() { for (int i = 0; i < 16; ++i) _w[i] = 0; }

    void read(const uint8_t *msg)
    { for (int i = 0; i < 16; ++i) _w[i] = *(uint32_t *)(msg + i * 4); }
};

const uint32_t Chunk::_k[64] = {
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

const uint32_t Chunk::_r[64] = { 7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
                                 5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
                                 4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
                                 6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21};

Hash Chunk::calc(const Hash &h)
{
    uint32_t a = h.h(0), b = h.h(1), c = h.h(2), d = h.h(3);

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

    Hash foo(a, b, c, d);
    return foo;
}

static Hash stream(istream &is)
{
    Hash hash;

    for (unsigned i = 0; is; ++i)
    {
        uint8_t data[64] = {0};
        is.read((char *)data, 64);
        Chunk chunk;

        if (is.gcount() < 56)
        {
            data[is.gcount()] = 0x80;
            chunk.read(data);
            chunk.fillTail(i * 64 + is.gcount());
        }
        else if (is.gcount() < 64)
        {
            data[is.gcount()] = 0x80;
            chunk.read(data);
            Hash foo = chunk.calc(hash);
            hash.add(foo);
            chunk.clear();
            chunk.fillTail(i * 64 + is.gcount());
        }
        else
        {
            chunk.read(data);
        }

        Hash foo = chunk.calc(hash);
        hash.add(foo);
    }

    return hash;
}

#ifdef WINCE
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nCmdShow)
{
    wcout << lpCmdLine << "\n";
    wcout.flush();
    size_t len = wcslen(lpCmdLine);

    if (len < 1)
        return 0;

    ifstream ifs;
    ifs.open(lpCmdLine, ifstream::in | ifstream::binary);

    if (!ifs.good())
    {
        cerr << "Cannot open file!\n";
        cerr.flush();
    }

    Hash hash = ::stream(ifs);
    hash.dump(cout);
    cout << "\n";
    cout.flush();
    ifs.close();
    return 0;
}
#else
int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
#ifdef WIN32
    _setmode(_fileno(stdin), _O_BINARY);
#endif
    Hash hash = ::stream(cin);
    hash.dump(cout);
    cout << "\r\n";
    cout.flush();
    return 0;
}
#endif
