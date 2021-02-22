#ifndef TABLE_H
#define TABLE_H

#include <array>
#include <vector>
#include "toolbox.h"

class Table
{
private:
    Fugt _codeLengths;
    uint16_t _pos = 0;
    uint32_t _symbolCount;
    std::array<uint32_t, 25> _bases;
    std::array<int32_t, 24> _limits;
    std::array<uint32_t, 258> _symbols;
    uint8_t _minLength(uint32_t n);
    uint8_t _maxLength(uint32_t n);
public:
    Table(uint32_t symbolCount);
    void calc();
    uint8_t maxLength();
    uint8_t minLength();
    void add(uint8_t v);
    int32_t limit(uint8_t i) const;
    uint32_t symbol(uint16_t i) const;
    uint32_t base(uint8_t i) const;
    void dump(std::ostream &os) const;
    std::string toString() const;
};

class Tables : public std::vector<Table>
{
public:
    Tables() : vector<Table>() { }
    void dump(std::ostream &os) const;
    std::string toString() const;
};

#endif

