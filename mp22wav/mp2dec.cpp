// kjmp2 example application: decodes .mp2 into .wav
// this file is public domain -- do with it whatever you want!

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <fcntl.h>
#include <string>
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


///////////// Table 3-B.2: Possible quantization per subband ///////////////////

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

// synthesis window
static constexpr int D[512] = {
     0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000,-0x00001,
    -0x00001,-0x00001,-0x00001,-0x00002,-0x00002,-0x00003,-0x00003,-0x00004,
    -0x00004,-0x00005,-0x00006,-0x00006,-0x00007,-0x00008,-0x00009,-0x0000A,
    -0x0000C,-0x0000D,-0x0000F,-0x00010,-0x00012,-0x00014,-0x00017,-0x00019,
    -0x0001C,-0x0001E,-0x00022,-0x00025,-0x00028,-0x0002C,-0x00030,-0x00034,
    -0x00039,-0x0003E,-0x00043,-0x00048,-0x0004E,-0x00054,-0x0005A,-0x00060,
    -0x00067,-0x0006E,-0x00074,-0x0007C,-0x00083,-0x0008A,-0x00092,-0x00099,
    -0x000A0,-0x000A8,-0x000AF,-0x000B6,-0x000BD,-0x000C3,-0x000C9,-0x000CF,
     0x000D5, 0x000DA, 0x000DE, 0x000E1, 0x000E3, 0x000E4, 0x000E4, 0x000E3,
     0x000E0, 0x000DD, 0x000D7, 0x000D0, 0x000C8, 0x000BD, 0x000B1, 0x000A3,
     0x00092, 0x0007F, 0x0006A, 0x00053, 0x00039, 0x0001D,-0x00001,-0x00023,
    -0x00047,-0x0006E,-0x00098,-0x000C4,-0x000F3,-0x00125,-0x0015A,-0x00190,
    -0x001CA,-0x00206,-0x00244,-0x00284,-0x002C6,-0x0030A,-0x0034F,-0x00396,
    -0x003DE,-0x00427,-0x00470,-0x004B9,-0x00502,-0x0054B,-0x00593,-0x005D9,
    -0x0061E,-0x00661,-0x006A1,-0x006DE,-0x00718,-0x0074D,-0x0077E,-0x007A9,
    -0x007D0,-0x007EF,-0x00808,-0x0081A,-0x00824,-0x00826,-0x0081F,-0x0080E,
     0x007F5, 0x007D0, 0x007A0, 0x00765, 0x0071E, 0x006CB, 0x0066C, 0x005FF,
     0x00586, 0x00500, 0x0046B, 0x003CA, 0x0031A, 0x0025D, 0x00192, 0x000B9,
    -0x0002C,-0x0011F,-0x00220,-0x0032D,-0x00446,-0x0056B,-0x0069B,-0x007D5,
    -0x00919,-0x00A66,-0x00BBB,-0x00D16,-0x00E78,-0x00FDE,-0x01148,-0x012B3,
    -0x01420,-0x0158C,-0x016F6,-0x0185C,-0x019BC,-0x01B16,-0x01C66,-0x01DAC,
    -0x01EE5,-0x02010,-0x0212A,-0x02232,-0x02325,-0x02402,-0x024C7,-0x02570,
    -0x025FE,-0x0266D,-0x026BB,-0x026E6,-0x026ED,-0x026CE,-0x02686,-0x02615,
    -0x02577,-0x024AC,-0x023B2,-0x02287,-0x0212B,-0x01F9B,-0x01DD7,-0x01BDD,
     0x019AE, 0x01747, 0x014A8, 0x011D1, 0x00EC0, 0x00B77, 0x007F5, 0x0043A,
     0x00046,-0x003E5,-0x00849,-0x00CE3,-0x011B4,-0x016B9,-0x01BF1,-0x0215B,
    -0x026F6,-0x02CBE,-0x032B3,-0x038D3,-0x03F1A,-0x04586,-0x04C15,-0x052C4,
    -0x05990,-0x06075,-0x06771,-0x06E80,-0x0759F,-0x07CCA,-0x083FE,-0x08B37,
    -0x09270,-0x099A7,-0x0A0D7,-0x0A7FD,-0x0AF14,-0x0B618,-0x0BD05,-0x0C3D8,
    -0x0CA8C,-0x0D11D,-0x0D789,-0x0DDC9,-0x0E3DC,-0x0E9BD,-0x0EF68,-0x0F4DB,
    -0x0FA12,-0x0FF09,-0x103BD,-0x1082C,-0x10C53,-0x1102E,-0x113BD,-0x116FB,
    -0x119E8,-0x11C82,-0x11EC6,-0x120B3,-0x12248,-0x12385,-0x12467,-0x124EF,
     0x1251E, 0x124F0, 0x12468, 0x12386, 0x12249, 0x120B4, 0x11EC7, 0x11C83,
     0x119E9, 0x116FC, 0x113BE, 0x1102F, 0x10C54, 0x1082D, 0x103BE, 0x0FF0A,
     0x0FA13, 0x0F4DC, 0x0EF69, 0x0E9BE, 0x0E3DD, 0x0DDCA, 0x0D78A, 0x0D11E,
     0x0CA8D, 0x0C3D9, 0x0BD06, 0x0B619, 0x0AF15, 0x0A7FE, 0x0A0D8, 0x099A8,
     0x09271, 0x08B38, 0x083FF, 0x07CCB, 0x075A0, 0x06E81, 0x06772, 0x06076,
     0x05991, 0x052C5, 0x04C16, 0x04587, 0x03F1B, 0x038D4, 0x032B4, 0x02CBF,
     0x026F7, 0x0215C, 0x01BF2, 0x016BA, 0x011B5, 0x00CE4, 0x0084A, 0x003E6,
    -0x00045,-0x00439,-0x007F4,-0x00B76,-0x00EBF,-0x011D0,-0x014A7,-0x01746,
     0x019AE, 0x01BDE, 0x01DD8, 0x01F9C, 0x0212C, 0x02288, 0x023B3, 0x024AD,
     0x02578, 0x02616, 0x02687, 0x026CF, 0x026EE, 0x026E7, 0x026BC, 0x0266E,
     0x025FF, 0x02571, 0x024C8, 0x02403, 0x02326, 0x02233, 0x0212B, 0x02011,
     0x01EE6, 0x01DAD, 0x01C67, 0x01B17, 0x019BD, 0x0185D, 0x016F7, 0x0158D,
     0x01421, 0x012B4, 0x01149, 0x00FDF, 0x00E79, 0x00D17, 0x00BBC, 0x00A67,
     0x0091A, 0x007D6, 0x0069C, 0x0056C, 0x00447, 0x0032E, 0x00221, 0x00120,
     0x0002D,-0x000B8,-0x00191,-0x0025C,-0x00319,-0x003C9,-0x0046A,-0x004FF,
    -0x00585,-0x005FE,-0x0066B,-0x006CA,-0x0071D,-0x00764,-0x0079F,-0x007CF,
     0x007F5, 0x0080F, 0x00820, 0x00827, 0x00825, 0x0081B, 0x00809, 0x007F0,
     0x007D1, 0x007AA, 0x0077F, 0x0074E, 0x00719, 0x006DF, 0x006A2, 0x00662,
     0x0061F, 0x005DA, 0x00594, 0x0054C, 0x00503, 0x004BA, 0x00471, 0x00428,
     0x003DF, 0x00397, 0x00350, 0x0030B, 0x002C7, 0x00285, 0x00245, 0x00207,
     0x001CB, 0x00191, 0x0015B, 0x00126, 0x000F4, 0x000C5, 0x00099, 0x0006F,
     0x00048, 0x00024, 0x00002,-0x0001C,-0x00038,-0x00052,-0x00069,-0x0007E,
    -0x00091,-0x000A2,-0x000B0,-0x000BC,-0x000C7,-0x000CF,-0x000D6,-0x000DC,
    -0x000DF,-0x000E2,-0x000E3,-0x000E3,-0x000E2,-0x000E0,-0x000DD,-0x000D9,
     0x000D5, 0x000D0, 0x000CA, 0x000C4, 0x000BE, 0x000B7, 0x000B0, 0x000A9,
     0x000A1, 0x0009A, 0x00093, 0x0008B, 0x00084, 0x0007D, 0x00075, 0x0006F,
     0x00068, 0x00061, 0x0005B, 0x00055, 0x0004F, 0x00049, 0x00044, 0x0003F,
     0x0003A, 0x00035, 0x00031, 0x0002D, 0x00029, 0x00026, 0x00023, 0x0001F,
     0x0001D, 0x0001A, 0x00018, 0x00015, 0x00013, 0x00011, 0x00010, 0x0000E,
     0x0000D, 0x0000B, 0x0000A, 0x00009, 0x00008, 0x00007, 0x00007, 0x00006,
     0x00005, 0x00005, 0x00004, 0x00004, 0x00003, 0x00003, 0x00002, 0x00002,
     0x00002, 0x00002, 0x00001, 0x00001, 0x00001, 0x00001, 0x00001, 0x00001
};

