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

void pack(std::ifstream &ifs, std::ostream &os, uint32_t &l_insize, uint32_t &outsize);

#endif

