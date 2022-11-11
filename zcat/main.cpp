#include <cstring>
#include <iostream>
#include <vector>
#include <algorithm>

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

static constexpr uint8_t BLOCK_MODE = 0x80, BITS = 16, INIT_BITS = 9, BIT_MASK = 0x1f;
static constexpr uint32_t HSIZE = 1<<17;
static constexpr uint16_t CLEAR = 256;

class Decomp
{
private:
    std::istream *_is;
    std::ostream *_os;
    int _posbits = 0;
    uint8_t htab[HSIZE];
    uint16_t g_codetab[HSIZE];
    uint32_t _code(uint32_t mask);
    void reset();
    //std::vector<char> _inbuf;
    char _inbuf[BUFSIZ + 64];
    int _insize = 0;
    int _rsize = 0;
    int _inbits = 0;
    int _n_bits = INIT_BITS;
    void dinges();
public:
    Decomp(std::istream *is, std::ostream *os);
    void read();
    int decompress(int maxbits, int block_mode);
};

void Decomp::dinges()
{
    _posbits = (_posbits-1) + (_n_bits * 8 - (_posbits - 1 + _n_bits * 8) % (_n_bits<<3));
}

uint32_t Decomp::_code(uint32_t mask)
{
    uint32_t ret = *(uint32_t *)(&_inbuf[_posbits >> 3]) >> (_posbits & 0x7);
    ret &= mask;
    _posbits += _n_bits;
    return ret;
}

void Decomp::reset()
{
    int o = _posbits >> 3;
    int e = o <= _insize ? _insize - o : 0;

    for (int i = 0 ; i < e ; ++i)
        _inbuf[i] = _inbuf[i+o];

    _insize = e;
    _posbits = 0;

    if (_insize < 64)
    {
        std::cin.read(_inbuf + _insize, BUFSIZ);
        _rsize = std::cin.gcount();

        if (_rsize < 0)
            throw "read error";

        _insize += _rsize;
    }

    _inbits = _rsize > 0 ? _insize - _insize % _n_bits << 3 : (_insize<<3)-(_n_bits-1);
}

Decomp::Decomp(std::istream *is, std::ostream *os) : _is(is), _os(os)
{
}

void Decomp::read()
{
}

int Decomp::decompress(int maxbits, int block_mode)
{
    _is->read(_inbuf, BUFSIZ);
    _rsize = _insize = _is->gcount();
    uint32_t bitmask, maxcode;
    maxcode = bitmask = (1<<_n_bits) - 1;
    long oldcode = -1;
    int finchar = 0;
    uint32_t free_ent = block_mode ? 257 : 256;

    for (uint16_t i = 0; i < 256; ++i)
        htab[i] = uint8_t(i);
resetbuf:
    reset();

    if (oldcode == -1)
    {
        uint32_t code = _code(bitmask);

        if (code >= 256)
            throw "corrupt input";

        _os->put(uint8_t(finchar = int(oldcode = code)));
    }   

    while (_inbits > _posbits)
    {
        if (free_ent > maxcode)
        {
            dinges();
            ++_n_bits;
            maxcode = _n_bits == maxbits ? 1 << maxbits : (1 << _n_bits) - 1;
            bitmask = (1 << _n_bits)-1;
            reset();
        }

        uint32_t code = _code(bitmask);

        if (code == CLEAR && block_mode)
        {
            memset(g_codetab, 0, 256);
            free_ent = 256;
            dinges();
            _n_bits = INIT_BITS;
            maxcode = (1<<_n_bits) - 1;
            bitmask = (1<<_n_bits) - 1;
            reset();
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

    if (_rsize > 0)
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
    d.read();
    return d.decompress(tmp & BIT_MASK, tmp & BLOCK_MODE);
}


