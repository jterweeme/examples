#include <fstream>
#include <cstring>
#include <string>
#include <iostream>
#include <cstdint>

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

#if __cplusplus >= 201103L
#define CPP11
#endif

#ifdef CPP11
#define CONSTEXPR constexpr
#define NULLPTR nullptr
#else
#define CONSTEXPR const
#define NULLPTR NULL

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned uint32_t;
#endif

class Toolbox
{
public:
    static char bin(uint8_t b);
    static char nibble(uint8_t n);
    static std::string bin8(uint8_t b);
    static std::string hex8(uint8_t b);
    static void hex8(std::ostream &os, uint8_t b);
    static std::string hex16(uint16_t w);
    static std::string hex32(uint32_t dw);
    static std::string hex64(uint64_t dw64);
    static std::string padding(const std::string &s, char c, size_t n);
    static std::wstring strtowstr(const std::string &s);
    static std::string wstrtostr(const std::wstring &ws);
    void hexdump(std::ostream &os, const uint8_t *data, uint32_t len) const;
    static uint32_t swapEndian(uint32_t x);
    static uint32_t be32tohost(uint32_t num);
};

char Toolbox::nibble(uint8_t n)
{
    return n <= 9 ? '0' + char(n) : 'a' + char(n - 10);
}

std::string Toolbox::hex8(uint8_t b)
{
    std::string ret;
    ret += nibble(b >> 4 & 0xf);
    ret += nibble(b >> 0 & 0xf);
    return ret;
}

void Toolbox::hex8(std::ostream &os, uint8_t b)
{
    os.put(nibble(b >> 4 & 0xf));
    os.put(nibble(b >> 0 & 0xf));
}

std::string Toolbox::hex16(uint16_t w)
{
    std::string ret;
    ret += hex8(w >> 8 & 0xff);
    ret += hex8(w >> 0 & 0xff);
    return ret;
}

std::string Toolbox::hex32(uint32_t dw)
{
    std::string ret;
    ret += hex16(dw >> 16 & 0xffff);
    ret += hex16(dw >>  0 & 0xffff);
    return ret;
}

//TODO: IS NIET PORTABLE!!!
uint32_t Toolbox::be32tohost(uint32_t num)
{
    return swapEndian(num);
}

uint32_t Toolbox::swapEndian(uint32_t n)
{
    return (n >> 24 & 0xff) | (n << 8 & 0xff0000) | (n >> 8 & 0xff00) | (n << 24 & 0xff000000);
}

std::wstring Toolbox::strtowstr(const std::string &s)
{
    std::wstring ret;
    size_t len = s.length();

    for (size_t i = 0; i < len; ++i)
        ret.push_back(wchar_t(s.at(i)));

    return ret;
}

std::string Toolbox::wstrtostr(const std::wstring &ws)
{
    std::string ret;
    size_t len = ws.length();

    for (size_t i = 0; i < len; ++i)
        ret.push_back(char(ws.at(i)));

    return ret;
}

class Hash
{
private:
    static CONSTEXPR uint32_t H0 = 0x67452301;
    static CONSTEXPR uint32_t H1 = 0xefcdab89;
    static CONSTEXPR uint32_t H2 = 0x98badcfe;
    static CONSTEXPR uint32_t H3 = 0x10325476;
    uint32_t _h0, _h1, _h2, _h3;
public:
    Hash();
    Hash(uint32_t h0, uint32_t h1, uint32_t h2, uint32_t h3);
    void reset();
    void add(const Hash &h);
    uint32_t h0() const { return _h0; }
    uint32_t h1() const { return _h1; }
    uint32_t h2() const { return _h2; }
    uint32_t h3() const { return _h3; }
    void dump(std::ostream &os) const;
    std::string toString() const;
};

class Chunk
{
private:
    uint32_t _w[16];
    static const uint32_t _k[64];
    static const uint32_t _r[64];
    static uint32_t leftRotate(uint32_t x, uint32_t c) { return x << c | x >> (32 - c); }
    static uint32_t to_uint32(const uint8_t *bytes);
public:
    Hash calc(const Hash &hash);
    void read(const uint8_t *msg);
    void fillTail(uint32_t size);

    void clear()
    { 
        for (int i = 0; i < 16; ++i)
            _w[i] = 0;
    }
};


void Chunk::fillTail(uint32_t size)
{
    _w[14] = size * 8;
    _w[15] = size >> 29;
}

uint32_t Chunk::to_uint32(const uint8_t * const bytes)
{
    return uint32_t(bytes[0])
        | (uint32_t(bytes[1]) << 8)
        | (uint32_t(bytes[2]) << 16)
        | (uint32_t(bytes[3]) << 24);
}

