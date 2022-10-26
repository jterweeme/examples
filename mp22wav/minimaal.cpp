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

static constexpr uint8_t JOINT_STEREO = 1;
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
    44100, 48000, 32000, 0, 22050, 24000, 16000, 0
};

class BitBuffer
{
private:
    int _bit_window;
    int bits_in_window;
    int show_bits(int bit_count) { return _bit_window >> 24 - bit_count; }
    uint64_t _counter2 = 0;
    FILE *_fin;
public:
    BitBuffer(FILE *fin);
    bool peek() { return bits_in_window > 0; }
    int get_bits(int bit_count);
    uint64_t counter2() const { return _counter2; }
    void reset_counter() { _counter2 = 0; }
};

class Decoder
{
private:

    int Voffs;
    int N[64][32];  // N[i][j] as 8-bit fixed-point
    int _V[2][1024];
    int initialized = 0;
    void read_samples(const Quantizer_spec *q, int scalefactor, int *sample, BitBuffer &b);
    static const Quantizer_spec *read_allocation(int sb, int b2_table, BitBuffer &b);
public:
    void kjmp2_init();
    uint32_t kjmp2_decode_frame(BitBuffer &b, int16_t *pcm, int &samplerate);
};

class Toolbox
{
public:
    static char hex4(uint8_t n);
    static std::string hex8(uint8_t b);
    static void writeWLE(char *buf, uint16_t w);
    static void writeDwLE(char *buf, uint32_t dw);
};

class COptions
{
private:
    bool _stdinput = false;
    bool _stdoutput = false;
    std::string _ifn;
    std::string _ofn;
public:
    void parse(int argc, char **argv);
    bool stdinput() const { return _stdinput; }
    bool stdoutput() const { return _stdoutput; }
    std::string ifn() const { return _ifn; }
    std::string ofn() const { return _ofn; }
};

class CWavHeader
{
private:
    int _rate;
    uint32_t _filesize = 0;
public:
    void rate(int val) { _rate = val; }
    void filesize(uint32_t val) { _filesize = val; }
    void write(FILE *fp) const;
};

class CMain
{
private:
    static constexpr uint32_t KJMP2_SAMPLES_PER_FRAME = 1152;
public:
    int run(FILE *fin, FILE *outfile);
};

