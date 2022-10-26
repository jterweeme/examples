#include <cstdio>
#include <cstring>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <math.h>

struct Quantizer_spec
{
    uint16_t nlevels;
    uint8_t grouping;
    uint8_t cw_bits;
};

//static constexpr uint8_t STEREO = 0;
static constexpr uint8_t JOINT_STEREO = 1;
//static constexpr uint8_t DUAL_CHANNEL = 2;
static constexpr uint8_t MONO = 3;

static constexpr short bitrates[28] = {
    32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384,  // MPEG-1
     8, 16, 24, 32, 40, 48,  56,  64,  80,  96, 112, 128, 144, 160   // MPEG-2
};

// scale factor base values (24-bit fixed-point)
static constexpr int scf_base[3] = { 0x02000000, 0x01965FEA, 0x01428A30 };

// quantizer lookup, step 1: bitrate classes
static constexpr char quant_lut_step1[2][16] = {
    // 32, 48, 56, 64, 80, 96,112,128,160,192,224,256,320,384 <- bitrate
    {   0,  0,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2 },  // mono
    // 16, 24, 28, 32, 40, 48, 56, 64, 80, 96,112,128,160,192 <- BR / chan
    {   0,  0,  0,  0,  0,  0,  1,  1,  1,  2,  2,  2,  2,  2 }   // stereo
};

// quantizer lookup, step 2: bitrate class, sample rate -> B2 table idx, sblimit
static constexpr char QUANT_TAB_A = (27 | 64);   // Table 3-B.2a: high-rate, sblimit = 27
static constexpr char QUANT_TAB_B = (30 | 64);   // Table 3-B.2b: high-rate, sblimit = 30
static constexpr char QUANT_TAB_C =  8;         // Table 3-B.2c:  low-rate, sblimit =  8
static constexpr char QUANT_TAB_D = 12;         // Table 3-B.2d:  low-rate, sblimit = 12

static constexpr char quant_lut_step2[3][4] = {
    //   44.1 kHz,      48 kHz,      32 kHz
    { QUANT_TAB_C, QUANT_TAB_C, QUANT_TAB_D },  // 32 - 48 kbit/sec/ch
    { QUANT_TAB_A, QUANT_TAB_A, QUANT_TAB_A },  // 56 - 80 kbit/sec/ch
    { QUANT_TAB_B, QUANT_TAB_A, QUANT_TAB_B },  // 96+     kbit/sec/ch
};


// quantizer lookup, step 3: B2 table, subband -> nbal, row index
// (upper 4 bits: nbal, lower 4 bits: row index)
static constexpr char quant_lut_step3[3][32] = {
    // low-rate table (3-B.2c and 3-B.2d)
    { 0x44,0x44,                                                   // SB  0 -  1
      0x34,0x34,0x34,0x34,0x34,0x34,0x34,0x34,0x34,0x34            // SB  2 - 12
    },
    // high-rate table (3-B.2a and 3-B.2b)
    { 0x43,0x43,0x43,                                              // SB  0 -  2
      0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,                     // SB  3 - 10
      0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31, // SB 11 - 22
      0x20,0x20,0x20,0x20,0x20,0x20,0x20                           // SB 23 - 29
    },
    // MPEG-2 LSR table (B.2 in ISO 13818-3)
    { 0x45,0x45,0x45,0x45,                                         // SB  0 -  3
      0x34,0x34,0x34,0x34,0x34,0x34,0x34,                          // SB  4 - 10
      0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,           // SB 11 -
                     0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24  //       - 29
    }
};

// quantizer lookup, step 4: table row, allocation[] value -> quant table index
static constexpr char quant_lut_step4[6][16] = {
    { 0, 1, 2, 17 },
    { 0, 1, 2, 3, 4, 5, 6, 17 },
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17 },
    { 0, 1, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17 },
    { 0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 17 },
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }
};

static constexpr struct Quantizer_spec quantizer_table[17] = {
    {     3, 1,  5 },
    {     5, 1,  7 },
    {     7, 0,  3 },
    {     9, 1, 10 },
    {    15, 0,  4 },
    {    31, 0,  5 },
    {    63, 0,  6 },
    {   127, 0,  7 },
    {   255, 0,  8 },
    {   511, 0,  9 },
    {  1023, 0, 10 },
    {  2047, 0, 11 },
    {  4095, 0, 12 },
    {  8191, 0, 13 },
    { 16383, 0, 14 },
    { 32767, 0, 15 },
    { 65535, 0, 16 }  
};

