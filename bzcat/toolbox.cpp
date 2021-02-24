#include "toolbox.h"
#include <sstream>

Fugt::Fugt(uint32_t size) : Fector<uint8_t>(size)
{

}

std::string Fugt::toString() const
{
    std::ostringstream o; dump(o); return o.str();
}

void Fugt::dump(std::ostream &os) const
{
    os << "Fugt, size: " << size() << "\n";

    for (uint32_t i = 0; i < size(); i++)
        os << (uint32_t)at(i) << " ";
}

MoveToFront::MoveToFront() : Fugt(256)
{
    for (uint32_t i = 0; i < 256; i++)
        set(i, i);
}

uint8_t MoveToFront::indexToFront(uint32_t index)
{
    uint8_t value = at(index);

    for (uint32_t i = index; i > 0; i--)
        set(i, at(i - 1));

    return set(0, value);
}

