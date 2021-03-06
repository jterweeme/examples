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
#include <iostream>
#include <fstream>
#include <algorithm>

#define	END	256
#define	BLKSIZE	512

union FOUR
{
    struct { long int lng; } l;
    struct { char c0, c1, c2, c3; } c;
};

static void output(std::ifstream &ifs, std::ostream &os, long &outsize, long l_insize, int maxlev, long bits[END + 1], int levcount[25], char length[END + 1])
{
    char g_inbuff[BLKSIZE];
    char *inp;
    char **q;
    char outbuff[BLKSIZE + 4];
    outbuff[0] = 0x1f;
    outbuff[1] = 0x1e;
    long temp = l_insize;

    for (int i = 5; i >= 2; i--)
    {
        outbuff[i] = char(temp & 0xff);
        temp >>= 8;
    }

    char *outp = outbuff + 6;
    *outp++ = maxlev;

    for (int i = 1; i < maxlev; i++)
        *outp++ = levcount[i];

    *outp++ = levcount[maxlev] - 2;

    for (int i = 1; i <= maxlev; i++)
        for (int j = 0; j < END; j++)
            if (length[j] == i)
                *outp++ = j;

    ifs.clear();
    ifs.seekg(0, std::ios::beg);
    outsize = 0;
    int bitsleft = 8;
    int inleft = 0;
    union FOUR mask;
    char *maskshuff[4] = {&mask.c.c3, &mask.c.c2, &mask.c.c1, &mask.c.c0};
    int c;

    do
    {
        if (inleft <= 0)
        {
            inp = g_inbuff;
            ifs.read(inp, BLKSIZE);
            inleft = ifs.gcount();
        }
        c = --inleft < 0 ? END : *inp++ & 0xff;
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
            os.write(outbuff, BLKSIZE);
            ((union FOUR *) outbuff)->l.lng = ((union FOUR *) &outbuff[BLKSIZE])->l.lng;
            outp -= BLKSIZE;
            outsize += BLKSIZE;
        }
    }
    while (c != END);

    if (bitsleft < 8)
        outp++;

    c = outp - outbuff;
    os.write(outbuff, c);
    outsize += c;
}

struct Heap
{
    long int count;
    int node;
    void set(const Heap &heap);
};

void Heap::set(const Heap &heap)
{
    count = heap.count;
    node = heap.node;
}

/* makes a heap out of heap[i],...,heap[n] */
static void heapify(int i, int n, Heap *heap)
{
    struct Heap heapsubi;
    heapsubi.set(heap[i]);
    int lastparent = n / 2;

    while (i <= lastparent)
    {
        int k = 2 * i;

        if (heap[k].count > heap[k+1].count && k < n)
            k++;

        if (heapsubi.count < heap[k].count)
            break;

        heap[i].set(heap[k]);
        i = k;
    }

    heap[i].set(heapsubi);
}

static void pack(std::ifstream &ifs, std::ostream &os, long &l_insize, long &outsize)
{
    long g_count[END + 1];

    for (int i = 0; i < END; i++)
        g_count[i] = 0;

    char g_inbuff[BLKSIZE];

    // gather frequency statistics
    std::streamsize bytes_read = 0;

    do
    {
        ifs.read(g_inbuff, BLKSIZE);
        bytes_read = ifs.gcount();

        for (int i = int(bytes_read); i > 0;)
            g_count[g_inbuff[--i] & 0xff] += 2;
    }
    while (bytes_read > 0);

    /* put occurring chars in heap with their counts */
    g_count[END] = 1;
    int n = 0;
    l_insize = 0;
    int	parent[2 * END + 1];
    Heap g_heap[END + 2];

    for (int i = END; i >= 0; i--)
    {
        parent[i] = 0;

        if (g_count[i] > 0)
        {
            l_insize += g_count[i];
            g_heap[++n].count = g_count[i];
            g_heap[n].node = i;
        }
    }

    l_insize >>= 1;

    for (int i = n / 2; i >= 1; i--)
        heapify(i, n, g_heap);

    /* build Huffman tree */
    int lastnode = END;

    while (n > 1)
    {
        parent[g_heap[1].node] = ++lastnode;
        long inc = g_heap[1].count;
        g_heap[1].set(g_heap[n]);
        n--;
        heapify(1, n, g_heap);
        parent[g_heap[1].node] = lastnode;
        g_heap[1].node = lastnode;
        g_heap[1].count += inc;
        heapify(1, n, g_heap);
    }

    parent[lastnode] = 0;
    int g_maxlev = 0;
    int g_levcount[25];

    for (int i = 1; i <= 24; i++)
        g_levcount[i] = 0;

    char g_length[END + 1];

    for (int i = 0; i <= END; i++)
    {
        int c = 0;

        for (int p = parent[i]; p != 0; p = parent[p])
            c++;

        g_levcount[c]++;
        g_length[i] = c;

        if (c > g_maxlev)
            g_maxlev = c;
    }

    if (g_maxlev > 24)
    {
        /* can't occur unless insize >= 2**24 */
        throw ": Huffman tree has too many levels";
    }

    /* compute bit patterns for each character */
    long inc = 1L << 24;
    inc >>= g_maxlev;
    long foo = 0;
    long g_bits[END + 1];

    for (int i = g_maxlev; i > 0; i--)
    {
        for (int c = 0; c <= END; c++)
        {
            if (g_length[c] == i)
            {
                g_bits[c] = foo;
                foo += inc;
            }
        }

        foo &= ~inc;
        inc <<= 1;
    }

    output(ifs, os, outsize, l_insize, g_maxlev, g_bits, g_levcount, g_length);
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

static void packfile(std::string fn, std::ostream &msgs)
{
    msgs << fn;
    std::ifstream ifs;
    ifs.open(fn, std::ifstream::binary);
    fn.append(".z");
    std::ofstream ofs;
    ofs.open(fn, std::ofstream::binary);
    long insize = 0;
    long outsize = 0;
    pack(ifs, ofs, insize, outsize);
    msgs << ": " << insize << " in, " << outsize << " out\r\n";
    ofs.close();
    ifs.close();
}

int main(int argc, char **argv)
{
    Options o;
    o.parse(argc, argv);
    int failcount = 0;

    for (Options::fit it = o.fcbegin(); it != o.fcend(); ++it)
    {
        try
        {
            packfile(*it, std::cout);
        }
        catch (...)
        {
            failcount++;
            std::cerr << "Error\r\n";
        }
    }

    return failcount;
}



