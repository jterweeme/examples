//Usage: ./extractc archive.Z | ./lzw > archive

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
    std::vector<char> _stack;
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

    void lookup(ByteStack &s, uint16_t code)
    {
        for (; code >= 256U; code = _codes[code - 256])
            s.push(_bytes[code - 256]);

        s.push(code);
    }

    void store(unsigned code, uint8_t c)
    {
        if (_pos + 256 < _cap)
            _codes[_pos] = code, _bytes[_pos] = c, ++_pos;
    }

    ~Dictionary() { delete[] _codes; delete[] _bytes; }
    auto size() const { return _pos + 256; }
    void clear() { _pos = 0; }
};

static Generator<unsigned>
codes(istream &is)
{
    unsigned n = 0;
    bool flag = false;

    for (int c; (c = is.get()) != -1;)
    {
        if (isdigit(c))
        {
            flag = true;
            n = n * 10 + c - '0';
        }
        else
        {
            if (flag)
                co_yield n;

            flag = false;
            n = 0;
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

int main(int argc, char **argv)
{
    istream *is = &cin;
    ostream *os = &cout;
    ifstream ifs;
    
    if (argc > 1)
        ifs.open(argv[1]), is = &ifs;

    ::lzw(1 << 16, *os, ::codes(*is));
    os->flush();
    ifs.close();
    return 0;
}


