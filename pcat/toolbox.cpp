#include "toolbox.h"

//TODO: gebruik preprocessor om host te achterhalen
uint32_t Toolbox::be32toh(uint32_t num)
{
    return swapEndian(num);
}

uint32_t Toolbox::swapEndian(uint32_t num)
{
    return (num >> 24 & 0xff) | (num << 8 & 0xff0000) | (num >> 8 & 0xff00) | (num << 24 & 0xff000000);
}

