/*
 *	Huffman encoding program
 *	Adapted April 1979, from program by T.G. Szymanski, March 1978
 *	Usage:	pack [[ - ] filename ... ] filename ...
 *		- option: enable/disable listing of statistics
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>
#include <string>

#define	END	256
#define	BLKSIZE	512
#define NAMELEN 80

union FOUR
{
    struct { long int lng; } l;
    struct { char c0, c1, c2, c3; } c;
};

char g_inbuff[BLKSIZE];
long outsize;
union FOUR mask;
char *maskshuff[4] = {&(mask.c.c3), &(mask.c.c2), &(mask.c.c1), &(mask.c.c0)};

int static output(int infile, int outfile, long l_insize, int maxlev, long bits[END + 1], int levcount[25], char length[END + 1])
{
    int inleft;
    char *inp;
    char **q, *outp;
    char outbuff[BLKSIZE + 4];
    outbuff[0] = 0x1f;  /* ascii US */
    outbuff[1] = 0x1e;  /* ascii RS */
    /* output the length and the dictionary */
    long temp = l_insize;

    for (int i = 5; i >= 2; i--)
    {
        outbuff[i] = char(temp & 0xff);
        temp >>= 8;
    }

    outp = &outbuff[6];
    *outp++ = maxlev;

    for (int i = 1; i < maxlev; i++)
        *outp++ = levcount[i];

    *outp++ = levcount[maxlev] - 2;

    int c;

    for (int i = 1; i <= maxlev; i++)
        for (c = 0; c < END; c++)
            if (length[c] == i)
                *outp++ = c;

    /* output the text */
    lseek(infile, 0L, 0);
    outsize = 0;
    int bitsleft = 8;
    inleft = 0;

    do
    {
        if (inleft <= 0)
        {
            inleft = read(infile, inp = g_inbuff, BLKSIZE);

            if (inleft < 0)
            {
                printf (": read error");
                return (0);
            }
        }
        c = (--inleft < 0) ? END : (*inp++ & 0377);
        mask.l.lng = bits[c] << bitsleft;
        q = &maskshuff[0];

        if (bitsleft == 8)
            *outp = **q++;
        else
            *outp |= **q++;

        bitsleft -= length[c];

        while (bitsleft < 0)
        {
            *++outp = **q++;
            bitsleft += 8;
        }

        if (outp >= &outbuff[BLKSIZE])
        {
            if (write(outfile, outbuff, BLKSIZE) != BLKSIZE)
            {
wrerr:
                printf (".z: write error");
                return (0);
            }
            ((union FOUR *) outbuff)->l.lng = ((union FOUR *) &outbuff[BLKSIZE])->l.lng;
            outp -= BLKSIZE;
            outsize += BLKSIZE;
        }
    } while (c != END);

    if (bitsleft < 8)
        outp++;

    c = outp - outbuff;

    if (write(outfile, outbuff, c) != c)
        goto wrerr;

    outsize += c;
    return 1;
}

struct Heap
{
    long int count;
    int node;
};

Heap heap[END + 2];

static void hmove(const Heap &src, Heap &dst)
{
    dst.count = src.count;
    dst.node = src.node;
}

/* makes a heap out of heap[i],...,heap[n] */
static void heapify(int i, int n)
{
    int lastparent;
    struct Heap heapsubi;
    hmove(heap[i], heapsubi);
    lastparent = n / 2;

    while (i <= lastparent)
    {
        int k = 2 * i;

        if (heap[k].count > heap[k+1].count && k < n)
            k++;

        if (heapsubi.count < heap[k].count)
            break;

        hmove(heap[k], heap[i]);
        i = k;
    }
    hmove(heapsubi, heap[i]);
}


//union FOUR g_insize;

