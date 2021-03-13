#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>

#define NAMELEN 80
#define SUF0	'.'
#define SUF1	'z'
#define US	037
#define RS	036

char *argv0, *argvk;
short errorm;
char characters[256];
char *inp;
char *outp;
char inbuff[BUFSIZ];
char outbuff[BUFSIZ];
short inleft, infile, outfile;
long origsize;
struct stat status;
int	Tree[1024];
char *tree[25];
short intnodes[25];
char filename[NAMELEN + 2];
short maxlev;
char *xeof;

void eprintf(const char *s)
{
    if (!errorm)
    {
        errorm = 1;
        fprintf(stderr, "%s: %s", argv0, argvk);
    }

    fprintf(stderr, s);
}

void putch(char c)
{
    *outp++ = c;

    if (outp == &outbuff[BUFSIZ])
    {
        int n = write (outfile, outp = outbuff, BUFSIZ);
        if (n < BUFSIZ) {
            eprintf (": write error");
            //longjmp (env, 2);
        }
    }
}

char getch()
{
    if (inleft <= 0)
    {
        inleft = read(infile, inp = inbuff, BUFSIZ);

        if (inleft < 0)
        {
            eprintf(".z: read error");
            //longjmp(env, 1);
        }
    }
    inleft--;
    return (*inp++ & 0377);
}

uint16_t getwd()
{
    char c;
    return 0;
}

void expand()
{
    int tp, bit;
    short word;
    int keysize, i, *t;
    outp = outbuff;
    inp = &inbuff[2];
    inleft -= 2;
    origsize = ((long)(unsigned)getwd()) * 256 * 256;
    origsize += (unsigned)getwd();
    t = Tree;

    for (keysize = getwd(); keysize--;)
    {
        if ((i = getch()) == 0377)
            *t++ = getwd();
        else
            *t++ = i & 0377;
    }

    bit = tp = 0;

    while (true)
    {
        if (bit <= 0)
        {
            word = getwd();
            bit = 16;
        }

        tp += Tree[tp + (word < 0)];
        word <<= 1;
        bit--;

        if (Tree[tp] == 0)
        {
            putch(Tree[tp + 1]);
            tp = 0;

            if ((origsize -= 1) == 0)
            {
                write(outfile, outbuff, outp - outbuff);
                return;
            }
        }
    }
}

int decode()
{
    int bitsleft, c, i;
    int j, lev;
    char *p;

    outp = &outbuff[0];
    lev = 1;
    i = 0;

    while (1)
    {
        if (inleft <= 0)
        {
            inleft = read (infile, inp = &inbuff[0], BUFSIZ);

            if (inleft < 0)
            {
                eprintf (".z: read error");
                return (0);
            }
        }
        if (--inleft < 0)
        {
uggh:
            if(origsize == 0)
                return 1;

            eprintf (".z: unpacking error");
            return 0;
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
                        eprintf (": write error");
                        return (0);
                    }
                    origsize -= c;

                    if (origsize != 0)
                        goto uggh;

                    return (1);
                }
                *outp++ = *p;
                if (outp == &outbuff[BUFSIZ]) {
                    if (write (outfile, outp = &outbuff[0], BUFSIZ) != BUFSIZ)
                        goto wrerr;
                    origsize -= BUFSIZ;
                }
                lev = 1;
                i = 0;
            } else
                lev++;
        }
    }
}

int getdict()
{
    int c, i, nchildren;
    xeof = &characters[0];
    inbuff[6] = 25;
    inleft = read(infile, &inbuff[0], BUFSIZ);

    if (inleft < 0)
    {
        eprintf(".z: read error");
        return 0;
    }

    if (inbuff[0] != US)
        goto goof;

    if (inbuff[1] == US)
    {
#if 0
        if (setjmp(env))
            return 0;
#endif

        expand();
        return 1;
    }

    if (inbuff[1] != RS)
        goto goof;

    inp = &inbuff[2];
    origsize = 0;

    for (i = 0; i < 4; i++)
        origsize = origsize * 256 + ((*inp++) & 0377);

    maxlev = *inp++ & 0377;

    if (maxlev > 24)
    {
goof:
        eprintf(".z: not in packed format");
        return 0;
    }

    for (i = 1; i <= maxlev; i++)
        intnodes[i] = *inp++ & 0377;

    for (i = 1; i <= maxlev; i++)
    {
        tree[i] = xeof;

        for (c = intnodes[i]; c > 0; c--)
        {
            if (xeof >= &characters[255])
                goto goof;

            *xeof++ = *inp++;
        }
    }

    *xeof++ = *inp++;
    intnodes[maxlev] += 2;
    inleft -= inp - &inbuff[0];

    if (inleft < 0)
        goto goof;

    nchildren = 0;

    for (i = maxlev; i >= 1; i--)
    {
        c = intnodes[i];
        intnodes[i] = nchildren /= 2;
        nchildren += c;
    }

    return decode();
}

int main(int argc, char **argv)
{
    int i, k;
    int sep, pcat = 0;
    char *pl, *cp;
    int fcount = 0;

    pl = *argv;

    while (*pl++);  //point pl to end of argv[0] string

    while (--pl >= *argv)
        if (*pl == '/')
            break;

    *argv = pl + 1;
    argv0 = argv[0];

    if (**argv == 'p')
        pcat++;

    if (pcat && argc == 1)
    {
        infile = 0;
        outfile = 1;
        return getdict();
    }

    for (k = 1; k < argc; k++)
    {
        errorm = 0;
        sep = -1;
        cp = filename;
        argvk = argv[k];

        for (i = 0; i < (NAMELEN - 3) && (*cp = argvk[i]); i++)
            if (*cp++ == '/')
                sep = i;

        if (cp[-1] == SUF1 && cp[-2] == SUF0)
        {
            argvk[i - 2] = '\0';
            k--;
            continue;
        }

        fcount++;

        if (i >= (NAMELEN - 3) || (i - sep) > 13)
        {
            eprintf(": file name too long");
            goto done;
        }

        *cp++ = SUF0;
        *cp++ = SUF1;
        *cp = '\0';

        if ((infile = open(filename, 0)) == -1)
        {
            eprintf(".z: cannot open");
            goto done;
        }

        if (pcat)
            outfile = 1;
        else
        {
            if (stat(argvk, &status) != -1)
            {
                eprintf(": already exists");
                goto done;
            }

            fstat(infile, &status);

            if (status.st_nlink != 1)
                eprintf(".z: Warning: file has links");

            if ((outfile = creat(argvk, status.st_mode & 07777)) == -1)
            {
                eprintf(": cannot create");
                goto done;
            }

            if (chmod(argvk, status.st_mode) != 0)
                printf("can't change mode to %o\n", status.st_mode);

            //chown(argvk, status.st_uid, status.st_gid);
        }

        if (getdict())
        {
            fcount--;

            if (!pcat)
            {
                eprintf(": unpacked");
                unlink(filename);
                //utime(argvk, &status.st_atime);
            }
        }
        else
        {
            if (!pcat)
                unlink(argvk);
        }
done:
        if (errorm)
            fprintf(stderr, "\n");

        close(infile);

        if (!pcat)
            close(outfile);
    }

    return fcount;
}



