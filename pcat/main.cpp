#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdexcept>
#include <iostream>
#include <iostream>

static char *outp;
static char outbuff[BUFSIZ];
static short outfile;
static char *inp;
static char inbuff[BUFSIZ];
static short inleft, infile;
static long origsize;
static char *xeof;
static short intnodes[25];
static char *tree[25];

static int decode()
{
    int bitsleft, c;
    int j;
    char *p;

    outp = &outbuff[0];
    int lev = 1;
    int i = 0;

    while (1)
    {
        if (inleft <= 0)
        {
            inleft = read(infile, inp = &inbuff[0], BUFSIZ);

            if (inleft < 0)
                throw std::runtime_error(".z: read error");
        }

        if (--inleft < 0)
        {
uggh:
            if (origsize == 0)
                return 1;

            throw std::runtime_error(".z: unpacking error");
        }

        c = *inp++;
        bitsleft = 8;

        while (--bitsleft >= 0)
        {
            i *= 2;

            if (c & 0200)
                i++;

            c <<= 1;

            if ((j = i - intnodes[lev]) >= 0)
            {
                p = &tree[lev][j];

                if (p == xeof)
                {
                    c = outp - &outbuff[0];

                    if (write (outfile, &outbuff[0], c) != c)
                    {
wrerr:
                        throw std::runtime_error(": write error");
                    }
                    origsize -= c;

                    if (origsize != 0)
                        goto uggh;

                    return (1);
                }
                *outp++ = *p;

                if (outp == &outbuff[BUFSIZ])
                {
                    if (write(outfile, outp = &outbuff[0], BUFSIZ) != BUFSIZ)
                        goto wrerr;

                    origsize -= BUFSIZ;
                }

                lev = 1;
                i = 0;
            }
            else
            {
                lev++;
            }
        }
    }
}

static short maxlev;
static char characters[256];

int getdict()
{
    int c, i, nchildren;
    xeof = &characters[0];
    inbuff[6] = 25;
    inleft = read(infile, &inbuff[0], BUFSIZ);

    if (inleft < 0)
        throw std::runtime_error(".z: read error");

    if (inbuff[0] != 0x1f)
        throw std::runtime_error(".z: not in packed format");

    if (inbuff[1] != 0x1e)
        throw std::runtime_error(".z: not in packed format");

    inp = &inbuff[2];
    origsize = 0;

    for (i = 0; i < 4; i++)
        origsize = origsize * 256 + ((*inp++) & 0377);

    maxlev = *inp++ & 0377;

    if (maxlev > 24)
        throw std::runtime_error(".z: not in packed format");

    for (i = 1; i <= maxlev; i++)
        intnodes[i] = *inp++ & 0377;

    for (i = 1; i <= maxlev; i++)
    {
        tree[i] = xeof;

        for (c = intnodes[i]; c > 0; c--)
        {
            if (xeof >= &characters[255])
                throw std::runtime_error(".z: not in packed format");

            *xeof++ = *inp++;
        }
    }

    *xeof++ = *inp++;
    intnodes[maxlev] += 2;
    inleft -= inp - &inbuff[0];

    if (inleft < 0)
        throw std::runtime_error(".z: not in packed format");

    nchildren = 0;

    for (i = maxlev; i >= 1; i--)
    {
        c = intnodes[i];
        intnodes[i] = nchildren /= 2;
        nchildren += c;
    }

    return decode();
}

int meen(int argc, char **argv)
{
    (void)argc;
    char *pl;

    pl = *argv;

    while (*pl++);  //point pl to end of argv[0] string

    while (--pl >= *argv)
        if (*pl == '/')
            break;

    *argv = pl + 1;
    infile = 0;
    outfile = 1;
    return getdict();
}

int main(int argc, char **argv)
{
    int ret = -1;

    try
    {
        ret = meen(argc, argv);
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
