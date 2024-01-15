//This is a comment
//I love comments

#include "generator.h"
#include "mystl.h"
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <cassert>

using mystl::cin;
using mystl::cout;
using mystl::cerr;
using mystl::ifstream;
using mystl::istream;
using mystl::ostream;
using std::unordered_map;
using std::make_pair;
using std::pair;
using std::fill;
using std::div;

//(faster than unordered_map)
class Dictionary
{
    static constexpr unsigned HSIZE = 69001;
    uint16_t codetab[HSIZE];
    uint32_t htab[HSIZE];
    pair<uint32_t, uint16_t> _found;
    unsigned _hp;
public:
    void clear() { fill(htab, htab + HSIZE, 0xffffffff); }
    void store(uint32_t key, unsigned value) { codetab[_hp] = value, htab[_hp] = key; }
    Dictionary() { clear(); }
    auto end() const { return nullptr; }

    auto &operator [](unsigned i)
    {
        htab[_hp] = i;
        return codetab[_hp];
    }

    pair<uint32_t, uint16_t> *find(uint32_t key)
    {
        unsigned c = key >> 16;

        for (unsigned disp = HSIZE - (_hp = c << 8 ^ key & 0xffff) - 1; htab[_hp] != 0xffffffff;)
        {
            if (htab[_hp] == key)
                return &(_found = make_pair(key, codetab[_hp]));

            _hp = _hp < disp ? _hp + HSIZE - disp : _hp - disp;
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


