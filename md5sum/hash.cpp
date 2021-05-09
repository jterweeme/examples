#include "hash.h"

Hash::Hash() : _h0(H0), _h1(H1), _h2(H2), _h3(H3)
{

}

Hash::Hash(uint32_t h0, uint32_t h1, uint32_t h2, uint32_t h3)
    : _h0(h0), _h1(h1), _h2(h2), _h3(h3)
{
}

void Hash::dump(std::ostream &os) const
{
    Toolbox t;
    (void)t;

    os << t.hex32(t.be32tohost(h0())) <<
          t.hex32(t.be32tohost(h1())) <<
          t.hex32(t.be32tohost(h2())) <<
          t.hex32(t.be32tohost(h3()));
}

uint32_t Hash::h0() const
{
    return _h0;
}

uint32_t Hash::h1() const
{
    return _h1;
}

uint32_t Hash::h2() const
{
    return _h2;
}

uint32_t Hash::h3() const
{
    return _h3;
}

void Hash::add(const Hash &h)
{
    _h0 += h.h0();
    _h1 += h.h1();
    _h2 += h.h2();
    _h3 += h.h3();
}

void Hash::reset()
{
    _h0 = H0;
    _h1 = H1;
    _h2 = H2;
    _h3 = H3;
}