static constexpr uint16_t sample_rates[8] = {
    44100, 48000, 32000, 0,  // MPEG-1
    22050, 24000, 16000, 0   // MPEG-2
};

class BitBuffer
{
private:
    int _bit_window;
    int bits_in_window;
    const uint8_t *frame_pos;
    int show_bits(int bit_count);
public:
    void init(const uint8_t *frame);
    int get_bits(int bit_count);
};

class Decoder
{
private:
    const Quantizer_spec *allocation[2][32];
    int scfsi[2][32];
    int scalefactor[2][32][3];
    int sample[2][32][3];
    int Voffs;
    int N[64][32];  // N[i][j] as 8-bit fixed-point
    int _V[2][1024];
    int _U[512];
    int initialized = 0;
    void read_samples(const Quantizer_spec *q, int scalefactor, int *sample, BitBuffer &b);
public:
    int kjmp2_get_sample_rate(const uint8_t *frame);
    void kjmp2_init();
    uint32_t kjmp2_decode_frame(const uint8_t *frame, int16_t *pcm);
};

class Toolbox
{
public:
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
    bool stdinput() const;
    bool stdoutput() const;
    std::string ifn() const;
};

class CWavHeader
{
private:
    int _rate;
    uint32_t _filesize = 0;
public:
    void rate(int val);
    void filesize(uint32_t val);
    void write(FILE *fp) const;
};

