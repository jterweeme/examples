#ifndef CHUNK_H
#define CHUNK_H

#include "toolbox.h"

class Hash;

class Chunk
{
private:
    uint32_t _w[16];
    static const uint32_t _k[64];
    static const uint32_t _r[64];
    static uint32_t leftRotate(uint32_t x, uint32_t c);
    static uint32_t to_uint32(const uint8_t *bytes);
public:
    Hash calc(const Hash &hash);
    void read(const uint8_t *msg);
    void fillTail(uint32_t size);
    void clear();
};

#endif

