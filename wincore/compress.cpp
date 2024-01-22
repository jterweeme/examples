//This is a comment
//I love comments

#include "generator.h"
#include "mystd.h"
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <cassert>

using mystd::cin;
using mystd::cout;
using mystd::cerr;
using mystd::ifstream;
using mystd::istream;
using mystd::ostream;
using std::unordered_map;
using std::make_pair;
using std::pair;
using mystd::fill;
using std::div;

//(faster than unordered_map)
class Dictionary
{
    static constexpr unsigned HSIZE = 69001;
    uint16_t _codetab[HSIZE];
    uint32_t _htab[HSIZE];
    pair<uint32_t, uint16_t> _found;
    unsigned _hp;
public:
    void clear() { fill(_htab, _htab + HSIZE, 0xffffffff); }
    void store(uint32_t key, unsigned value) { _codetab[_hp] = value, _htab[_hp] = key; }
    Dictionary() { clear(); }
    auto end() const { return nullptr; }
    auto &operator [](unsigned i) { _htab[_hp] = i; return _codetab[_hp]; }

    pair<uint32_t, uint16_t> *find(uint32_t key)
    {
        const unsigned disp = HSIZE - 1 - (_hp = (key >> 16) << 8 ^ key & 0xffff);

        while (_htab[_hp] != 0xffffffff)
        {
            if (_htab[_hp] == key)
                return &(_found = make_pair(key, _codetab[_hp]));

            if (_hp < disp)
                _hp += HSIZE;

            _hp -= disp;
        }

        return end();
    }
};

static Generator<unsigned> codify(istream &is)
{
    //unordered_map<uint32_t, uint16_t> dict;
    Dictionary dict;
    unsigned free_ent = 257;
    unsigned ent = is.get();
    unsigned n_bits = 9;
    unsigned extcode = 513;

    for (int byte; (byte = is.get()) != -1;)
    {
        if (free_ent >= extcode && ent < 257)
        {
            if (++n_bits > 16)
            {
                co_yield 256;
                free_ent = 257;
                dict.clear();
                n_bits = 9;
            }

            extcode = 1 << n_bits;

            if (n_bits < 16)
                ++extcode;
        }

        if (auto search = dict.find(byte << 16 | ent); search != dict.end())
            ent = search->second;
        else
        {
            dict[byte << 16 | ent] = free_ent++;
            co_yield ent;
            ent = byte;
        }
    }

    co_yield ent;
}

static void press(Generator<unsigned> codes, ostream &os, unsigned bitdepth)
{
    unsigned cnt = 0, nbits = 9;
    char buf[20] = {0};

    while (codes)
    {
        unsigned code = codes();
        unsigned *window = (unsigned *)(buf + nbits * (cnt % 8) / 8);
        *window |= code << (cnt % 8) * (nbits - 8) % 8;

        if (++cnt % 8 == 0 || code == 256)
        {
            os.write(buf, nbits);
            fill(buf, buf + sizeof(buf), 0);
        }

        if (code == 256)
            nbits = 9, cnt = 0;
        
        if (nbits != bitdepth && cnt == 1U << nbits - 1)
            ++nbits, cnt = 0;
    }

    auto dv = div((cnt % 8) * nbits, 8);
    os.write(buf, dv.quot + (dv.rem ? 1 : 0));
    os.flush();
}

int main(int argc, char **argv)
{
    static constexpr unsigned bitdepth = 16;
    istream *is = &cin;
    ostream *os = &cout;
    ifstream ifs;

    if (argc > 1)
        ifs.open(argv[1]), is = &ifs;

    os->put(0x1f);
    os->put(0x9d);
    os->put(bitdepth | 0x80);
    press(codify(*is), *os, bitdepth);
    return 0;
}


