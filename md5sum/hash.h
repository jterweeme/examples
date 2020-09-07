#ifndef HASH_H
#define HASH_H

#include "toolbox.h"

class Hash
{
private:
    static CONSTEXPR uint32_t H0 = 0x67452301;
    static CONSTEXPR uint32_t H1 = 0xefcdab89;
    static CONSTEXPR uint32_t H2 = 0x98badcfe;
    static CONSTEXPR uint32_t H3 = 0x10325476;
    uint32_t _h0, _h1, _h2, _h3;
public:
    Hash();
    Hash(uint32_t h0, uint32_t h1, uint32_t h2, uint32_t h3);
    void reset();
    void add(const Hash &h);
    uint32_t h0() const;
    uint32_t h1() const;
    uint32_t h2() const;
    uint32_t h3() const;
    void dump(std::ostream &os) const;
};

#endif

