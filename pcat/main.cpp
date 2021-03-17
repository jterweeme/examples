#include "toolbox.h"
#include <stdexcept>
#include <iostream>
#include <iostream>
#include <vector>
#include <fstream>

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

static void dumpIntnodes(std::ostream &os, uint16_t *intnodes, uint8_t n)
{
#if 1
    os << "Intnodes:";

    for (uint8_t i = 0; i < n; ++i)
        os << " " << intnodes[i];

    os << "\r\n";
    os.flush();
#endif
}

static void unpack(std::istream &is, std::ostream &os, bool verbose = false)
{
    uint16_t magic;
    is.read((char *)(&magic), 2);
    uint32_t origsize = 0;
    is.read((char *)(&origsize), 4);
    origsize = Toolbox::be32toh(origsize);
    uint8_t maxlev = uint8_t(is.get());

    if (verbose)
    {
        std::cerr << "Length: " << origsize << ", Levels: " << uint16_t(maxlev) << "\r\n";
        std::cerr.flush();
    }

    uint16_t intnodes[maxlev];

    for (uint8_t i = 0; i < maxlev; ++i)
        intnodes[i] = uint16_t(is.get());

    if (verbose)
        dumpIntnodes(std::cerr, intnodes, maxlev);

    char *tree[maxlev];
    char characters[256];
    char *xeof = characters;

    for (uint8_t i = 0; i < maxlev; ++i)
    {
        tree[i] = xeof;

        for (int c = intnodes[i]; c > 0; --c)
        {
            if (xeof >= characters + 255)
                throw std::runtime_error(".z: not in packed format");

            *xeof++ = char(is.get());
        }
    }

    *xeof++ = char(is.get());

    if (verbose)
        dumpIntnodes(std::cerr, intnodes, maxlev);

    intnodes[maxlev - 1] += 2;

    if (verbose)
        dumpIntnodes(std::cerr, intnodes, maxlev);

    {
        uint32_t nchildren = 0;

        for (uint8_t i = maxlev; i >= 1; --i)
        {
            int c = intnodes[i - 1];
            intnodes[i - 1] = nchildren /= 2;
            nchildren += c;
        }
    }

    if (verbose)
        dumpIntnodes(std::cerr, intnodes, maxlev);

    char outbuff[BUFSIZ];
    char *outp = outbuff;
    uint32_t lev = 1, i = 0;

    while (true)
    {
        int c = is.get();

        for (uint8_t bit = 0; bit < 8; ++bit)
        {
            i *= 2;

            if (c & 0200)
                ++i;

            c <<= 1;
            {
                int j = i - intnodes[lev - 1];

                if (j < 0)
                {
                    ++lev;
                    continue;
                }

                const char *p = tree[lev - 1] + j;

                if (p == xeof)
                {
                    c = outp - outbuff;
                    os.write(outbuff, c);
                    origsize -= c;

                    if (origsize != 0)
                        throw std::runtime_error(".z: unpacking error");

                    return;
                }

                *outp++ = *p;
            }

            if (outp == outbuff + BUFSIZ)
            {
                outp = outbuff;
                os.write(outp, BUFSIZ);
                origsize -= BUFSIZ;
            }

            lev = 1;
            i = 0;
        }
    }
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

#if 1
int main(int argc, char **argv)
{
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
            unpack(std::cin, std::cout, true);
        }
        else
        {
            std::cerr << o.fn() << "\r\n";
            std::cerr.flush();
            std::ifstream ifs;
            ifs.open(o.fn(), std::ios::binary);
            unpack(ifs, std::cout, true);
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

    return 0;
}
#else
int main()
{
#ifdef WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    try
    {
        std::ifstream ifs;
        ifs.open("d:\\temp\\nato.txt.z", std::ios::binary);
        unpack(ifs, std::cout, true);
        ifs.close();
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

    return 0;
}
#endif

