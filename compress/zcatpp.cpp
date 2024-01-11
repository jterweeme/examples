//This is a comment
//I love comments

//zcatpp (zcat c++)

#include "generator.h"
#include "mystl.h"
#include <cassert>
#include <vector>

using std::vector;
using mystl::ostream;
using mystl::istream;
using mystl::ifstream;
using mystl::cin;
using mystl::cout;
using mystl::cerr;

class ByteStack
{
    vector<char> _stack;
public:
    void push(char c) { _stack.push_back(c); }
    char top() const { return _stack.back(); }
    void pop_all(ostream &os) { for (; _stack.size(); _stack.pop_back()) os.put(top()); }
};

class Dictionary
{
    unsigned _cap;
    uint16_t *_codes;
    char *_bytes;
    unsigned _pos = 0;
public:
    Dictionary(unsigned cap)
      : _cap(cap), _codes(new uint16_t[cap - 256]), _bytes(new char[cap - 256]) { }

    void lookup(ByteStack &s, uint16_t code) const
    {
        for (; code >= 256U; code = _codes[code - 256])
            s.push(_bytes[code - 256]);

        s.push(code);
    }

    void store(unsigned code, char c)
    {
        if (_pos + 256 < _cap)
            _codes[_pos] = code, _bytes[_pos] = c, ++_pos;
    }

    ~Dictionary() { delete[] _codes; delete[] _bytes; }
    auto size() const { return _pos + 256; }
    void clear() { _pos = 0; }
};

static Generator<unsigned>
codes(istream &is, unsigned bitdepth)
{
    char buf[20];
start_block:
    for (unsigned nbits = 9; nbits <= bitdepth; ++nbits)
    {
        for (unsigned i = 0; i < 1U << nbits - 1 || nbits == bitdepth;)
        {
            is.read(buf, nbits);
            unsigned ncodes = is.gcount() * 8 / nbits;

            if (ncodes == 0)
                co_return;
            
            for (unsigned bits = 0, j = 0; ncodes--; bits += nbits, ++i, ++j)
            {
                unsigned *window = (unsigned *)(buf + bits / 8);
                unsigned code = *window >> j * (nbits - 8) % 8 & (1 << nbits) - 1;
                co_yield code;

                if (code == 256)
                    goto start_block;
            }
        }
    }
}

static void
lzw(unsigned dictcap, ostream &os, Generator<unsigned> codes)
{
    Dictionary dict(dictcap);
    ByteStack stack;
    unsigned oldcode = 0;
    char finchar = 0;

    while (codes)
    {
        unsigned newcode, c;
        newcode = c = codes();
        assert(c <= dict.size());
        
        if (c == 256)
        {
            dict.clear();
            continue;
        }

        if (c == dict.size())
            stack.push(finchar), c = oldcode;

        dict.lookup(stack, c);
        dict.store(oldcode, finchar = stack.top());
        oldcode = newcode;
        stack.pop_all(os);
    }
}

int
main(int argc, char **argv)
{
    istream *is = &cin;
    ostream * const os = &cout;
    ifstream ifs;

    if (argc > 1)
        ifs.open(argv[1]), is = &ifs;

    assert(is->get() == 0x1f && is->get() == 0x9d); //magic
    int c = is->get();
    assert(c >= 0 && c & 0x80);   //block mode bit is hardcoded in ncompress
    const unsigned bitdepth = c & 0x7f;
    ::lzw(1 << bitdepth, *os, codes(*is, bitdepth));
    os->flush();
    ifs.close();
    return 0;
}


