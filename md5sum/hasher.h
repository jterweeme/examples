#ifndef HASHER_H
#define HASHER_H

#include "hash.h"

class Hasher
{
public:
    static Hash stream(std::istream &is);
};

#endif

