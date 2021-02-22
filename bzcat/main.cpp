#include "bitstream.h"
#include "table.h"
#include "block.h"
#include "stream.h"
#include <vector>
#include <array>
#include <fstream>

MoveToFront::MoveToFront() : Fugt(256)
{
    for (uint32_t i = 0; i < 256; i++)
        set(i, i);
}

uint8_t MoveToFront::indexToFront(uint32_t index)
{
    uint8_t value = at(index);
    for (uint32_t i = index; i > 0; i--) set(i, at(i - 1));
    return set(0, value);
}

int main(int argc, char **argv)
{
    std::ifstream ifs;
    ifs.open(argv[1]);

    std::cerr << ifs.is_open();

    std::cerr << "Bestand geopend\r\n";
    BitInputStream bi(&ifs);
    DecStream ds(&bi);
    ds.extractTo(std::cout);
    ifs.close();
    return 0;
}

