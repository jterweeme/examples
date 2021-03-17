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

//static constexpr uint8_t LEVEL_LIMIT = 24;

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

static void unpack(std::istream &infile, std::ostream &os)
{
    uint16_t magic;
    infile.read((char *)(&magic), 2);
    uint32_t origsize = 0;
    infile.read((char *)(&origsize), 4);
    origsize = Toolbox::be32toh(origsize);
    uint8_t maxlev = uint8_t(infile.get());
#if 1
    std::cerr << "Length: " << origsize << ", Levels: " << uint16_t(maxlev) << "\r\n";
    std::cerr.flush();
#endif

    uint16_t intnodes[maxlev];


    for (uint8_t i = 0; i < maxlev; ++i)
        intnodes[i] = uint16_t(infile.get());


#if 1
    dumpIntnodes(std::cerr, intnodes, maxlev);
#endif



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

            *xeof++ = char(infile.get());
        }
    }

    *xeof++ = char(infile.get());
#if 1
    dumpIntnodes(std::cerr, intnodes, maxlev);
#endif
    intnodes[maxlev - 1] += 2;
#if 1
    dumpIntnodes(std::cerr, intnodes, maxlev);
#endif
    {
        uint32_t nchildren = 0;

        for (uint8_t i = maxlev; i >= 1; --i)
        {
            int c = intnodes[i - 1];
            intnodes[i - 1] = nchildren /= 2;
            nchildren += c;
        }
    }

#if 1
    dumpIntnodes(std::cerr, intnodes, maxlev);
#endif

    char inbuff[BUFSIZ];
    infile.read(inbuff, BUFSIZ);
    std::streamsize inleft = infile.gcount();
    char *inp = inbuff;

    char outbuff[BUFSIZ];
    char *outp = outbuff;
    uint32_t lev = 1, i = 0;

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
                return;

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
            unpack(std::cin, std::cout);
        }
        else
        {
            std::cerr << o.fn() << "\r\n";
            std::cerr.flush();
            std::ifstream ifs;
            ifs.open(o.fn(), std::ios::binary);
            unpack(ifs, std::cout);
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
        unpack(ifs, std::cout);
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