class CMain
{
private:
    static constexpr uint32_t KJMP2_SAMPLES_PER_FRAME = 1152;
    static constexpr uint32_t KJMP2_MAX_FRAME_SIZE = 1440;
    static constexpr uint32_t MAX_BUFSIZE = 1000 * KJMP2_MAX_FRAME_SIZE;
public:
    int run(FILE *fin, FILE *outfile);
};

void CWavHeader::rate(int val)
{
    _rate = val;
}

void CWavHeader::filesize(uint32_t val)
{
    _filesize = val;
}

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

bool COptions::stdinput() const
{
    return _stdinput;
}

bool COptions::stdoutput() const
{
    return _stdoutput;
}

std::string COptions::ifn() const
{
    return _ifn;
}

int main(int argc, char **argv)
{
    COptions opts;
    opts.parse(argc, argv);
    CMain inst;
    FILE *fin;

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
    int ret = inst.run(fin, stdout);
    return ret;
}

int CMain::run(FILE *fin, FILE *fout)
{
    uint8_t buffer[MAX_BUFSIZE];
    signed short samples[KJMP2_SAMPLES_PER_FRAME * 2];
    int bufsize = (int) fread((void*) buffer, 1, MAX_BUFSIZE, fin);
    Decoder d;
    int rate = bufsize > 4 ? d.kjmp2_get_sample_rate(buffer) : 0;

    if (!rate)
    {
        fprintf(stderr, "Input is not a valid MP2 audio file, exiting.\n");
        fclose(fin);
        return 1;
    }
#ifdef _WIN32
    setmode(fileno(stdout), O_BINARY);
#endif
    if (!fout)
    {
        fprintf(stderr, "Could not open output file %s!\n", "out.wav");
        return 1;
    }

    CWavHeader h;
    h.rate(rate);
    h.write(fout);

    //printf("Decoding %s into %s ...\n", infn, outname);
    int out_bytes = 0;
    int desync = 0;
    int bufpos = 0;
    int eof = 0;
    int in_offset = 0;
    d.kjmp2_init();

    while (!eof || (bufsize > 4))
    {
        int bytes;

        if (!eof && (bufsize < int(KJMP2_MAX_FRAME_SIZE)))
        {
            //std::cerr << bufsize << "\r\n";
            //std::cerr.flush();
            memcpy(buffer, buffer + bufpos, bufsize);
            bufpos = 0;
            in_offset += bufsize;
            bytes = (int) fread(buffer + bufsize, 1, MAX_BUFSIZE - bufsize, fin);

            if (bytes > 0) {
                bufsize += bytes;
            } else {
                eof = 1;
            }
        }
        else
        {
            bytes = d.kjmp2_decode_frame(&buffer[bufpos], samples);

            if ((bytes < 4) || (bytes > int(KJMP2_MAX_FRAME_SIZE)) || (bytes > bufsize))
            {
                if (!desync)
                {
                    fprintf(stderr, "Stream error detected at file offset %d.\n", in_offset + bufpos);
                }

                desync = bytes = 1;
            }
            else
            {
                out_bytes += (int) fwrite((const void*) samples, 1, KJMP2_SAMPLES_PER_FRAME * 4, fout);
                desync = 0;
            }

            bufsize -= bytes;
            bufpos += bytes;
        }
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

// kjmp2_get_sample_rate: Returns the sample rate of a MP2 stream.
// frame: Points to at least the first three bytes of a frame from the
//        stream.
// return value: The sample rate of the stream in Hz, or zero if the stream
//               isn't valid.
int Decoder::kjmp2_get_sample_rate(const uint8_t *frame)
{
    if (!frame)
        return 0;

    if (( frame[0]         != 0xFF)   // no valid syncword?
    ||  ((frame[1] & 0xF6) != 0xF4)   // no MPEG-1/2 Audio Layer II?
    ||  ((frame[2] - 0x10) >= 0xE0))  // invalid bitrate?
        return 0;

    return sample_rates[(((frame[1] & 0x08) >> 1) ^ 4)  // MPEG-1/2 switch
                      + ((frame[2] >> 2) & 3)];         // actual rate
}

void BitBuffer::init(const uint8_t *frame)
{
    _bit_window = frame[0] << 16;
    bits_in_window = 8;
    frame_pos = &frame[1];
}

int BitBuffer::show_bits(int bit_count)
{
    return _bit_window >> 24 - bit_count;
}

int BitBuffer::get_bits(int bit_count)
{
    int result = show_bits(bit_count);
    _bit_window = (_bit_window << bit_count) & 0xFFFFFF;
    bits_in_window -= bit_count;

    while (bits_in_window < 16)
    {
        _bit_window |= (*frame_pos++) << (16 - bits_in_window);
        bits_in_window += 8;
    }

    return result;
}

// kjmp2_init: This function must be called once to initialize each kjmp2
// decoder instance.
void Decoder::kjmp2_init()
{
    // check if global initialization is required
    if (!initialized)
    {
        int *nptr = &N[0][0];

        // compute N[i][j]
        for (int i = 0;  i < 64;  ++i)
            for (int j = 0;  j < 32;  ++j)
                *nptr++ = int(256.0 * cos(((16 + i) * ((j << 1) + 1)) * 0.0490873852123405));

        initialized = 1;
    }

    // perform local initialization: clean the context and put the magic in it
    for (int i = 0;  i < 2;  ++i)
        for (int j = 1023;  j >= 0;  --j)
            _V[i][j] = 0;

    Voffs = 0;
}

// DECODE HELPER FUNCTIONS
static const Quantizer_spec *read_allocation(int sb, int b2_table, BitBuffer &b)
{
    int table_idx = quant_lut_step3[b2_table][sb];
    table_idx = quant_lut_step4[table_idx & 15][b.get_bits(table_idx >> 4)];
    return table_idx ? (&quantizer_table[table_idx - 1]) : 0;
}


void Decoder::read_samples(const Quantizer_spec *q, int scalefactor, int *sample, BitBuffer &b)
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
        int val = b.get_bits(q->cw_bits);
        sample[0] = val % adj;
        val /= adj;
        sample[1] = val % adj;
        sample[2] = val / adj;
    }
    else
    {
        // decode direct samples
        for (int idx = 0;  idx < 3;  ++idx)
            sample[idx] = b.get_bits(q->cw_bits);
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

// kjmp2_decode_frame: Decode one frame of audio.
// mp2: A pointer to a context record that has been initialized with
//      kjmp2_init before.
// frame: A pointer to the frame to decode. It *must* be a complete frame,
//        because no error checking is done!
// pcm: A pointer to the output PCM data. kjmp2_decode_frame() will always
//      return 1152 (=KJMP2_SAMPLES_PER_FRAME) interleaved stereo samples
//      in a native-endian 16-bit signed format. Even for mono streams,
//      stereo output will be produced.
// return value: The number of bytes in the current frame. In a valid stream,
//               frame + kjmp2_decode_frame(..., frame, ...) will point to
//               the next frame, if frames are consecutive in memory.
// Note: pcm may be NULL. In this case, kjmp2_decode_frame() will return the
//       size of the frame without actually decoding it.
uint32_t
Decoder::kjmp2_decode_frame(const uint8_t *frame, int16_t *pcm)
{
    BitBuffer b;
    b.init(frame);
    uint8_t frame0 = b.get_bits(8);
    uint8_t frame1 = b.get_bits(8);

    // check for valid header: syncword OK, MPEG-Audio Layer 2
    if ((frame0 != 0xFF) || ((frame1 & 0xF6) != 0xF4))
        return 0;

    // set up the bitstream reader


    // read the rest of the header
    unsigned bit_rate_index_minus1 = b.get_bits(4) - 1;

    if (bit_rate_index_minus1 > 13)
        return 0;  // invalid bit rate or 'free format'

    unsigned sampling_frequency = b.get_bits(2);

    if (sampling_frequency == 3)
        return 0;

    if ((frame1 & 0x08) == 0) {  // MPEG-2
        sampling_frequency += 4;
        bit_rate_index_minus1 += 14;
    }
    unsigned padding_bit = b.get_bits(1);
    b.get_bits(1);  // discard private_bit
    unsigned mode = b.get_bits(2);
    int bound, sblimit;

    // parse the mode_extension, set up the stereo bound
    if (mode == JOINT_STEREO)
    {
        bound = b.get_bits(2) + 1 << 2;
    }
    else
    {
        b.get_bits(2);
        bound = mode == MONO ? 0 : 32;
    }

    // discard the last 4 bits of the header and the CRC value, if present
    b.get_bits(4);

    if ((frame1 & 1) == 0)
        b.get_bits(16);

    // compute the frame size
    uint32_t frame_size = (144000 * bitrates[bit_rate_index_minus1]
               / sample_rates[sampling_frequency]) + padding_bit;

    if (!pcm)
        return frame_size;  // no decoding

    int table_idx;

    // prepare the quantizer table lookups
    if (sampling_frequency & 4)
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
        table_idx = quant_lut_step2[table_idx][sampling_frequency];
        sblimit = table_idx & 63;
        table_idx >>= 6;
    }

    bound = std::min(bound, sblimit);


    // read the allocation information
    for (int sb = 0;  sb < bound;  ++sb)
        for (int ch = 0;  ch < 2;  ++ch)
            allocation[ch][sb] = read_allocation(sb, table_idx, b);

    for (int sb = bound;  sb < sblimit;  ++sb)
        allocation[0][sb] = allocation[1][sb] = read_allocation(sb, table_idx, b);

    // read scale factor selector information
    int nch = (mode == MONO) ? 1 : 2;

    for (int sb = 0;  sb < sblimit;  ++sb)
    {
        for (int ch = 0;  ch < nch;  ++ch)
            if (allocation[ch][sb])
                scfsi[ch][sb] = b.get_bits(2);

        if (mode == MONO)
            scfsi[1][sb] = scfsi[0][sb];
    }

    // read scale factors
    for (int sb = 0;  sb < sblimit;  ++sb)
    {
        for (int ch = 0;  ch < nch;  ++ch)
        {
            if (allocation[ch][sb])
            {
                switch (scfsi[ch][sb])
                {
                case 0:
                    scalefactor[ch][sb][0] = b.get_bits(6);
                    scalefactor[ch][sb][1] = b.get_bits(6);
                    scalefactor[ch][sb][2] = b.get_bits(6);
                    break;
                case 1:
                    scalefactor[ch][sb][0] =
                    scalefactor[ch][sb][1] = b.get_bits(6);
                    scalefactor[ch][sb][2] = b.get_bits(6);
                    break;
                case 2:
                    scalefactor[ch][sb][0] =
                    scalefactor[ch][sb][1] =
                    scalefactor[ch][sb][2] = b.get_bits(6);
                    break;
                case 3:
                    scalefactor[ch][sb][0] = b.get_bits(6);
                    scalefactor[ch][sb][1] =
                    scalefactor[ch][sb][2] = b.get_bits(6);
                    break;
                }
            }
        }

        if (mode == MONO)
            for (int part = 0;  part < 3;  ++part)
                scalefactor[1][sb][part] = scalefactor[0][sb][part];
    }

    // coefficient input and reconstruction
    for (int part = 0;  part < 3;  ++part)
    {
        for (int gr = 0;  gr < 4;  ++gr)
        {
            // read the samples
            for (int sb = 0;  sb < bound;  ++sb)
                for (int ch = 0;  ch < 2;  ++ch)
                    read_samples(allocation[ch][sb], scalefactor[ch][sb][part], &sample[ch][sb][0], b);

            for (int sb = bound;  sb < sblimit;  ++sb)
            {
                read_samples(allocation[0][sb], scalefactor[0][sb][part], &sample[0][sb][0], b);

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
                Voffs = table_idx = (Voffs - 64) & 1023;

                for (int ch = 0;  ch < 2;  ++ch)
                {
                    // matrixing
                    for (int i = 0;  i < 64;  ++i)
                    {
                        int sum = 0;

                        for (int j = 0;  j < 32;  ++j)
                            sum += N[i][j] * sample[ch][j][idx];  // 8b*15b=23b

                        // intermediate value is 28 bit (23 + 5), clamp to 14b
                        _V[ch][table_idx + i] = sum + 8192 >> 14;
                    }

                    // construction of U
                    for (int i = 0;  i < 8;  ++i)
                    {
                        for (int j = 0;  j < 32;  ++j)
                        {
                            _U[(i<<6) + j]      = _V[ch][(table_idx + (i<<7) + j     ) & 1023];
                            _U[(i<<6) + j + 32] = _V[ch][(table_idx + (i<<7) + j + 96) & 1023];
                        }
                    }

                    // apply window
                    for (int i = 0;  i < 512;  ++i)
                        _U[i] = _U[i] * D[i] + 32 >> 6;

                    // output samples
                    for (int j = 0; j < 32; ++j)
                    {
                        int sum = 0;

                        for (int i = 0;  i < 16;  ++i)
                            sum -= _U[(i << 5) + j];

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
    return frame_size;
}