void CWavHeader::write(FILE *fp) const
{
    Toolbox t;
    uint8_t header[44];
    strncpy((char *)header + 0, "RIFF", 4);
    t.writeDwLE((char *)(header + 4), _filesize - 36);
    strncpy((char *)header + 8, "WAVE", 4);
    strncpy((char *)header + 12, "fmt ", 4);
    t.writeDwLE((char *)(header + 16), 16);
    t.writeWLE((char *)(header + 20), 1);
    t.writeWLE((char *)(header + 22), 2);
    t.writeDwLE((char *)(header + 24), _rate);
    t.writeDwLE((char *)(header + 28), _rate << 2);
    t.writeWLE((char *)(header + 32), 4);
    t.writeWLE((char *)(header + 34), 16);
    strncpy((char *)header + 36, "data", 4);
    t.writeDwLE((char *)(header + 40), _filesize);

    //write wav header to file
    fwrite((const void*) header, 44, 1, fp);
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

void COptions::parse(int argc, char **argv)
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

int main(int argc, char **argv)
{
    COptions opts;
    opts.parse(argc, argv);
    CMain inst;
    FILE *fin, *fout;

    if (opts.stdinput())
    {
        fin = stdin;
#ifdef _WIN32
        setmode(fileno(stdin), O_BINARY);
#endif
    }
    else
    {
        fin = fopen(opts.ifn().c_str(), "rb");
    }

    if (opts.stdoutput())
    {
        fout = stdout;
    }
    else
    {
        fout = fopen(opts.ofn().c_str(), "wb");
    }

    int ret = inst.run(fin, fout);
    return ret;
}

int CMain::run(FILE *fin, FILE *fout)
{
    Decoder d;
#ifdef _WIN32
    setmode(fileno(stdout), O_BINARY);
#endif
    if (!fout)
    {
        fprintf(stderr, "Could not open output file %s!\n", "out.wav");
        return 1;
    }
    CWavHeader h;
    int out_bytes = 0;
    d.kjmp2_init();
    BitBuffer b(fin);
    int samplerate = 0;
    while (b.peek())
    {
        int16_t samples[KJMP2_SAMPLES_PER_FRAME * 2];
        int bytes = d.kjmp2_decode_frame(b, samples, samplerate);
        if (out_bytes == 0)
        {   h.rate(samplerate);
            h.write(fout);
        }

        out_bytes += (int) fwrite((const void*)samples, 1, KJMP2_SAMPLES_PER_FRAME * 4, fout);
    }

    if (fout != stdout)
    {
        h.filesize(out_bytes + 36);

        //rewrite WAV header
        fseek(fout, 0, SEEK_SET);
        h.write(fout);
    }

    fflush(fout);
    fclose(fout);
    fclose(fin);
    fprintf(stderr, "Done.\n");
    return 0;
}

BitBuffer::BitBuffer(FILE *fin) : _fin(fin)
{
    _bit_window = fgetc(fin) << 16;
    bits_in_window = 8;
}

int BitBuffer::get_bits(int bit_count)
{
    _counter2 += bit_count;
    int result = show_bits(bit_count);
    _bit_window = (_bit_window << bit_count) & 0xFFFFFF;
    bits_in_window -= bit_count;

    while (bits_in_window < 16)
    {
        _bit_window |= fgetc(_fin) << (16 - bits_in_window);
        bits_in_window += 8;
    }

    return result;
}

// kjmp2_init: This function must be called once to initialize each kjmp2
// decoder instance.
void Decoder::kjmp2_init()
{   for (int i = 0;  i < 64;  ++i)
        for (int j = 0;  j < 32;  ++j)
            N[i][j] = int(256.0 * cos(((16 + i) * ((j << 1) + 1)) * 0.0490873852123405));
    for (int i = 0;  i < 2;  ++i)
        for (int j = 1023;  j >= 0;  --j)
            _V[i][j] = 0;
    Voffs = 0;
}

const Quantizer_spec *Decoder::read_allocation(int sb, int b2_table, BitBuffer &b)
{
    std::cerr << sb << " " << b2_table << "\r\n";
    int table_idx = quant_lut_step3[b2_table][sb];
    table_idx = quant_lut_step4[table_idx & 15][b.get_bits(table_idx >> 4)];
    return table_idx ? (&quantizer_table[table_idx - 1]) : 0;
}


void Decoder::read_samples(const Quantizer_spec *q, int scalefactor, int *sample, BitBuffer &b)
{
    if (!q)
    {
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

    int adj = q->nlevels;

    if (q->grouping)
    {   int val = b.get_bits(q->cw_bits);
        sample[0] = val % adj;
        val /= adj;
        sample[1] = val % adj;
        sample[2] = val / adj;
    }
    else
    {   for (int idx = 0;  idx < 3;  ++idx)
            sample[idx] = b.get_bits(q->cw_bits);
    }

    // postmultiply samples
    int scale = 65536 / (adj + 1);
    adj = (adj + 1 >> 1) - 1;

    for (int idx = 0;  idx < 3;  ++idx)
    {
        int val = (adj - sample[idx]) * scale;
        // step 2: apply scalefactor
        sample[idx] = ( val * (scalefactor >> 12)                  // upper part
                    + ((val * (scalefactor & 4095) + 2048) >> 12)) // lower part
                    >> 12;  // scale adjust
    }
}

void printsf(int sf[2][32][3])
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

uint32_t
Decoder::kjmp2_decode_frame(BitBuffer &b, int16_t *pcm, int &samplerate)
{   b.reset_counter();
    uint8_t frame0 = b.get_bits(8);
    uint8_t frame1 = b.get_bits(8);
    if ((frame0 != 0xFF) || ((frame1 & 0xF6) != 0xF4))
        throw "invalid magic";
    samplerate = 44100;
    unsigned bit_rate_index_minus1 = b.get_bits(4) - 1;
    if (bit_rate_index_minus1 > 13)
        return 0;  // invalid bit rate or 'free format'
    unsigned freq = b.get_bits(2);
    if (freq == 3)
        return 0;
    unsigned padding = b.get_bits(1);
    b.get_bits(1);  // discard private_bit
    unsigned mode = b.get_bits(2);
    int bound = b.get_bits(2) + 1 << 2;
    if (mode != JOINT_STEREO)
        bound = mode == MONO ? 0 : 32;
    b.get_bits(4);
    if ((frame1 & 1) == 0)
    {
        std::cerr << "CRC" << "\r\n";
        std::cerr.flush();
        b.get_bits(16);
    }
    uint32_t frame_size = 144000 * bitrates[bit_rate_index_minus1]
               / sample_rates[freq] + padding;
    int table_idx = (mode == MONO) ? 0 : 1;
    table_idx = quant_lut_step1[table_idx][bit_rate_index_minus1];
    table_idx = quant_lut_step2[table_idx][freq];
    int sblimit = table_idx & 63;
    table_idx >>= 6;
    bound = std::min(bound, sblimit);
    const Quantizer_spec *alloc[2][32];
    for (int sb = 0;  sb < bound;  ++sb)
        for (int ch = 0;  ch < 2;  ++ch)
            alloc[ch][sb] = read_allocation(sb, table_idx, b);
    for (int sb = bound;  sb < sblimit;  ++sb)
        alloc[0][sb] = alloc[1][sb] = read_allocation(sb, table_idx, b);
    int scfsi[2][32];
    for (int sb = 0;  sb < sblimit;  ++sb)
        for (int ch = 0;  ch < 2;  ++ch)
            if (alloc[ch][sb])
                scfsi[ch][sb] = b.get_bits(2);
    int sf[2][32][3];
    for (int sb = 0;  sb < sblimit;  ++sb)
    {   for (int ch = 0;  ch < 2;  ++ch)
        {   if (alloc[ch][sb])
            {
                switch (scfsi[ch][sb])
                {
                case 0:
                    sf[ch][sb][0] = b.get_bits(6);
                    sf[ch][sb][1] = b.get_bits(6);
                    sf[ch][sb][2] = b.get_bits(6);
                    break;
                case 1:
                    sf[ch][sb][0] =
                    sf[ch][sb][1] = b.get_bits(6);
                    sf[ch][sb][2] = b.get_bits(6);
                    break;
                case 2:
                    sf[ch][sb][0] =
                    sf[ch][sb][1] =
                    sf[ch][sb][2] = b.get_bits(6);
                    break;
                case 3:
                    sf[ch][sb][0] = b.get_bits(6);
                    sf[ch][sb][1] =
                    sf[ch][sb][2] = b.get_bits(6);
                    break;
                }
            }
        }
    }
    //printsf(sf);
    int sample[2][32][3] = {0};
    for (int part = 0;  part < 3;  ++part)
    {   for (int gr = 0;  gr < 4;  ++gr)
        {   for (int sb = 0;  sb < bound;  ++sb)
                for (int ch = 0;  ch < 2;  ++ch)
                    read_samples(alloc[ch][sb], sf[ch][sb][part], &sample[ch][sb][0], b);
            for (int sb = bound;  sb < sblimit;  ++sb)
            {   read_samples(alloc[0][sb], sf[0][sb][part], &sample[0][sb][0], b);
                for (int idx = 0;  idx < 3;  ++idx)
                    sample[1][sb][idx] = sample[0][sb][idx];
            }
            for (int idx = 0;  idx < 3;  ++idx)
            {   Voffs = table_idx = (Voffs - 64) & 1023;
                for (int ch = 0;  ch < 2;  ++ch)
                {   for (int i = 0;  i < 64;  ++i)
                    {   int sum = 0;
                        for (int j = 0;  j < 32;  ++j)
                            sum += N[i][j] * sample[ch][j][idx];  // 8b*15b=23b
                        _V[ch][table_idx + i] = sum + 8192 >> 14;
                    }
                    int U[512];
                    for (int i = 0;  i < 8;  ++i)
                    {   for (int j = 0;  j < 32;  ++j)
                        {   U[(i<<6) + j]      = _V[ch][(table_idx + (i<<7) + j     ) & 1023];
                            U[(i<<6) + j + 32] = _V[ch][(table_idx + (i<<7) + j + 96) & 1023];
                        }
                    }
                    for (int i = 0;  i < 512;  ++i)
                        U[i] = U[i] * D[i] + 32 >> 6;
                    for (int j = 0; j < 32; ++j)
                    {   int sum = 0;
                        for (int i = 0;  i < 16;  ++i)
                            sum -= U[(i << 5) + j];
                        sum = (sum + 8) >> 4;
                        sum = std::max(sum, -32768);
                        sum = std::min(sum, 32767);
                        pcm[idx << 6 | j << 1 | ch] = int16_t(sum);
                    }
                }
            }
            pcm += 192;
        }
    }
    b.get_bits(frame_size * 8 - b.counter2());
    return frame_size;
}


