#include <iostream>

int findHead(std::istream *is)
{
    while (*is)
    {
        char c;

        c = is->get();

        if (c != 0x00)
            continue;

        c = is->get();

        if (c != 0x00)
            continue;

        c = is->get();

        if (c != 0x01)
            continue;

        return 1;
    }

    // niet gevonden, einde stream
    return 0;
}

int main()
{
    std::istream *is = &std::cin;
    std::ostream *os = &std::cout;

    while (*is)
    {
        findHead(is);
        uint8_t status = is->get();
        
        switch (status)
        {
        case 0xba:
            is->ignore(10);
            break;
        case 0xbd:
        {
            uint16_t len = 0;
            len |= is->get();
            len <<= 8;
            len |= is->get();
            is->ignore(11);
            uint8_t id = is->get();
            is->ignore(3);
            len -= 15;
            char buf[len];
            is->read(buf, len);
            
            if (id == 0x80)
                os->write(buf, len);
        }
            break;
        }
    }

    return 0;
}


