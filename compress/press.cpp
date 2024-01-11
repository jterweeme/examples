#include "generator.h"
#include "mystl.h"
#include <iostream>
#include <cassert>

using mystl::istream;
using std::ostream;
using mystl::cin;
using std::cout;
using std::cerr;
using std::fill;
using std::div;

static Generator<unsigned> codes(istream &is)
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

int main()
{
    ostream *os = &cout;
    os->put(0x1f);
    os->put(0x9d);
    os->put(0x90);
    unsigned cnt = 0, nbits = 9;
    const unsigned bitdepth = 16;
    char buf[20] = {0};
    
    for (auto codes = ::codes(cin); codes;)
    {
        unsigned code = codes();
        unsigned *window = (unsigned *)(buf + nbits * (cnt % 8) / 8);
        *window |= code << (cnt % 8) * (nbits - 8) % 8;
        ++cnt;

        if (cnt % 8 == 0 || code == 256)
        {
            os->write(buf, nbits);
            fill(buf, buf + sizeof(buf), 0);
        }

        if (code == 256)
            nbits = 9, cnt = 0;
        
        if (nbits != bitdepth && cnt == 1U << nbits - 1)
            ++nbits, cnt = 0;
    }

    auto dv = div((cnt % 8) * nbits, 8);
    os->write(buf, dv.quot + (dv.rem ? 1 : 0));
    os->flush();
    return 0;
}


