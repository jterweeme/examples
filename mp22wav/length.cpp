// kjmp2 example application: decodes .mp2 into .wav
// this file is public domain -- do with it whatever you want!

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cassert>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <math.h>

using std::ifstream;
using std::istream;
using std::string;
using std::cerr;
using std::cin;

static constexpr short bitrates[28] = {
    32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384,  //MPEG-1
     8, 16, 24, 32, 40, 48,  56,  64,  80,  96, 112, 128, 144, 160   //MPEG-2
};

static constexpr uint16_t sample_rates[8] = {
    44100, 48000, 32000, 0,  // MPEG-1
    22050, 24000, 16000, 0   // MPEG-2
};

class Buffer
{
    char *_buf = new char[1000];
    uint16_t _size = 1000;
    unsigned get_bit(uint16_t offset);
public:
    auto read(uint16_t offset, istream &is, uint16_t n);
    unsigned get_bits(uint16_t offset, uint16_t n);
};

auto Buffer::read(uint16_t offset, istream &is, uint16_t n)
{
    is.read(_buf + offset, n);
    return is.gcount();
}

unsigned Buffer::get_bit(uint16_t offset)
{
    unsigned mask = 7 - offset % 8;
    return (_buf[offset / 8] & 1 << mask) >> mask;
}

unsigned Buffer::get_bits(uint16_t offset, uint16_t n)
{
    unsigned ret = 0;

    for (uint16_t i = 0; i < n; ++i)
    {
        ret <<= 1;
        ret |= get_bit(i + offset);
    }

    return ret;
}

int main(int argc, char **argv)
{
    istream *is = &cin;
    int out_bytes = 0;
    Buffer _buf;
    int16_t _samples[1152 * 2];
    int16_t samples[1152 * 2];
    unsigned frameno = 0;

    while (_buf.read(0, *is, 10) == 10)
    {
        ++frameno;
        uint8_t frame0 = _buf.get_bits(0, 8);
        uint8_t frame1 = _buf.get_bits(8, 8);
        assert(frame0 == 0xff);
        assert((frame1 & 0xf6) == 0xf4);
    
        // read the rest of the header
        unsigned bit_rate_index_minus1 = _buf.get_bits(16, 4) - 1;
        assert(bit_rate_index_minus1 <= 13);    //invalid bit rate or 'free format'
        unsigned freq = _buf.get_bits(20, 2);
        assert(freq != 3);
    
        //MPEG-2
        if ((frame1 & 0x08) == 0)
            bit_rate_index_minus1 += 14;
    
        unsigned padding_bit = _buf.get_bits(22, 1);
        _buf.get_bits(28, 4);
    
        // compute the frame size
        uint32_t frame_size = 144000 * bitrates[bit_rate_index_minus1]
                   / sample_rates[freq] + padding_bit;

        _buf.read(10, *is, frame_size - 10);
    }

    cerr << frameno << "\r\n";
    return 0;
}


