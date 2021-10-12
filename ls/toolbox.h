//file: ls/toolbox.h

#include <filesystem>

class Toolbox
{
public:
    static std::filesystem::path pwd();
    static char hex4(uint8_t n);
    static std::string hex8(uint8_t b);
    static void hex8(std::ostream &os, uint8_t b);
    static std::string hex16(uint16_t w);
    static std::string hex32(uint32_t dw);
    static std::string hex64(uint64_t dw64);
    static uint32_t iPow32(uint32_t base, uint32_t exp);
    static uint64_t iPow64(uint64_t base, uint64_t exp);
    static std::string dec32(uint32_t dw);
    static std::string padding(const std::string &s, char c, size_t n);
    static std::string reverseStr(const std::string &s);
};

