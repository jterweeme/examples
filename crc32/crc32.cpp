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
    //adapted from https://www.ietf.org/rfc/rfc1952.txt
    for (uint32_t n = 0; n < 256; ++n)
    {
        uint32_t c = n;
        for (uint32_t k = 0; k < 8; ++k)
            c = c & 1 ? 0xedb88320 ^ (c >> 1) : c >> 1;
        _table[n] = c;
    }
#if 0
    for (uint32_t crc : _table)
        std::cerr << "0x" << hex32(crc) << "\r\n";
#endif
}

void CRC32::update(char c)
{
#if 1
    _crc = _table[(_crc ^ c) & 0xff] ^ (_crc >> 8);
#else
    _crc ^= c;

    for (int i = 0; i < 8; ++i)
        _crc = (_crc >> 1) ^ ((_crc & 1) * uint32_t(0xedb88320));
#endif
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


