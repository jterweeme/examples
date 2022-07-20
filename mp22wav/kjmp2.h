/******************************************************************************
** kjmp2 -- a minimal MPEG-1/2 Audio Layer II decoder library                **
** version 1.1                                                               **
*******************************************************************************
** Copyright (C) 2006-2013 Martin J. Fiedler <martin.fiedler@gmx.net>        **
**                                                                           **
** This software is provided 'as-is', without any express or implied         **
** warranty. In no event will the authors be held liable for any damages     **
** arising from the use of this software.                                    **
**                                                                           **
** Permission is granted to anyone to use this software for any purpose,     **
** including commercial applications, and to alter it and redistribute it    **
** freely, subject to the following restrictions:                            **
**   1. The origin of this software must not be misrepresented; you must not **
**      claim that you wrote the original software. If you use this software **
**      in a product, an acknowledgment in the product documentation would   **
**      be appreciated but is not required.                                  **
**   2. Altered source versions must be plainly marked as such, and must not **
**      be misrepresented as being the original software.                    **
**   3. This notice may not be removed or altered from any source            **
**      distribution.                                                        **
******************************************************************************/

#ifndef __KJMP2_H__
#define __KJMP2_H__

#include <cstdint>

// GENERAL WARNING: kjmp2 is *NOT* threadsafe!

// kjmp2_context_t: A kjmp2 context record. You don't need to use or modify
// any of the values inside this structure; just pass the whole structure
// to the kjmp2_* functions
typedef struct _kjmp2_context {
    int id;
    int V[2][1024];
    int Voffs;
} kjmp2_context_t;


extern void kjmp2_init(kjmp2_context_t *mp2);


extern int kjmp2_get_sample_rate(const unsigned char *frame);



extern unsigned long kjmp2_decode_frame(
    kjmp2_context_t *mp2,
    const unsigned char *frame,
    signed short *pcm
);

//static constexpr uint8_t STEREO = 0;
static constexpr uint8_t JOINT_STEREO = 1;
//static constexpr uint8_t DUAL_CHANNEL = 2;
static constexpr uint8_t MONO = 3;

static constexpr uint16_t sample_rates[8] = {
    44100, 48000, 32000, 0,  // MPEG-1
    22050, 24000, 16000, 0   // MPEG-2
};

static constexpr short bitrates[28] = {
    32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384,  // MPEG-1
     8, 16, 24, 32, 40, 48,  56,  64,  80,  96, 112, 128, 144, 160   // MPEG-2
};

// scale factor base values (24-bit fixed-point)
static constexpr int scf_base[3] = { 0x02000000, 0x01965FEA, 0x01428A30 };

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

struct Quantizer_spec
{
    uint16_t nlevels;
    uint8_t grouping;
    uint8_t cw_bits;
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

static constexpr uint32_t KJMP2_MAGIC = 0x32706D;

#endif
