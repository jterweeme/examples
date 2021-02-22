#include "table.h"
#include <iostream>
#include <sstream>

Table::Table(uint32_t symbolCount) : _codeLengths(258), _symbolCount(symbolCount)
{
    _bases.fill(0);
    _limits.fill(0);
    _symbols.fill(0);
}

void Table::calc()
{
    for (uint32_t i = 0; i < _symbolCount + 2; i++)
        _bases[_codeLengths.at(i) + 1]++;

    for (uint32_t i = 1; i < 25; i++)
        _bases.at(i) += _bases.at(i - 1);

    uint8_t minLength2 = minLength();
    uint8_t maxLength2 = maxLength();

    for (int32_t i = minLength2, code = 0; i <= maxLength2; i++)
    {
        int32_t base = code;
        code += _bases.at(i + 1) - _bases.at(i);
        _bases.at(i) = base - _bases.at(i);
        _limits.at(i) = code - 1;
        code <<= 1;
    }

    uint8_t n = minLength2;

    for (uint32_t i = 0; n <= maxLength2; n++)
        for (uint32_t symbol = 0; symbol < _symbolCount + 2; symbol++)
            if (_codeLengths.at(symbol) == n)
                _symbols.at(i++) = symbol;
}

uint8_t Table::_minLength(uint32_t n)
{
    return _codeLengths.min(n);
}

uint8_t Table::_maxLength(uint32_t n)
{
    return _codeLengths.max(n);
}

uint8_t Table::maxLength()
{
    return _maxLength(_symbolCount + 2);
}

uint8_t Table::minLength()
{
    return _minLength(_symbolCount + 2);
}

void Table::add(uint8_t v)
{
    _codeLengths.set(_pos++, v);
}

int32_t Table::limit(uint8_t i) const
{
    return _limits.at(i);
}

uint32_t Table::symbol(uint16_t i) const
{
    return _symbols.at(i);
}

uint32_t Table::base(uint8_t i) const
{
    return _bases.at(i);
}

void Table::dump(std::ostream &os) const
{
    os << _codeLengths.toString() << "\n";
}

std::string Table::toString() const
{
    std::ostringstream o; dump(o); return o.str();
}

void Tables::dump(std::ostream &os) const
{
    for (const_iterator it = begin(); it != end(); it++)
        os << it->toString() << "\n";
}