// synthesis window
static constexpr int D[512] = {
0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -2, -2, -3, -3, -4, -4, -5, -6, -6, -7,
-8, -9, -10, -12, -13, -15, -16, -18, -20, -23, -25, -28, -30, -34, -37, -40,
-44, -48, -52, -57, -62, -67, -72, -78, -84, -90, -96, -103, -110, -116,
-124, -131, -138, -146, -153, -160, -168, -175, -182, -189, -195, -201,
-207, 213, 218, 222, 225, 227, 228, 228, 227, 224, 221, 215, 208, 200, 189,
177, 163, 146, 127, 106, 83, 57, 29, -1, -35, -71, -110, -152, -196, -243,
-293, -346, -400, -458, -518, -580, -644, -710, -778, -847, -918, -990,
-1063, -1136, -1209, -1282, -1355, -1427, -1497, -1566, -1633, -1697, -1758,
-1816, -1869, -1918, -1961, -2000, -2031, -2056, -2074, -2084, -2086, -2079,
-2062, 2037, 2000, 1952, 1893, 1822, 1739, 1644, 1535, 1414, 1280, 1131, 970,
794, 605, 402, 185, -44, -287, -544, -813, -1094, -1387, -1691, -2005, -2329,
-2662, -3003, -3350, -3704, -4062, -4424, -4787, -5152, -5516, -5878, -6236,
-6588, -6934, -7270, -7596, -7909, -8208, -8490, -8754, -8997, -9218, -9415,
-9584, -9726, -9837, -9915, -9958, -9965, -9934, -9862, -9749, -9591, -9388,
-9138, -8839, -8491, -8091, -7639, -7133, 6574, 5959, 5288, 4561, 3776, 2935,
2037, 1082, 70, -997, -2121, -3299, -4532, -5817, -7153, -8539, -9974, -11454,
-12979, -14547, -16154, -17798, -19477, -21188, -22928, -24693, -26481, -28288,
-30111, -31946, -33790, -35639, -37488, -39335, -41175, -43005, -44820, -46616,
-48389, -50136, -51852, -53533, -55177, -56777, -58332, -59837, -61288, -62683,
-64018, -65289, -66493, -67628, -68691, -69678, -70589, -71419, -72168, -72834,
-73414, -73907, -74312, -74629, -74855, -74991, 75038, 74992, 74856, 74630,
74313, 73908, 73415, 72835, 72169, 71420, 70590, 69679, 68692, 67629, 66494,
65290, 64019, 62684, 61289, 59838, 58333, 56778, 55178, 53534, 51853, 50137,
48390, 46617, 44821, 43006, 41176, 39336, 37489, 35640, 33791, 31947, 30112,
28289, 26482, 24694, 22929, 21189, 19478, 17799, 16155, 14548, 12980, 11455,
9975, 8540, 7154, 5818, 4533, 3300, 2122, 998, -69, -1081, -2036, -2934,
-3775, -4560, -5287, -5958, 6574, 7134, 7640, 8092, 8492, 8840, 9139, 9389,
9592, 9750, 9863, 9935, 9966, 9959, 9916, 9838, 9727, 9585, 9416, 9219, 8998,
8755, 8491, 8209, 7910, 7597, 7271, 6935, 6589, 6237, 5879, 5517, 5153, 4788,
4425, 4063, 3705, 3351, 3004, 2663, 2330, 2006, 1692, 1388, 1095, 814, 545, 288,
45, -184, -401, -604, -793, -969, -1130, -1279, -1413, -1534, -1643, -1738, -1821,
-1892, -1951, -1999, 2037, 2063, 2080, 2087, 2085, 2075, 2057, 2032, 2001, 1962,
1919, 1870, 1817, 1759, 1698, 1634, 1567, 1498, 1428, 1356, 1283, 1210, 1137,
1064, 991, 919, 848, 779, 711, 645, 581, 519, 459, 401, 347, 294, 244, 197,
153, 111, 72, 36, 2, -28, -56, -82, -105, -126, -145, -162, -176, -188, -199,
-207, -214, -220, -223, -226, -227, -227, -226, -224, -221, -217, 213, 208, 202,
196, 190, 183, 176, 169, 161, 154, 147, 139, 132, 125, 117, 111, 104, 97, 91,
85, 79, 73, 68, 63, 58, 53, 49, 45, 41, 38, 35, 31, 29, 26, 24, 21, 19, 17, 16,
14, 13, 11, 10, 9, 8, 7, 7, 6, 5, 5, 4, 4, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1
};

