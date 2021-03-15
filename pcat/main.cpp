#include <unistd.h>
#include <stdint.h>
#include <stdexcept>
#include <iostream>
#include <iostream>

static int decode(const char *xeof, const short intnodes[25], char **tree, long origsize, short infile, short inleft, char *inbuff, char *inp)
{
    char g_outbuff[BUFSIZ];
    char *g_outp = &g_outbuff[0];
    uint32_t lev = 1;
    int i = 0;
#if 0
    std::cerr << inleft << "\r\n";
    std::cerr.flush();
#endif

    while (true)
    {
        if (inleft <= 0)
        {
            inleft = read(infile, inp = &inbuff[0], BUFSIZ);

            if (inleft < 0)
                throw std::runtime_error(".z: read error");
        }

        if (--inleft < 0)
        {
            if (origsize == 0)
                return 1;

            throw std::runtime_error(".z: unpacking error");
        }

        int c = *inp++;
        int bitsleft = 8;

        while (--bitsleft >= 0)
        {
            i *= 2;

            if (c & 0200)
                i++;

            c <<= 1;
            int j = i - intnodes[lev];

            if (j < 0)
            {
                ++lev;
                continue;
            }

            const char *p = &tree[lev][j];

            if (p == xeof)
            {
                c = g_outp - &g_outbuff[0];

                if (write(1, &g_outbuff[0], c) != c)
                    throw std::runtime_error(": write error");

                origsize -= c;

                if (origsize != 0)
                    throw std::runtime_error(".z: unpacking error");

                return (1);
            }
            *g_outp++ = *p;

            if (g_outp == &g_outbuff[BUFSIZ])
            {
                if (write(1, g_outp = &g_outbuff[0], BUFSIZ) != BUFSIZ)
                    throw std::runtime_error(": write error");

                origsize -= BUFSIZ;
            }

            lev = 1;
            i = 0;
        }
    }
}

static int getdict(short infile)
{
    char g_inbuff[BUFSIZ];
    g_inbuff[6] = 25;
    short g_inleft = read(infile, &g_inbuff[0], BUFSIZ);

    if (g_inleft < 0)
        throw std::runtime_error(".z: read error");

    if (g_inbuff[0] != 0x1f)
        throw std::runtime_error(".z: not in packed format");

    if (g_inbuff[1] != 0x1e)
        throw std::runtime_error(".z: not in packed format");

    char *g_inp = &g_inbuff[2];
    long g_origsize = 0;

    for (int i = 0; i < 4; ++i)
        g_origsize = g_origsize * 256 + (*g_inp++ & 0xff);

    short maxlev = *g_inp++ & 0xff;

    if (maxlev > 24)
        throw std::runtime_error(".z: not in packed format");

    short g_intnodes[25];

    for (int i = 1; i <= maxlev; ++i)
        g_intnodes[i] = *g_inp++ & 0377;

    char *g_tree[25];
    char characters[256];
    char *g_xeof = &characters[0];

    for (int i = 1; i <= maxlev; ++i)
    {
        g_tree[i] = g_xeof;

        for (int c = g_intnodes[i]; c > 0; --c)
        {
            if (g_xeof >= &characters[255])
                throw std::runtime_error(".z: not in packed format");

            *g_xeof++ = *g_inp++;
        }
    }

    *g_xeof++ = *g_inp++;
    g_intnodes[maxlev] += 2;
    g_inleft -= g_inp - &g_inbuff[0];

    if (g_inleft < 0)
        throw std::runtime_error(".z: not in packed format");

    int nchildren = 0;

    for (int i = maxlev; i >= 1; i--)
    {
        int c = g_intnodes[i];
        g_intnodes[i] = nchildren /= 2;
        nchildren += c;
    }

    return decode(g_xeof, g_intnodes, g_tree, g_origsize, infile, g_inleft, g_inbuff, g_inp);
}

int main()
{
    int ret = -1;

    try
    {
        ret = getdict(0);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\r\n";
        std::cerr.flush();
    }
    catch (...)
    {
        std::cerr << "Unknown error\r\n";
        std::cerr.flush();
    }

    return ret;
}
