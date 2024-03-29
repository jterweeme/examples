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
using std::ostream;
using std::string;
using std::clamp;
using std::endl;
using std::cerr;
using std::cout;
using std::cin;
using std::min;
using std::ios;

struct Quantizer_spec
{
    uint16_t nlevels;
    uint8_t grouping;
    uint8_t cw_bits;
};

static constexpr short bitrates[28] = {
    32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384,  //MPEG-1
     8, 16, 24, 32, 40, 48,  56,  64,  80,  96, 112, 128, 144, 160   //MPEG-2
};

//scale factor base values (24-bit fixed-point)
static constexpr int scf_base[3] = { 0x02000000, 0x01965FEA, 0x01428A30 };

//quantizer lookup, step 1: bitrate classes
static constexpr char quant_lut_step1[2][16] = {
    //32, 48, 56, 64, 80, 96,112,128,160,192,224,256,320,384 <- bitrate
    {  0,  0,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2 },  // mono
    //16, 24, 28, 32, 40, 48, 56, 64, 80, 96,112,128,160,192 <- BR / chan
    {  0,  0,  0,  0,  0,  0,  1,  1,  1,  2,  2,  2,  2,  2 }   // stereo
};

//quantizer lookup, step 2: bitrate class, sample rate -> B2 table idx, sblimit
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