void Chunk::read(const uint8_t *msg)
{
    for (int i = 0; i < 16; ++i)
        _w[i] = to_uint32(msg + i * 4);
}   

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

const uint32_t Chunk::_r[64] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
                      5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
                      4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                      6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

Hash Chunk::calc(const Hash &hash)
{
    uint32_t a, b, c, d, f, g, temp;
    a = hash.h0();
    b = hash.h1();
    c = hash.h2();
    d = hash.h3();

    for (int i = 0; i < 64; ++i)
    {
        if (i < 16)
        {
            f = (b & c) | ((~b) & d);
            g = i;
        }
        else if (i < 32)
        {
            f = (d & b) | ((~d) & c);
            g = (5 * i + 1) % 16;
        }
        else if (i < 48)
        {
            f = b ^ c ^ d;
            g = (3 * i + 5) % 16;
        }
        else
        {
            f = c ^ (b | (~d));
            g = (7 * i) % 16;
        }

        temp = d;
        d = c;
        c = b;
        b = b + leftRotate((a + f + _k[i] + _w[g]), _r[i]);
        a = temp;
    }

    Hash foo(a, b, c, d);
    return foo;
}


class Hasher
{
public:
    static Hash stream(std::istream &is);
    static Hash array(const char *s, size_t size);
};

Hash::Hash() : _h0(H0), _h1(H1), _h2(H2), _h3(H3)
{

}

Hash::Hash(uint32_t h0, uint32_t h1, uint32_t h2, uint32_t h3)
    : _h0(h0), _h1(h1), _h2(h2), _h3(h3)
{
}

void Hash::dump(std::ostream &os) const
{
    Toolbox t;
    (void)t;

    os << t.hex32(t.be32tohost(h0())) <<
          t.hex32(t.be32tohost(h1())) <<
          t.hex32(t.be32tohost(h2())) <<
          t.hex32(t.be32tohost(h3()));
}

std::string Hash::toString() const
{
    Toolbox t;
    std::string ret;
    ret.append(t.hex32(t.be32tohost(h0())));
    ret.append(t.hex32(t.be32tohost(h1())));
    ret.append(t.hex32(t.be32tohost(h2())));
    ret.append(t.hex32(t.be32tohost(h3())));
    return ret;
}

void Hash::add(const Hash &h)
{
    _h0 += h.h0();
    _h1 += h.h1();
    _h2 += h.h2();
    _h3 += h.h3();
}

void Hash::reset()
{
    _h0 = H0;
    _h1 = H1;
    _h2 = H2;
    _h3 = H3;
}

Hash Hasher::stream(std::istream &is)
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

Hash Hasher::array(const char *s, size_t size)
{
    Hash hash;

    for (size_t i = 0; i <= size; i+= 64)
    {
        size_t readSize = 64;

        if (size - i < 64)
            readSize = size - i;

        uint8_t data[64] = {0};
        memcpy(data, s + i, readSize);
        Chunk chunk;

        if (readSize < 56)
        {
            data[readSize] = 0x80;
            chunk.read(data);
            chunk.fillTail(size);
        }
        else if (readSize < 64)
        {
            data[readSize] = 0x80;
            chunk.read(data);
            Hash foo = chunk.calc(hash);
            hash.add(foo);
            chunk.clear();
            chunk.fillTail(size);
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
    std::wcout << lpCmdLine << "\n";
    std::wcout.flush();
    size_t len = wcslen(lpCmdLine);

    if (len < 1)
        return 0;

    std::ifstream ifs;
    ifs.open(lpCmdLine, std::ifstream::in | std::ifstream::binary);

    if (!ifs.good())
    {
        std::cerr << "Cannot open file!\n";
        std::cerr.flush();
    }

    Hash hash = Hasher::stream(ifs);
    hash.dump(std::cout);
    std::cout << "\n";
    std::cout.flush();
    ifs.close();
    return 0;
}
#else
class Options
{
private:
public:
    void parse(int argc, char **argv);
};

void Options::parse(int argc, char **argv)
{

}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
#ifdef WIN32
    _setmode(_fileno(stdin), _O_BINARY);
#endif
    Hash hash = Hasher::stream(std::cin);
#if 0
    std::ifstream ifs;
    ifs.open("d:\\temp\\random.dat", std::ios::binary);
    char *buf = new char[1280256];
    ifs.read(buf, 1280256);
    Hash hash;
    hash = Hasher::array(buf, 1280256);
    delete[] buf;
    std::cout << hash.toString() << "\r\n";
    ifs.seekg(0, std::ios::beg);
    hash = Hasher::stream(ifs);
#endif
    std::cout << hash.toString() << "\r\n";
    std::cout.flush();
    return 0;
}
#endif
