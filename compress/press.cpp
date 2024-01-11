#include "mystl.h"
#include <algorithm>
#include <ctype.h>

using mystl::istream;
using mystl::ostream;
using mystl::cin;
using mystl::cout;
using mystl::cerr;
using std::fill;
using std::div;

static int code(istream &is)
{
    int n = 0;
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
                return n;

            flag = false;
            n = 0;
        }
    }

    return -1;
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
    
    for (int code; (code = ::code(cin)) != -1;)
    {
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