static constexpr uint16_t sample_rates[8] = {
    44100, 48000, 32000, 0,  // MPEG-1
    22050, 24000, 16000, 0   // MPEG-2
};

class Buffer
{
    char *_buf;
    uint16_t _size;
public:
    Buffer();
    void read(uint16_t offset, std::istream &is, uint16_t n);
    void reserve(uint16_t size);
    unsigned get_bit(uint16_t offset);
    unsigned get_bits(uint16_t offset, uint16_t n);
};

class CWavHeader
{
public:
    void write(std::ostream &os) const;
};

class Toolbox
{
public:
    static void writeWLE(std::ostream &os, uint16_t w);
    static void writeDwLE(std::ostream &os, uint32_t dw);
};

class Frame
{
private:
    Buffer _buf;
    int _N[64][32];
    int _V[2][1024];
    int _Voffs;
    int16_t _samples[1152 * 2];
    const Quantizer_spec *_read_allocation(int sb, int b2_table, unsigned &offset);
    void _read_samples(const Quantizer_spec *q, int scalefactor, int *sample, unsigned &offset);
public:
    void init();
    void read(std::istream &is);
    void decode();
    void write(std::ostream &os);
};

void Toolbox::writeWLE(std::ostream &os, uint16_t w)
{
    os.put(w >> 0 & 0xff);
    os.put(w >> 8 & 0xff);
}

void Toolbox::writeDwLE(std::ostream &os, uint32_t dw)
{
    os.put(dw >>  0 & 0xff);
    os.put(dw >>  8 & 0xff);
    os.put(dw >> 16 & 0xff);
    os.put(dw >> 24 & 0xff);
}

Buffer::Buffer()
{
    _buf = new char[1000];
    _size = 1000;
}

void Buffer::read(uint16_t offset, std::istream &is, uint16_t n)
{
    is.read(_buf + offset, n);
    
    if (is.gcount() != n)
        throw "cannot read";
}

