#include <stdexcept>
#include <iostream>
#include <iostream>
#include <vector>
#include <fstream>

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

static int decode(const char *xeof, const short intnodes[25], char **tree, long origsize, std::istream &infile, short inleft, char *inbuff, char *inp, std::ostream &os)
{
    char outbuff[BUFSIZ];
    char *g_outp = outbuff;
    uint32_t lev = 1;
    int i = 0;

    while (true)
    {
        if (inleft <= 0)
        {
            inp = inbuff;
            infile.read(inp, BUFSIZ);
            inleft = infile.gcount();

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
                c = g_outp - outbuff;
                os.write(outbuff, c);
                origsize -= c;

                if (origsize != 0)
                    throw std::runtime_error(".z: unpacking error");

                return (1);
            }

            *g_outp++ = *p;

            if (g_outp == outbuff + BUFSIZ)
            {
                g_outp = outbuff;
                os.write(g_outp, BUFSIZ);
                origsize -= BUFSIZ;
            }

            lev = 1;
            i = 0;
        }
    }
}

static int getdict(std::istream &infile, std::ostream &os)
{
    char g_inbuff[BUFSIZ];
    g_inbuff[6] = 25;
    infile.read(g_inbuff, BUFSIZ);
    std::streamsize g_inleft = infile.gcount();

    if (g_inleft < 0)
        throw std::runtime_error(".z: read error");

    if (g_inbuff[0] != 0x1f)
        throw std::runtime_error(".z: not in packed format");

    if (g_inbuff[1] != 0x1e)
        throw std::runtime_error(".z: not in packed format");

    char *g_inp = g_inbuff + 2;
    long g_origsize = 0;

    for (int i = 0; i < 4; ++i)
        g_origsize = g_origsize * 256 + (*g_inp++ & 0xff);

    short maxlev = *g_inp++ & 0xff;

    if (maxlev > 24)
        throw std::runtime_error(".z: not in packed format");

    short g_intnodes[25];

    for (int i = 1; i <= maxlev; ++i)
        g_intnodes[i] = *g_inp++ & 0xff;

    char *g_tree[25];
    char characters[256];
    char *g_xeof = characters;

    for (int i = 1; i <= maxlev; ++i)
    {
        g_tree[i] = g_xeof;

        for (int c = g_intnodes[i]; c > 0; --c)
        {
            if (g_xeof >= characters + 255)
                throw std::runtime_error(".z: not in packed format");

            *g_xeof++ = *g_inp++;
        }
    }

    *g_xeof++ = *g_inp++;
    g_intnodes[maxlev] += 2;
    g_inleft -= g_inp - g_inbuff;

    if (g_inleft < 0)
        throw std::runtime_error(".z: not in packed format");

    int nchildren = 0;

    for (int i = maxlev; i >= 1; --i)
    {
        int c = g_intnodes[i];
        g_intnodes[i] = nchildren /= 2;
        nchildren += c;
    }

    return decode(g_xeof, g_intnodes, g_tree, g_origsize, infile, g_inleft, g_inbuff, g_inp, os);
}

class Options
{
private:
    bool _stdin;
    std::string _fn;
public:
    Options();
    void parse(int argc, char **argv);
    bool stdinput() const;
    std::string fn() const;
};

Options::Options() : _stdin(true)
{

}

void Options::parse(int argc, char **argv)
{
    if (argc > 1)
    {
        _fn = argv[1];
        _stdin = false;
    }
}

bool Options::stdinput() const
{
    return _stdin;
}

std::string Options::fn() const
{
    return _fn;
}

int main(int argc, char **argv)
{
    int ret = -1;

#ifdef WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    try
    {
        Options o;
        o.parse(argc, argv);

        if (o.stdinput())
        {
            ret = getdict(std::cin, std::cout);
        }
        else
        {
            std::cerr << o.fn() << "\r\n";
            std::cerr.flush();
            std::ifstream ifs;
            ifs.open(o.fn(), std::ios::binary);
            ret = getdict(ifs, std::cout);
            ifs.close();
        }
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
