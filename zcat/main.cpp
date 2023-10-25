#include <cstdint>
#include <cstring>
#include <iostream>
#include <fstream>

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

class Decomp
{
private:
    static constexpr uint8_t INIT_BITS = 9, BIT_MASK = 0x1f;
    static constexpr uint16_t CLEAR = 256;
    static constexpr uint32_t HSIZE = 1<<17, BFSZ = 99;

    int _posbits = 0;
    char _inbuf[BFSZ + 64];
    int _insize = 0;
    int _rsize = 0;
    int _inbits = 0;
    int _n_bits = INIT_BITS;
    uint32_t _bitmask;
    std::istream *_is;

    void _dinges();
    uint32_t _code();
    void _reset();
public:
    void decompress(std::istream *is, std::ostream *os);
};

void Decomp::_dinges()
{
    _posbits = (_posbits-1) + (_n_bits * 8 - (_posbits - 1 + _n_bits * 8) % (_n_bits<<3));
}

uint32_t Decomp::_code()
{
    uint32_t ret = *(uint32_t *)(&_inbuf[_posbits >> 3]) >> (_posbits & 0x7);
    ret &= _bitmask;
    _posbits += _n_bits;
    return ret;
}

void Decomp::_reset()
{
    int o = _posbits >> 3;
    int e = o <= _insize ? _insize - o : 0;

    for (int i = 0 ; i < e ; ++i)
        _inbuf[i] = _inbuf[i + o];

    _insize = e;
    _posbits = 0;

    if (_insize < 64)
    {
        _is->read(_inbuf + _insize, BFSZ);
        _rsize = _is->gcount();

        if (_rsize < 0)
            throw "read error";

        _insize += _rsize;
    }

    _inbits = _rsize > 0 ? _insize - _insize % _n_bits << 3 : (_insize << 3) - (_n_bits - 1);
}

void Decomp::decompress(std::istream *is, std::ostream *os)
{
    _is = is;
    uint16_t g_codetab[HSIZE];
    uint8_t htab[HSIZE];

    for (uint16_t i = 0; i < 256; ++i)
        htab[i] = uint8_t(i);

    if (is->get() != 0x1f)
        throw "invalid magic";

    if (is->get() != 0x9d)
        throw "invalid magic";

    uint8_t tmp = is->get();
    bool block_mode = tmp & 0x80 ? true : false;
    int maxbits = tmp & BIT_MASK;

    uint32_t maxcode;
    maxcode = _bitmask = (1<<_n_bits) - 1;
    long oldcode = -1;
    int finchar = 0;
    uint32_t free_ent = block_mode ? 257 : 256;
    _reset();

    while (_rsize > 0)
    {
        _reset();

        if (oldcode == -1)
        {
            uint32_t code = _code();

            if (code >= 256)
                throw "corrupt input";

            os->put(uint8_t(finchar = oldcode = int(code)));
        }

        while (_inbits > _posbits)
        {
            if (free_ent > maxcode)
            {
                _dinges();
                ++_n_bits;
                maxcode = _n_bits == maxbits ? 1 << maxbits : (1 << _n_bits) - 1;
                _bitmask = (1 << _n_bits)-1;
                _reset();
            }

            uint32_t code = _code();

            if (code == CLEAR && block_mode)
            {
                memset(g_codetab, 0, 256);
                free_ent = 256;
                _dinges();
                _n_bits = INIT_BITS;
                maxcode = _bitmask = (1<<_n_bits) - 1;
                _reset();
                continue;
            }

            long incode = code;
            uint8_t *stackp = htab + HSIZE - 1;

            if (code >= free_ent)
            {
                if (code > free_ent)
                    throw "corrupt input";

                *--stackp = uint8_t(finchar), code = oldcode;
            }

            while (code >= 256)
                *--stackp = htab[code], code = g_codetab[code];

            *--stackp = uint8_t(finchar = htab[code]);

            for (int i = 0;;)
            {
                os->write((const char *)(stackp), i);
                stackp += i;
                i = (htab + HSIZE - 1) - stackp;

                if (i <= 0)
                    break;
            }

            code = free_ent;

            if (code < 1 << maxbits)
            {
                g_codetab[code] = uint16_t(oldcode);
                htab[code] = uint8_t(finchar);
                free_ent = code + 1;
            }

            oldcode = incode;
        }
    }
}

int main(int argc, char **argv)
{
#ifdef WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    std::ifstream ifs;
    std::istream *is = &std::cin;

    if (argc == 2)
    {
        ifs.open(argv[1]);
        is = &ifs;
    }
        
    Decomp d;
    d.decompress(is, &std::cout);
    return 0;
}


