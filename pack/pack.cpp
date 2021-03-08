#include "pack.h"

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

void pack(std::ifstream &ifs, std::ostream &os, long &l_insize, long &outsize)
{
    uint32_t maxlev = 0;
    int levcount[25] = {0};
    uint8_t length[END + 1];
    long bits[END + 1];

    {
        long g_count[END + 1] = {0};

        // gather frequency statistics
        while (ifs.good())
        {
            char g_inbuff[BLKSIZE];
            ifs.read(g_inbuff, BLKSIZE);
            std::streamsize bytes_read = ifs.gcount();

            for (int i = 0; i < int(bytes_read); ++i)
            {
                uint8_t byte = g_inbuff[i];
                g_count[byte] += 2;
            }
        }

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

        for (int i = n / 2; i >= 1; --i)
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

        for (int i = 0; i <= END; i++)
        {
            uint32_t c = 0;

            for (int p = parent[i]; p != 0; p = parent[p])
                c++;

            levcount[c]++;
            length[i] = c;
            maxlev = std::max(maxlev, c);
        }

        if (maxlev > 24)
        {
            /* can't occur unless insize >= 2**24 */
            throw std::range_error(": Huffman tree has too many levels");
        }

        /* compute bit patterns for each character */
        for (uint32_t i = maxlev, foo = 0, inc = 1 << (24 - maxlev); i > 0; i--)
        {
            for (int c = 0; c <= END; c++)
            {
                if (length[c] == i)
                {
                    bits[c] = foo;
                    foo += inc;
                }
            }

            foo &= ~inc;
            inc <<= 1;
        }
    }

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

        for (uint32_t i = 1; i < maxlev; i++)
            *outp++ = levcount[i];

        *outp++ = levcount[maxlev] - 2;

        for (uint32_t i = 1; i <= maxlev; i++)
            for (int j = 0; j < END; j++)
                if (length[j] == i)
                    *outp++ = j;

        //status bits moeten gecleared worden om te kunnen seeken
        ifs.clear();
        ifs.seekg(0, std::ios::beg);
        outsize = 0;
        int bitsleft = 8;

        {
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
        }

        if (bitsleft < 8)
            outp++;

        int n2 = outp - outbuff;
        os.write(outbuff, n2);
        outsize += n2;
    }
}

