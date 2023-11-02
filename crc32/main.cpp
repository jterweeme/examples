#include <iostream>
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

int main()
{
    std::istream *is = &std::cin;
    
    uint32_t crc = 0xffffffff;
    
    while (true)
    {
        int c = is->get();
        
        if (c == -1)
            break;

        crc ^= c;

        for (int i = 0; i < 8; ++i)
            crc = (crc >> 1) ^ ((crc & 1) * uint32_t(0xedb88320));
    }

    crc = ~crc;
    std::cout << "0x" << hex32(crc) << "\r\n";
    return 0;
}


