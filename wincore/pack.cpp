/*
 *	Huffman encoding program
 *	Adapted April 1979, from program by T.G. Szymanski, March 1978
 *	Usage:	pack [[ - ] filename ... ] filename ...
 *		- option: enable/disable listing of statistics
 */

#include <fstream>
#include <cstdint>
#include <vector>
#include <string>
#include <cassert>
#include <iostream>
#include <stdexcept>

using std::ifstream;
using std::istream;
using std::ostream;
using std::cerr;
using std::cout;
using std::cin;

static constexpr uint16_t END = 256;
static constexpr uint16_t BLKSIZE = 512;
static constexpr uint8_t LEVEL_LIMIT = 24;

union FOUR
{
    struct { long int lng; } l;
    struct { char c0, c1, c2, c3; } c;
};

class Heap
{
private:
    uint32_t _count;
    uint32_t _node;
public:
    void set(const Heap &heap);
    void set(uint32_t count, uint32_t node);
    uint32_t count() const;
    uint32_t node() const;
};

void Heap::set(const Heap &heap)
{
    _count = heap._count;
    _node = heap._node;
}

void Heap::set(uint32_t count, uint32_t node)
{
    _count = count;
    _node = node;
}

uint32_t Heap::count() const
{
    return _count;
}

uint32_t Heap::node() const
{
    return _node;
}

/* makes a heap out of heap[i],...,heap[n] */
static void heapify(uint32_t i, const uint32_t n, Heap *heap)
{       
    Heap heapsubi;
    heapsubi.set(heap[i]);
    const uint32_t lastparent = n / 2;
            
    while (i <= lastparent)
    {           
        uint32_t k = 2 * i;
                    
        if (heap[k].count() > heap[k + 1].count() && k < n)
            k++;    
                    
        if (heapsubi.count() < heap[k].count())
            break;  
                    
        heap[i].set(heap[k]);
        i = k;      
    }               

    heap[i].set(heapsubi);
}

void pack(ifstream &ifs, ostream &os, uint32_t &insize, uint32_t &outsize)
{           
    uint32_t maxlev = 0;
    uint32_t levcount[25] = {0};
    uint8_t length[END + 1];
    uint32_t bits[END + 1];
            
    {           
        uint32_t count[END + 1] = {0};
            
        // gather frequency statistics
        while (ifs.good())
        {
            char g_inbuff[BLKSIZE];
            ifs.read(g_inbuff, BLKSIZE);
            uint32_t bytes_read = uint32_t(ifs.gcount());

            for (uint32_t i = 0; i < bytes_read; ++i)
            {
                uint8_t byte = g_inbuff[i];
                count[byte] += 2;
            }
        }

        uint32_t parent[2 * END + 1];

        /* put occurring chars in heap with their counts */
        {
            count[END] = 1;
            uint32_t n = 0;
            insize = 0;

            Heap g_heap[END + 2];

            for (int i = END; i >= 0; i--)
            {
                parent[i] = 0;

                if (count[i] > 0)
                {
                    insize += count[i];
                    ++n;
                    g_heap[n].set(count[i], i);
                }
            }

            insize >>= 1;

            for (uint32_t i = n / 2; i >= 1; --i)
                heapify(i, n, g_heap);

            /* build Huffman tree */
            {
                uint32_t lastnode = END;

                while (n > 1)
                {
                    uint32_t tmp = g_heap[1].node();
                    parent[tmp] = ++lastnode;
                    uint32_t inc = g_heap[1].count();
                    g_heap[1].set(g_heap[n]);
                    n--;
                    heapify(1, n, g_heap);
                    tmp = g_heap[1].node();
                    parent[tmp] = lastnode;
                    tmp = g_heap[1].count();
                    g_heap[1].set(tmp + inc, lastnode);
                    heapify(1, n, g_heap);
                }

                parent[lastnode] = 0;
            }
        }

        for (uint32_t i = 0; i <= END; i++)
        {
            uint32_t c = 0;

            for (int p = parent[i]; p != 0; p = parent[p])
                c++;

            levcount[c]++;
            length[i] = c;
            maxlev = std::max(maxlev, c);
        }

        if (maxlev > LEVEL_LIMIT)
        {
            /* can't occur unless insize >= 2**24 */
            throw std::range_error(": Huffman tree has too many levels");
        }

        /* compute bit patterns for each character */
        for (uint32_t i = maxlev, foo = 0, inc = 1 << (LEVEL_LIMIT - maxlev); i > 0; --i)
        {
            for (uint16_t c = 0; c <= END; ++c)
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
        char outbuff[BLKSIZE + 4];
        outbuff[0] = 0x1f;
        outbuff[1] = 0x1e;
        long temp = insize;

        for (int i = 5; i >= 2; --i)
        {
            outbuff[i] = char(temp & 0xff);
            temp >>= 8;
        }

        char *outp = outbuff + 6;
        *outp++ = maxlev;

        for (uint32_t i = 1; i < maxlev; i++)
            *outp++ = levcount[i];

        *outp++ = levcount[maxlev] - 2;

        for (uint32_t i = 1; i <= maxlev; ++i)
            for (uint16_t j = 0; j < END; ++j)
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
            char *inp;

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
                char **q = &maskshuff[0];

                if (bitsleft == 8)
                    *outp = **q++;
                else
                    *outp |= **q++;

                bitsleft -= length[c];

                while (bitsleft < 0)
                    *++outp = **q++, bitsleft += 8;

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

        uint32_t n2 = outp - outbuff;
        os.write(outbuff, n2);
        outsize += n2;
    }
}

int main(int argc, char **argv)
{
    uint32_t insize, outsize;
    ifstream ifs(argv[1]);
    pack(ifs, cout, insize, outsize);
    cerr << ": " << insize << " in, " << outsize << " out\r\n";
    return 0;
}



