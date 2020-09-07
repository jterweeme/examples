#include "hasher.h"
#include "chunk.h"

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

