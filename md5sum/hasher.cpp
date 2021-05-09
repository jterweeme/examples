#include "hasher.h"
#include "chunk.h"
#include <cstring>

Hash Hasher::stream(std::istream &is)
{
    Hash hash;

    for (unsigned i = 0; is; ++i)
    {
        uint8_t data[64] = {0};
        is.read((char *)data, 64);
        Chunk chunk;

        if (is.gcount() < 56)
        {
            data[is.gcount()] = 0x80;
            chunk.read(data);
            chunk.fillTail(i * 64 + is.gcount());
        }
        else if (is.gcount() < 64)
        {
            data[is.gcount()] = 0x80;
            chunk.read(data);
            Hash foo = chunk.calc(hash);
            hash.add(foo);
            chunk.clear();
            chunk.fillTail(i * 64 + is.gcount());
        }
        else
        {
            chunk.read(data);
        }

        Hash foo = chunk.calc(hash);
        hash.add(foo);
    }

    return hash;
}

Hash Hasher::array(const char *s, size_t size)
{
    Hash hash;

    for (size_t i = 0; i < size; i+= 64)
    {
        size_t readSize = (size - i) % 64;
        uint8_t data[64] = {0};
        memcpy(data, s + i, readSize);
        Chunk chunk;

        if (readSize < 56)
        {
            data[readSize] = 0x80;
            chunk.read(data);
            chunk.fillTail(size);
        }
        else if (readSize < 64)
        {
            data[readSize] = 0x80;
            chunk.read(data);
            Hash foo = chunk.calc(hash);
            hash.add(foo);
            chunk.clear();
            chunk.fillTail(size);
        }
        else
        {
            chunk.read(data);
        }

        Hash foo = chunk.calc(hash);
        hash.add(foo);
    }

    return hash;
}

