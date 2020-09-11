#ifndef HASHER_H
#define HASHER_H

#include "hash.h"

class Hasher
{
public:
    static Hash stream(std::istream &is);
    static Hash array(const char *s, size_t size);
};
#endif

