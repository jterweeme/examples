#include <iostream>
#include <fstream>

class Toolbox
{
public:
    static char hex4(uint8_t n);
    static std::string hex8(uint8_t b);
    static void writeWLE(char *buf, uint16_t w);
    static void writeDwLE(char *buf, uint32_t dw);
    static uint16_t readWBE(std::istream &is);
};

class Options
{
private:
    bool _summary = true;
public:
    void parse(int argc, char **argv);
    bool summary() const { return _summary; }
    bool audio() const { return true; }
};

uint16_t Toolbox::readWBE(std::istream &is)
{
    uint16_t ret = 0;
    ret |= is.get();
    ret <<= 8;
    ret |= is.get();
    return ret;
}

char Toolbox::hex4(uint8_t n)
{
    return n <= 9 ? '0' + char(n) : 'A' + char(n - 10);
}

std::string Toolbox::hex8(uint8_t b)
{
    std::string ret;
    ret += hex4(b >> 4 & 0xf);
    ret += hex4(b >> 0 & 0xf);
    return ret;
}

void Toolbox::writeWLE(char *buf, uint16_t w)
{
    buf[0] = char(w >> 0 & 0xff);
    buf[1] = char(w >> 8 & 0xff);
}

void Toolbox::writeDwLE(char *buf, uint32_t dw)
{
    buf[0] = char(dw >>  0 & 0xff);
    buf[1] = char(dw >>  8 & 0xff);
    buf[2] = char(dw >> 16 & 0xff);
    buf[3] = char(dw >> 24 & 0xff);
}

uint32_t get_bit(const char *buf, size_t bit_offset)
{
    int byte = bit_offset >> 3;
    return 0;
}

uint32_t get_bits(const char *buf, size_t bit_offset, size_t n)
{
    uint32_t ret = 0;

    while (n)
    {
        int current_byte = buf[bit_offset >> 3];
        int remaining = 8 - (bit_offset & 7);
        int read = remaining < n ? remaining : n;
        int shift = remaining - read;
        int mask = 0xff >> 8 - read;
        ret = (ret << read) | ((current_byte & (mask << shift)) >> shift);
        bit_offset += read;
        n -= read;
    }

    return ret;
}

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
    Options o;
    std::istream *is = &std::cin;
    std::ostream *os = &std::cout;
    std::ofstream ao;
    uint32_t count[256] = {0};

    if (o.audio())
        ao.open("audio.mp2");

    while (*is)
    {
        findHead(is);
        uint8_t status = is->get();
        ++count[status];
        
        switch (status)
        {
        case 0xba:
        {
            //reserveer 10 bytes voor mogelijk type 2 pack header
            char buf[10];
            is->read(buf, 1);
            uint8_t mpeg_type = 0;
            uint64_t clock = 0;
            uint32_t mux = 0;

            if (get_bits(buf, 0, 4) == 0x02)
            {
                mpeg_type = 1;

                //type 1 pack headers zijn 12 bytes lang
                //we hebben 5 byte gehad, dus nog 7 nodig
                is->read(buf + 1, 7);
                mux = get_bits(buf, 41, 22);
            }
            else if (get_bits(buf, 0, 2) == 0x01)
            {
                mpeg_type = 2;

                //type 2 pack headers zijn 14 bytes lang
                //we hebben 5 bytes gehad, dus nog 9 nodig
                is->read(buf + 1, 9);
                mux = get_bits(buf, 48, 22);
            }
            
            //std::cout << uint32_t(mux) << "\r\n";
            std::cout.flush();
        }
            break;
        case 0xbd:
        {
            uint16_t len = Toolbox::readWBE(*is);
            is->ignore(11);
            uint8_t id = is->get();
            is->ignore(3);
            len -= 15;
            char buf[len];
            is->read(buf, len);
#if 0            
            if (id == 0x80)
                os->write(buf, len);
#endif
        }
            break;
        case 0xc0:
        {
            uint16_t len = Toolbox::readWBE(*is);
            is->ignore(5);
            len -= 5;
            char buf[len];
            is->read(buf, len);

            if (o.audio())
                ao.write(buf, len);
        }
            break;
        case 0xe0:
        {
            uint16_t len = Toolbox::readWBE(*is);
            is->ignore(len);
        }
            break;
        }
    }

    if (o.summary())
        for (uint16_t i = 0; i <= 0xff; ++i)
            if (count[i])
                std::cout << Toolbox::hex8(i) << ": " << count[i] << "\r\n";

    std::cout.flush();
    return 0;
}