//quantizer lookup, step 3: B2 table, subband -> nbal, row index
//(upper 4 bits: nbal, lower 4 bits: row index)
static constexpr char quant_lut_step3[3][32] = {
    //low-rate table (3-B.2c and 3-B.2d)
    { 68,68,52,52,52,52,52,52,52,52,52,52},
    //high-rate table (3-B.2a and 3-B.2b)
    { 67,67,67,
      66,66,66,66,66,66,66,66,
      49,49,49,49,49,49,49,49,49,49,49,49,
      32,32,32,32,32,32,32
    },
    //MPEG-2 LSR table (B.2 in ISO 13818-3)
    { 69,69,69,69,
      52,52,52,52,52,52,52,
      36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36
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
    {     3, 1,  5 },  //  1
    {     5, 1,  7 },  //  2
    {     7, 0,  3 },  //  3
    {     9, 1, 10 },  //  4
    {    15, 0,  4 },  //  5
    {    31, 0,  5 },  //  6
    {    63, 0,  6 },  //  7
    {   127, 0,  7 },  //  8
    {   255, 0,  8 },  //  9
    {   511, 0,  9 },  // 10
    {  1023, 0, 10 },  // 11
    {  2047, 0, 11 },  // 12
    {  4095, 0, 12 },  // 13
    {  8191, 0, 13 },  // 14
    { 16383, 0, 14 },  // 15
    { 32767, 0, 15 },  // 16
    { 65535, 0, 16 }   // 17
};

//synthesis window
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

static constexpr uint8_t STEREO = 0; //not used
static constexpr uint8_t JOINT_STEREO = 1;
static constexpr uint8_t DUAL_CHANNEL = 2; //not used
static constexpr uint8_t MONO = 3;

class Buffer
{
    //wat is de maximale frame size?
    char *_buf = new char[1000];

    unsigned get_bit(uint16_t offset)
    {
        unsigned mask = 7 - offset % 8;
        return (_buf[offset / 8] & 1 << mask) >> mask;
    }
public:
    auto read(uint16_t offset, istream &is, uint16_t n)
    {
        is.read(_buf + offset, n);
        return is.gcount();
    }

    unsigned get_bits(uint16_t offset, uint16_t n)
    {
        unsigned ret = 0;

        for (uint16_t i = 0; i < n; ++i)
        {
            ret <<= 1;
            ret |= get_bit(i + offset);
        }

        return ret;
    }
};

struct Toolbox
{
    static constexpr void writeWLE(char *buf, uint16_t w) { *(uint16_t *)buf = w;}
    static constexpr void writeDwLE(char *buf, uint32_t dw) { *(uint32_t *)buf = dw; }
};

class COptions
{
private:
    bool _stdinput = false;
    bool _stdoutput = false;
    string _ifn;
    string _ofn;
public:
    bool stdinput() const { return _stdinput; }
    bool stdoutput() const { return _stdoutput; }
    string ifn() const { return _ifn; }
    string ofn() const { return _ofn; }

    void parse(int argc, char **argv)
    {
        if (argc < 2)
        {
            _stdinput = true;
            _stdoutput = true;
        }
        else if (argc == 2)
        {
            _ifn = argv[1];
            _stdoutput = true;
        }
        else
        {
            _ifn = argv[1];
            _ofn = argv[2];
        }
    }
};

class CWavHeader
{
private:
    int _rate;
    uint32_t _filesize = 0;
    uint32_t _datasize = 0;
public:
    void rate(int val) { _rate = val; }
    void datasize(uint32_t n) { _datasize = n; }

    void write(FILE *fp) const
    {
        Toolbox t;
        char header[44];
        strncpy(header + 0, "RIFF", 4);
        t.writeDwLE(header + 4, _datasize + 36);  //filesize - 8 bytes
        strncpy(header + 8, "WAVE", 4);
        strncpy(header + 12, "fmt ", 4);
        t.writeDwLE(header + 16, 16);
        t.writeWLE(header + 20, 1);
        t.writeWLE(header + 22, 2);
        t.writeDwLE(header + 24, _rate);
        t.writeDwLE(header + 28, _rate << 2);
        t.writeWLE(header + 32, 4);
        t.writeWLE(header + 34, 16);
        strncpy(header + 36, "data", 4);
        t.writeDwLE(header + 40, _datasize);
    
        //write wav header to file
        fwrite((const void*) header, 44, 1, fp);
    }
};

int main(int argc, char **argv)
{
#ifdef _WIN32
    setmode(fileno(stdin), O_BINARY);
    setmode(fileno(stdout), O_BINARY);
#endif
    COptions opts;
    opts.parse(argc, argv);
    FILE *fout = stdout;
    ifstream ifs;
    istream *is = &cin;
    //ostream *os = &cout;

    if (opts.stdinput() == false)
    {
        ifs.open(opts.ifn());
        is = &ifs;
    }

    if (opts.stdoutput() == false)
        fout = fopen(opts.ofn().c_str(), "wb");

    CWavHeader h;
    int out_bytes = 0;
    Buffer _buf;
    int _Voffs;
    int _N[64][32];
    int _V[2][1024];

    for (int i = 0;  i < 64;  ++i)
        for (int j = 0;  j < 32;  ++j)                                  //pi/64
            _N[i][j] = int(256.0 * cos(((16 + i) * ((j << 1) + 1)) * 0.0490873852123405));

    for (int i = 0;  i < 2;  ++i)
        for (int j = 1023;  j >= 0;  --j)
            _V[i][j] = 0;

    _Voffs = 0;
    int samplerate;
    int16_t samples[1152 * 2];

    //Ga eerst de lengte opnemen, zodat dit vantevoren kan worden weggeschreven
    //naar de WAV header. Dit kan niet met input van stdin, want deze kan niet
    //teruggespoeld worden naar begin
    if (opts.stdinput() == false)
    {
        unsigned frames = 0;

        while (_buf.read(0, *is, 10) == 10)
        {
            ++frames;
            uint8_t frame0 = _buf.get_bits(0, 8);
            uint8_t frame1 = _buf.get_bits(8, 8);
            assert(frame0 == 0xff);
            assert((frame1 & 0xf6) == 0xf4);
    
            //read the rest of the header
            unsigned bit_rate_index_minus1 = _buf.get_bits(16, 4) - 1;
            assert(bit_rate_index_minus1 <= 13);    //invalid bit rate or 'free format'
            unsigned freq = _buf.get_bits(20, 2);
            assert(freq != 3);
    
            //MPEG-2
            if ((frame1 & 0x08) == 0)
                bit_rate_index_minus1 += 14;
    
            unsigned padding_bit = _buf.get_bits(22, 1);
            _buf.get_bits(28, 4);
    
            //compute the frame size
            uint32_t frame_size = 144000 * bitrates[bit_rate_index_minus1]
                       / sample_rates[freq] + padding_bit;
    
            _buf.read(10, *is, frame_size - 10);
        }

        h.datasize(frames * 1152 * 4);
        is->clear();
        is->seekg(0, ios::beg);
    }

    while (_buf.read(0, *is, 10) == 10)
    {
        int16_t *pcm = samples;
        uint8_t frame0 = _buf.get_bits(0, 8);
        assert(frame0 == 0xff);
        uint8_t frame1 = _buf.get_bits(8, 8);
        assert((frame1 & 0xf6) == 0xf4);
        uint8_t frame2 = _buf.get_bits(16, 8);

        samplerate = sample_rates[(((frame1 & 0x08) >> 1) ^ 4)  // MPEG-1/2 switch
                      + ((frame2 >> 2) & 3)];         // actual rate
    
        // read the rest of the header
        unsigned bit_rate_index_minus1 = _buf.get_bits(16, 4) - 1;
        assert(bit_rate_index_minus1 <= 13);    //invalid bit rate or 'free format'
        unsigned freq = _buf.get_bits(20, 2);
        assert(freq != 3);
    
        //MPEG-2
        if ((frame1 & 0x08) == 0)
        {  
            freq += 4;
            bit_rate_index_minus1 += 14;
        }
    
        unsigned padding_bit = _buf.get_bits(22, 1);
        unsigned mode = _buf.get_bits(24, 2);
        int sblimit, table_idx;
        int bound = _buf.get_bits(26, 2) + 1 << 2;
    
        if (mode != JOINT_STEREO)
            bound = mode == MONO ? 0 : 32;
    
        _buf.get_bits(28, 4);
        unsigned offset = 32;
    
        if ((frame1 & 1) == 0)
        {
            _buf.get_bits(offset, 16);
            offset += 16;
        }
    
        // compute the frame size
        uint32_t frame_size = 144000 * bitrates[bit_rate_index_minus1]
                   / sample_rates[freq] + padding_bit;

        _buf.read(10, *is, frame_size - 10);

        // prepare the quantizer table lookups
        if (freq & 4)
        {
            // MPEG-2 (LSR)
            table_idx = 2;
            sblimit = 30;
        }
        else
        {
            // MPEG-1
            table_idx = (mode == MONO) ? 0 : 1;
            table_idx = quant_lut_step1[table_idx][bit_rate_index_minus1];
            table_idx = quant_lut_step2[table_idx][freq];
            sblimit = table_idx & 63;
            table_idx >>= 6;
        }
    
        bound = min(bound, sblimit);
        const Quantizer_spec *alloc[2][32];
    
        // read the allocation information
        for (int sb = 0; sb < sblimit; ++sb)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                int xtable_idx = quant_lut_step3[table_idx][sb];
                unsigned n = xtable_idx >> 4;
                xtable_idx = quant_lut_step4[xtable_idx & 15][_buf.get_bits(offset, n)];
                offset += n;
                const Quantizer_spec *foo = xtable_idx ? &quantizer_table[xtable_idx - 1] : 0;
                alloc[ch][sb] = foo;
    
                if (sb >= bound)
                {
                    alloc[++ch][sb] = foo;
                    break;
                }
            }
        }
    
        // read scale factor selector information
        int nch = (mode == MONO) ? 1 : 2;
        int scfsi[2][32];
    
        for (int sb = 0; sb < sblimit; ++sb)
        {
            for (int ch = 0; ch < nch; ++ch)
                if (alloc[ch][sb])
                    scfsi[ch][sb] = _buf.get_bits(offset, 2), offset += 2;
    
            if (mode == MONO)
                scfsi[1][sb] = scfsi[0][sb];
        }
    
        int sf[2][32][3];
    
        // read scale factors
        for (int sb = 0; sb < sblimit; ++sb)
        {
            for (int ch = 0; ch < nch; ++ch)
            {
                if (alloc[ch][sb])
                {
                    switch (scfsi[ch][sb])
                    {
                    case 0:
                        sf[ch][sb][0] = _buf.get_bits(offset, 6);
                        sf[ch][sb][1] = _buf.get_bits(offset + 6, 6);
                        sf[ch][sb][2] = _buf.get_bits(offset + 12, 6);
                        offset += 18;
                        break;
                    case 1:
                        sf[ch][sb][0] =
                        sf[ch][sb][1] = _buf.get_bits(offset, 6);
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
                        sf[ch][sb][0] = _buf.get_bits(offset, 6);
                        sf[ch][sb][1] =
                        sf[ch][sb][2] = _buf.get_bits(offset + 6, 6);
                        offset += 12;
                        break;
                    }
                }
            }
    
            if (mode == MONO)
                for (int part = 0;  part < 3;  ++part)
                    sf[1][sb][part] = sf[0][sb][part];
        }
    
        int sample[2][32][3];
    
        // coefficient input and reconstruction
        for (int part = 0; part < 3; ++part)
        {
            for (int gr = 0; gr < 4; ++gr)
            {
                // read the samples
                for (int sb = 0; sb < sblimit; ++sb)
                {
                    int foo = sb < bound ? 2 : 1;
    
                    for (int ch = 0; ch < foo; ++ch)
                    {
                        const Quantizer_spec *q = alloc[ch][sb];
                        int scalefactor = sf[ch][sb][part];
                        //int *sample = sample[ch][sb];
    
                        if (!q)
                        {
                            // no bits allocated for this subband
                            sample[ch][sb][0] = sample[ch][sb][1] = sample[ch][sb][2] = 0;
                            continue;
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
                            sample[ch][sb][0] = val % adj;
                            val /= adj;
                            sample[ch][sb][1] = val % adj;
                            sample[ch][sb][2] = val / adj;
                        }
                        else
                        {
                            // decode direct samples
                            for (int idx = 0;  idx < 3;  ++idx)
                            {
                                sample[ch][sb][idx] = _buf.get_bits(offset, q->cw_bits);
                                offset += q->cw_bits;
                            }
                        }
                    
                        // postmultiply samples
                        int scale = 65536 / (adj + 1);
                        adj = (adj + 1 >> 1) - 1;
                    
                        for (int idx = 0;  idx < 3;  ++idx)
                        {
                            // step 1: renormalization to [-1..1]
                            int val = (adj - sample[ch][sb][idx]) * scale;
                            // step 2: apply scalefactor
                            sample[ch][sb][idx] = ( val * (scalefactor >> 12)      // upper part
                                   + ((val * (scalefactor & 4095) + 2048) >> 12)) // lower part
                                     >> 12;  // scale adjust
                        }
    
                        if (sb >= bound)
                            for (int idx = 0; idx < 3; ++idx)
                                sample[1][sb][idx] = sample[0][sb][idx];
                    }
                }
    
                for (int ch = 0; ch < 2; ++ch)
                   for (int sb = sblimit; sb < 32; ++sb)
                        for (int idx = 0; idx < 3; ++idx)
                            sample[ch][sb][idx] = 0;
    
                // synthesis loop
                for (int idx = 0; idx < 3; ++idx)
                {
                    // shifting step
                    _Voffs = table_idx = _Voffs - 64 & 1023;
    
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
                        for (int i = 0; i < 8; ++i)
                        {
                            for (int j = 0; j < 32; ++j)
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
    
                            for (int i = 0; i < 16; ++i)
                                sum -= U[(i << 5) + j];
    
                            sum = clamp(sum + 8 >> 4, -32768, 32767);
                            pcm[idx << 6 | j << 1 | ch] = int16_t(sum);
                        }
                    } // end of synthesis channel loop
                } // end of synthesis sub-block loop
    
                // adjust PCM output pointer: decoded 3 * 32 = 96 stereo samples
                pcm += 192;
    
            } // decoding of the granule finished
        }

        //schrijf wav header voor eerste frame
        if (out_bytes == 0)
        {
            h.rate(samplerate);
            h.write(fout);
        }

        fwrite((const void*)samples, 1, 1152 * 4, fout);
        //os.write((const void *)samples, 1152 * 4);
        out_bytes += 1152 * 4;
    }

    if (fout != stdout)
    {
        h.datasize(out_bytes);

        //rewrite WAV header
        fseek(fout, 0, SEEK_SET);
        h.write(fout);
    }

    fflush(fout);
    fclose(fout);
    return 0;
}