/* return 1 after successful packing, 0 otherwise */
static int packfile(int infile, int outfile, long &l_insize)
{
    long g_count[END + 1];

    for (int i = 0; i < END; i++)
        g_count[i] = 0;

    // gather frequency statistics
    for (int i; (i = read(infile, g_inbuff, BLKSIZE)) > 0;)
        while (i > 0)
            g_count[g_inbuff[--i] & 0xff] += 2;

    /* put occurring chars in heap with their counts */
    int diffbytes = -1;
    g_count[END] = 1;
    int n = 0;
    //g_insize.l.lng = 0;
    l_insize = 0;
    int	parent[2 * END + 1];

    for (int i = END; i >= 0; i--)
    {
        parent[i] = 0;

        if (g_count[i] > 0)
        {
            diffbytes++;
            //g_insize.l.lng += g_count[i];
            l_insize += g_count[i];
            heap[++n].count = g_count[i];
            heap[n].node = i;
        }
    }

    //g_insize.l.lng >>= 1;
    l_insize >>= 1;

    for (int i = n / 2; i >= 1; i--)
        heapify(i, n);

    /* build Huffman tree */
    int lastnode = END;

    while (n > 1)
    {
        parent[heap[1].node] = ++lastnode;
        long inc = heap[1].count;
        hmove(heap[n], heap[1]);
        n--;
        heapify(1, n);
        parent[heap[1].node] = lastnode;
        heap[1].node = lastnode;
        heap[1].count += inc;
        heapify(1, n);
    }

    parent[lastnode] = 0;

    /* assign lengths to encoding for each character */
    long bitsout = 0;
    int g_maxlev = 0;
    int g_levcount[25];

    for (int i = 1; i <= 24; i++)
        g_levcount[i] = 0;

    int c;
    int p;
    char g_length[END + 1];

    for (int i = 0; i <= END; i++)
    {
        c = 0;

        for (p = parent[i]; p != 0; p = parent[p])
            c++;

        g_levcount[c]++;
        g_length[i] = c;

        if (c > g_maxlev)
            g_maxlev = c;

        bitsout += c * (g_count[i] >> 1);
    }

    if (g_maxlev > 24)
    {
        /* can't occur unless insize.l.lng >= 2**24 */
        printf (": Huffman tree has too many levels");
        return 0;
    }

    /* bitch if no compression, but do it anyway */
    outsize = ((bitsout + 7) >> 3) + 6 + g_maxlev + diffbytes;

    if ((l_insize + BLKSIZE - 1) / BLKSIZE <= (outsize + BLKSIZE - 1) / BLKSIZE)
    {
        printf (": no saving");
    }

    /* compute bit patterns for each character */
    long inc = 1L << 24;
    inc >>= g_maxlev;
    mask.l.lng = 0;
    long g_bits[END + 1];

    for (int i = g_maxlev; i > 0; i--)
    {
        for (c = 0; c <= END; c++)
        {
            if (g_length[c] == i)
            {
                g_bits[c] = mask.l.lng;
                mask.l.lng += inc;
            }
        }

        mask.l.lng &= ~inc;
        inc <<= 1;
    }

    return output(infile, outfile, l_insize, g_maxlev, g_bits, g_levcount, g_length);
}

class Options
{
private:
    std::vector<std::string> _files;
public:
    typedef std::vector<std::string>::const_iterator fit;
    void parse(int argc, char **argv);
    fit fcbegin() const;
    fit fcend() const;
};

void Options::parse(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i)
    {
        _files.push_back(argv[i]);
    }
}

Options::fit Options::fcbegin() const
{
    return _files.cbegin();
}

Options::fit Options::fcend() const
{
    return _files.cend();
}

int main(int argc, char **argv)
{
    const char SUF0 = '.';
    const char SUF1 = 'z';
    int fcount = 0; /* count failures */
    char filename[NAMELEN];
    struct stat status;

    for (int k = 1; k < argc; k++)
    {
        fcount++; // increase failure count - expect the worst
        printf("%s: %s", argv[0], argv[k]);
        int sep = -1;
        char *cp = filename;
        int i;

        for (i = 0; i < (NAMELEN - 3) && (*cp = argv[k][i]); i++)
            if (*cp++ == '/')
                sep = i;

        int infile = open(filename, 0);

        if (infile < 0)
        {
            printf(": cannot open\n");
            continue;
        }

        fstat(infile, &status);

        *cp++ = SUF0;
        *cp++ = SUF1;
        *cp = '\0';
        int	g_outfile = creat(filename, status.st_mode & 0xfff);

        if (g_outfile < 0)
        {
            printf(".z: cannot create\n");
            goto closein;
        }

        long l_insize;

        if (packfile(infile, g_outfile, l_insize))
        {
            fcount--;  /* success after all */

            if (l_insize != 0)
            {
                long saved = l_insize - outsize;
                printf(": %.1f%% saved\n", (double(saved) / double(l_insize)) * 100);
            }
            else
            {
                printf(": empty file\n");
            }
        }
        else
        {
            printf(" - file unchanged\n");
        }

closein:
        close(g_outfile);
        close(infile);
    }
    return fcount;
}



