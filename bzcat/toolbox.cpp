#include "toolbox.h"
#include <sstream>

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