//buffer moet minstens [size] grootte worden
void Buffer::reserve(uint16_t size)
{
    //buffer is al groot genoeg
    if (size <= _size)
        return;

    throw "not implemented";
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

void CWavHeader::write(std::ostream &os) const
{
    os << "RIFF";
    Toolbox::writeDwLE(os, 0xffffffff);
    os << "WAVE";
    os << "fmt ";
    Toolbox::writeDwLE(os, 16);
    Toolbox::writeWLE(os, 1);
    Toolbox::writeWLE(os, 2);
    Toolbox::writeDwLE(os, 44100);
    Toolbox::writeDwLE(os, 44100 << 2);
    Toolbox::writeWLE(os, 4);
    Toolbox::writeWLE(os, 16);
    os << "data";
    Toolbox::writeDwLE(os, 0xffffffff);
}

void Frame::init()
{
    for (int i = 0;  i < 64;  ++i)
        for (int j = 0;  j < 32;  ++j)
            _N[i][j] = int(256.0 * cos(((16 + i) * ((j << 1) + 1)) * 0.0490873852123405));

    for (int i = 0;  i < 2;  ++i)
        for (int j = 1023;  j >= 0;  --j)
            _V[i][j] = 0;

    _Voffs = 0;
}

void Frame::_read_samples(const Quantizer_spec *q, int scalefactor, int *sample, unsigned &offset)
{
    if (!q)
    {
        // no bits allocated for this subband
        sample[0] = sample[1] = sample[2] = 0;
        return;
    }

    // resolve scalefactor
    if (scalefactor == 63)
    {
        scalefactor = 0;
    }
    else
    {
        int xadj = scalefactor / 3;
        scalefactor = (scf_base[scalefactor % 3] + ((1 << xadj) >> 1)) >> xadj;
    }

    // decode samples
    int adj = q->nlevels;

    if (q->grouping)
    {
        // decode grouped samples
        int val = _buf.get_bits(offset, q->cw_bits);
        offset += q->cw_bits;
        sample[0] = val % adj;
        val /= adj;
        sample[1] = val % adj;
        sample[2] = val / adj;
    }
    else
    {
        // decode direct samples
        for (int idx = 0;  idx < 3;  ++idx)
        {
            sample[idx] = _buf.get_bits(offset, q->cw_bits);
            offset += q->cw_bits;
        }
    }

    // postmultiply samples
    int scale = 65536 / (adj + 1);
    adj = (adj + 1 >> 1) - 1;

    for (int idx = 0;  idx < 3;  ++idx)
    {
        // step 1: renormalization to [-1..1]
        int val = (adj - sample[idx]) * scale;
        // step 2: apply scalefactor
        sample[idx] = ( val * (scalefactor >> 12)                  // upper part
                    + ((val * (scalefactor & 4095) + 2048) >> 12)) // lower part
                    >> 12;  // scale adjust
    }
}

void Frame::read(std::istream &is)
{
    _buf.read(0, is, 3);
    unsigned bit_rate_index_minus1 = _buf.get_bits(16, 4) - 1;
    unsigned sampling_freq = _buf.get_bits(20, 2);
    unsigned padding = _buf.get_bits(22, 1);

    uint32_t frame_size = 144000 * bitrates[bit_rate_index_minus1]
                    / sample_rates[sampling_freq] + padding;

    _buf.read(3, is, frame_size - 3);
    
}

const Quantizer_spec *Frame::_read_allocation(int sb, int b2_table, unsigned &offset)
{
    //std::cerr << sb << " " << b2_table << " " << offset << "\r\n";
    int table_idx = quant_lut_step3[b2_table][sb];
    unsigned bits = table_idx >> 4;
    table_idx = quant_lut_step4[table_idx & 15][_buf.get_bits(offset, bits)];
    offset += bits;
    return table_idx ? (&quantizer_table[table_idx - 1]) : 0;
}

static void printsf(int sf[2][32][3])
{
    for (int i = 0; i < 32; ++i)
    {
        std::cerr << sf[0][i][0] << " " <<
                     sf[0][i][1] << " " <<
                     sf[0][i][2] << " " <<
                     sf[1][i][0] << " " <<
                     sf[1][i][1] << " " <<
                     sf[1][i][2] << "\r\n";
    }
}

void Frame::decode()
{
    int bound = _buf.get_bits(26, 2) + 1 << 2;
    const Quantizer_spec *alloc[2][32];
    unsigned mode = _buf.get_bits(24, 2);
    int table_idx = mode == MONO ? 0 : 1;
    unsigned freq = _buf.get_bits(20, 2);
    unsigned bit_rate_index_minus1 = _buf.get_bits(16, 4) - 1;
    table_idx = quant_lut_step1[table_idx][bit_rate_index_minus1];
    table_idx = quant_lut_step2[table_idx][freq];
    int sblimit = table_idx & 63;
    table_idx >>= 6;
    bound = std::min(bound, sblimit);
    unsigned offset = 32;

    // read the allocation information
    for (int sb = 0; sb < bound; ++sb)
        for (int ch = 0; ch < 2; ++ch)
            alloc[ch][sb] = _read_allocation(sb, table_idx, offset);

    for (int sb = bound;  sb < sblimit;  ++sb)
        alloc[0][sb] = alloc[1][sb] = _read_allocation(sb, table_idx, offset);
    
    // read scale factor selector information
    int nch = (mode == MONO) ? 1 : 2;
    int scfsi[2][32];

    for (int sb = 0;  sb < sblimit;  ++sb)
    {
        for (int ch = 0;  ch < nch;  ++ch)
        {
            if (alloc[ch][sb])
            {
                scfsi[ch][sb] = _buf.get_bits(offset, 2);
                offset += 2;
            }
        }

        if (mode == MONO)
            scfsi[1][sb] = scfsi[0][sb];
    }

    int sf[2][32][3];

    // read scale factors
    for (int sb = 0;  sb < sblimit;  ++sb)
    {
        for (int ch = 0;  ch < nch;  ++ch)
        {
            if (alloc[ch][sb])
            {
                switch (scfsi[ch][sb])
                {
                case 0:
                    sf[ch][sb][0] = _buf.get_bits(offset + 0, 6);
                    sf[ch][sb][1] = _buf.get_bits(offset + 6, 6);
                    sf[ch][sb][2] = _buf.get_bits(offset + 12, 6);
                    offset += 18;
                    break;
                case 1:
                    sf[ch][sb][0] =
                    sf[ch][sb][1] = _buf.get_bits(offset + 0, 6);
                    sf[ch][sb][2] = _buf.get_bits(offset + 6, 6);
                    offset += 12;
                    break;
                case 2:
                    sf[ch][sb][0] =
                    sf[ch][sb][1] =
                    sf[ch][sb][2] = _buf.get_bits(offset, 6);
                    offset += 6;
                    break;
                case 3:
                    sf[ch][sb][0] = _buf.get_bits(offset + 0, 6);
                    sf[ch][sb][1] =
                    sf[ch][sb][2] = _buf.get_bits(offset + 6, 6);
                    offset += 12;
                    break;
                }
            }
        }
    }

    //printsf(sf);
    int sample[2][32][3];
    int16_t *pcm = _samples;

    // coefficient input and reconstruction
    for (int part = 0;  part < 3;  ++part)
    {
        for (int gr = 0;  gr < 4;  ++gr)
        {
            // read the samples
            for (int sb = 0;  sb < bound;  ++sb)
                for (int ch = 0;  ch < 2;  ++ch)
                    _read_samples(alloc[ch][sb], sf[ch][sb][part], sample[ch][sb], offset);

            for (int sb = bound;  sb < sblimit;  ++sb)
            {
                _read_samples(alloc[0][sb], sf[0][sb][part], sample[0][sb], offset);

                for (int idx = 0;  idx < 3;  ++idx)
                    sample[1][sb][idx] = sample[0][sb][idx];
            }

            for (int ch = 0;  ch < 2;  ++ch)
               for (int sb = sblimit;  sb < 32;  ++sb)
                    for (int idx = 0;  idx < 3;  ++idx)
                        sample[ch][sb][idx] = 0;

            // synthesis loop
            for (int idx = 0;  idx < 3;  ++idx)
            {
                // shifting step
                _Voffs = table_idx = (_Voffs - 64) & 1023;

                for (int ch = 0; ch < 2; ++ch)
                {
                    // matrixing
                    for (int i = 0; i < 64; ++i)
                    {
                        int sum = 0;

                        for (int j = 0; j < 32; ++j)
                            sum += _N[i][j] * sample[ch][j][idx];  // 8b*15b=23b

                        // intermediate value is 28 bit (23 + 5), clamp to 14b
                        _V[ch][table_idx + i] = sum + 8192 >> 14;
                    }

                    int U[512];

                    // construction of U
                    for (int i = 0;  i < 8;  ++i)
                    {
                        for (int j = 0;  j < 32;  ++j)
                        {
                            U[(i<<6) + j]      = _V[ch][(table_idx + (i<<7) + j     ) & 1023];
                            U[(i<<6) + j + 32] = _V[ch][(table_idx + (i<<7) + j + 96) & 1023];
                        }
                    }
                    // apply window
                    for (int i = 0;  i < 512;  ++i)
                        U[i] = U[i] * D[i] + 32 >> 6;

                    // output samples
                    for (int j = 0; j < 32; ++j)
                    {
                        int sum = 0;

                        for (int i = 0;  i < 16;  ++i)
                            sum -= U[(i << 5) + j];

                        sum = (sum + 8) >> 4;
                        sum = std::max(sum, -32768);
                        sum = std::min(sum, 32767);
                        pcm[idx << 6 | j << 1 | ch] = int16_t(sum);
                    }
                } // end of synthesis channel loop
            } // end of synthesis sub-block loop

            // adjust PCM output pointer: decoded 3 * 32 = 96 stereo samples
            pcm += 192;

        } // decoding of the granule finished
    }

}

void Frame::write(std::ostream &os)
{
    os.write((const char *)(_samples), 1152 * 4);
}

int main()
{
    Frame f;
    f.init();
    uint32_t frame_n = 0;
    CWavHeader h;

    while (std::cin)
    {
        f.read(std::cin);
        f.decode();

        if (frame_n == 0)
        {
            h.write(std::cout);
            ++frame_n;
        }

        f.write(std::cout);
    }

}


