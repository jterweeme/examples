#include <cstring>
#include <iostream>
#include <algorithm>

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

static constexpr uint8_t BLOCK_MODE = 0x80, BITS = 16, INIT_BITS = 9, BIT_MASK = 0x1f;
static constexpr uint32_t HSIZE = 1<<17;
static constexpr uint16_t CLEAR = 256;

class Buffer
{
public:
    char _inbuf[BUFSIZ + 64];
    int _posbits = 0;
    int _insize;
    int _rsize;
    int _inbits;
    int _n_bits = INIT_BITS;
public:
    void read();
    uint32_t code(uint32_t mask);
    bool has();
    void dinges();
    void reset();
};

void Buffer::read()
{
    std::cin.read(_inbuf, BUFSIZ);
}

void Buffer::dinges()
{
    _posbits = (_posbits-1) + (_n_bits * 8 - (_posbits - 1 + _n_bits * 8) % (_n_bits<<3));
}

uint32_t Buffer::code(uint32_t mask)
{
    long ret = *(long *)(&_inbuf[_posbits >> 3]) >> (_posbits & 0x7);
    ret &= mask;
    _posbits += _n_bits;
    return ret;
}

bool Buffer::has()
{
    return false;
}

void Buffer::reset()
{
    int o = _posbits >> 3;
    int e = o <= _insize ? _insize - o : 0;

    for (int i = 0 ; i < e ; ++i)
        _inbuf[i] = _inbuf[i+o];

    _insize = e;
    _posbits = 0;

    if (_insize < sizeof(_inbuf) - BUFSIZ)
    {
        std::cin.read(_inbuf + _insize, BUFSIZ);
        _rsize = std::cin.gcount();

        if (_rsize < 0)
            throw "read error";

        _insize += _rsize;
    }

    _inbits = _rsize > 0 ? _insize - _insize % _n_bits << 3 : (_insize<<3)-(_n_bits-1);
}

class Decomp
{
private:
    std::istream *_is;
    std::ostream *_os;
public:
    Decomp(std::istream *is, std::ostream *os);
    int decompress(int maxbits, int block_mode);
};

Decomp::Decomp(std::istream *is, std::ostream *os) : _is(is), _os(os)
{
}

int Decomp::decompress(int maxbits, int block_mode)
{
    Buffer buf;
    uint8_t htab[HSIZE];
    uint16_t g_codetab[HSIZE];
    _is->read(buf._inbuf, BUFSIZ);
    buf._rsize = buf._insize = _is->gcount();
    long bytes_in = buf._insize;

    uint32_t bitmask, maxcode;
    maxcode = bitmask = (1<<buf._n_bits) - 1;
    long oldcode = -1;
    int finchar = 0;
    uint32_t free_ent = block_mode ? 257 : 256;

    for (uint16_t i = 0; i < 256; ++i)
        htab[i] = uint8_t(i);
resetbuf:
    buf.reset();

    while (buf._inbits > buf._posbits)
    {
        if (free_ent > maxcode)
        {
            buf.dinges();
            ++buf._n_bits;
            maxcode = buf._n_bits == maxbits ? 1 << maxbits : (1 << buf._n_bits) - 1;
            bitmask = (1<<buf._n_bits)-1;
            buf.reset();
            continue;
        }

        uint32_t code = buf.code(bitmask);

        if (oldcode == -1)
        {
            if (code >= 256)
                throw "corrupt input";

            _os->put(uint8_t(finchar = int(oldcode = code)));
            continue;
        }

        if (code == CLEAR && block_mode)
        {
            memset(g_codetab, 0, 256);
            free_ent = 256;
            buf.dinges();
            buf._n_bits = INIT_BITS;
            maxcode = (1<<buf._n_bits) - 1;
            bitmask = (1<<buf._n_bits) - 1;
            buf.reset();
            continue;
        }

        long incode = code;
        uint8_t *stackp = htab + HSIZE - 1;

        if (code >= free_ent)
        {
            if (code > free_ent)
                throw "corrupt input";

            *--stackp = uint8_t(finchar);
            code = oldcode;
        }

        while (code >= 256)
        {
            *--stackp = htab[code];
            code = g_codetab[code];
        }

        *--stackp = uint8_t(finchar = htab[code]);

        int i = 0;

        do
        {
            _os->write((const char *)(stackp), i);
            stackp += i;
            i = (htab + HSIZE - 1) - stackp;
        }
        while (i > 0);

        if ((code = free_ent) < (1 << maxbits))
        {
            g_codetab[code] = uint16_t(oldcode);
            htab[code] = uint8_t(finchar);
            free_ent = code + 1;
        }

        oldcode = incode;
    }

    bytes_in += buf._rsize;

    if (buf._rsize > 0)
        goto resetbuf;

    return 0;
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

#ifdef WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    if (std::cin.get() != 0x1f)
        throw "invalid magic";

    if (std::cin.get() != 0x9d)
        throw "invalid magic";

    uint8_t tmp = std::cin.get();
    Decomp d(&std::cin, &std::cout);
    return d.decompress(tmp & BIT_MASK, tmp & BLOCK_MODE);
}


