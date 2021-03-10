#ifndef PACK_H
#define PACK_H

#include <fstream>

static constexpr uint16_t END = 256;
static constexpr uint16_t BLKSIZE = 512;
static constexpr uint8_t LEVEL_LIMIT = 24;

union FOUR
{
    struct { long int lng; } l;
    struct { char c0, c1, c2, c3; } c;
};

struct Heap
{
    long int count;
    int node;
    void set(const Heap &heap);
};

void pack(std::ifstream &ifs, std::ostream &os, long &l_insize, long &outsize);

#endif

