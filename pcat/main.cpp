#include <stdexcept>
#include <iostream>
#include <iostream>
#include <vector>
#include <fstream>

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

static constexpr uint8_t LEVEL_LIMIT = 24;

static void unpack(std::istream &infile, std::ostream &os)
{
    uint16_t magic;
    infile.read((char *)(&magic), 2);

    char inbuff[BUFSIZ];
    infile.read(inbuff, BUFSIZ);
    std::streamsize inleft = infile.gcount();
    char *inp = inbuff;
    uint32_t origsize = 0;

    for (int i = 0; i < 4; ++i)
        origsize = origsize * 256 + (*inp++ & 0xff);

    std::cerr << "Length: " << origsize << "\r\n";
    std::cerr.flush();
    uint8_t maxlev = *inp++ & 0xff;

    if (maxlev > LEVEL_LIMIT)
        throw std::runtime_error(".z: not in packed format");

    short intnodes[LEVEL_LIMIT + 1];

    for (int i = 1; i <= maxlev; ++i)
        intnodes[i] = *inp++ & 0xff;

    char *tree[25];
    char characters[256];
    char *xeof = characters;

    for (int i = 1; i <= maxlev; ++i)
    {
        tree[i] = xeof;

        for (int c = intnodes[i]; c > 0; --c)
        {
            if (xeof >= characters + 255)
                throw std::runtime_error(".z: not in packed format");

            *xeof++ = *inp++;
        }
    }

    *xeof++ = *inp++;
    intnodes[maxlev] += 2;
    inleft -= inp - inbuff;

    if (inleft < 0)
        throw std::runtime_error(".z: not in packed format");

    int nchildren = 0;

    for (uint8_t i = maxlev; i >= 1; --i)
    {
        int c = intnodes[i];
        intnodes[i] = nchildren /= 2;
        nchildren += c;
    }

    {
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
                int j = i - intnodes[lev];

                if (j < 0)
                {
                    ++lev;
                    continue;
                }

                const char *p = tree[lev] + j;

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
