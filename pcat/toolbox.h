#ifndef TOOLBOX_H
#define TOOLBOX_H

#include <cstdint>

class Toolbox
{
public:
    static uint32_t swapEndian(uint32_t x);
    static uint32_t be32tohost(uint32_t num);
};

#endif

