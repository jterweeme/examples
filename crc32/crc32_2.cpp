#include <iostream>
#include <fstream>
#include <cstdint>

static char nibble(uint8_t n)
{
    return n <= 9 ? '0' + char(n) : 'a' + char(n - 10);
}

static std::string hex32(uint32_t dw)
{
    std::string ret;
    ret.push_back(nibble(dw >> 28 & 0xf));
    ret.push_back(nibble(dw >> 24 & 0xf));
    ret.push_back(nibble(dw >> 20 & 0xf));
    ret.push_back(nibble(dw >> 16 & 0xf));
    ret.push_back(nibble(dw >> 12 & 0xf));
    ret.push_back(nibble(dw >>  8 & 0xf));
    ret.push_back(nibble(dw >>  4 & 0xf));
    ret.push_back(nibble(dw >>  0 & 0xf));
    return ret;
}

class CRC32
{
private:
    uint32_t _table[256];
    uint32_t _crc = 0xffffffff;
    void _makeTable();
public:
    CRC32() { _makeTable(); }
    void update(char c);
    uint32_t crc() const { return ~_crc; }
};

void CRC32::_makeTable()
{
    for (uint32_t i = 0, j, c; i < 256; ++i)
    {
        for (c = i << 24, j = 8; j > 0; --j)
            c = c & 0x80000000 ? (c << 1) ^ 0x04c11db7 : (c << 1);
        _table[i] = c;
    }
#if 0
    for (uint32_t crc : _table)
        std::cerr << "0x" << hex32(crc) << "\r\n";
#endif
}

void CRC32::update(char c)
{
    _crc = (_crc << 8) ^ _table[((_crc >> 24) ^ uint32_t(c)) & 0xff];
}

int main(int argc, char **argv)
{
    CRC32 crc;
    std::istream *is = &std::cin;
    std::ifstream ifs;

    if (argc == 2)
    {
        ifs.open(argv[1]);
        is = &ifs;
    }

    while (true)
    {
        int c = is->get();
        
        if (c == -1)
            break;

        crc.update(c);
    }

    std::cout << "0x" << hex32(crc.crc()) << "\r\n";
    ifs.close();
    return 0;
}


