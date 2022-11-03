/*
 * slice.c
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 2003      Peter Gubanov <peter@elecard.net.ru>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include <inttypes.h>
#include "mpeg2.h"
#include <string.h> /* memcmp/memset, try to remove */
#include <stdlib.h>

typedef struct {
    mpeg2_fbuf_t fbuf;
} fbuf_alloc_t;

struct mpeg2dec_s {
    mpeg2_decoder_t decoder;

    mpeg2_info_t info;

    uint32_t shift;
    int is_display_initialized;
    mpeg2_state_t (* action) (struct mpeg2dec_s * mpeg2dec);
    mpeg2_state_t state;
    uint32_t ext_state;

    /* allocated in init - gcc has problems allocating such big structures */
    uint8_t * chunk_buffer;
    /* pointer to start of the current chunk */
    uint8_t * chunk_start;
    /* pointer to current position in chunk_buffer */
    uint8_t * chunk_ptr;
    /* last start code ? */
    uint8_t code;

    /* picture tags */
    uint32_t tag_current, tag2_current, tag_previous, tag2_previous;
    int num_tags;
    int bytes_since_tag;

    int first;
    int alloc_index_user;
    int alloc_index;
    uint8_t first_decode_slice;
    uint8_t nb_decode_slices;

    unsigned int user_data_len;

    mpeg2_sequence_t new_sequence;
    mpeg2_sequence_t sequence;
    mpeg2_gop_t new_gop;
    mpeg2_gop_t gop;
    mpeg2_picture_t new_picture;
    mpeg2_picture_t pictures[4];
    mpeg2_picture_t * picture;
    /*const*/ mpeg2_fbuf_t * fbuf[3];   /* 0: current fbuf, 1-2: prediction fbufs */

    fbuf_alloc_t fbuf_alloc[3];
    int custom_fbuf;
    uint8_t * yuv_buf[3][3];
    int yuv_index;
    mpeg2_convert_t * convert;
    void * convert_arg;
    unsigned int convert_id_size;
    int convert_stride;
    void (* convert_start) (void * id, const mpeg2_fbuf_t * fbuf,
                const mpeg2_picture_t * picture,
                const mpeg2_gop_t * gop);

    uint8_t * buf_start;
    uint8_t * buf_end;

    int16_t display_offset_x, display_offset_y;

    int copy_matrix;
    int8_t scaled[4]; /* XXX: MOVED */
    //int8_t q_scale_type, scaled[4];
    uint8_t quantizer_matrix[4][64];
    uint8_t new_quantizer_matrix[4][64];
};

typedef struct {
    int dummy;
} cpu_state_t;

static constexpr uint8_t SEQ_EXT = 2;
static constexpr uint8_t SEQ_DISPLAY_EXT = 4;
static constexpr uint8_t QUANT_MATRIX_EXT = 8;
static constexpr uint16_t COPYRIGHT_EXT = 0x10;
static constexpr uint16_t PIC_DISPLAY_EXT = 0x80;
static constexpr uint16_t PIC_CODING_EXT = 0x100;

/* default intra quant matrix, in zig-zag order */
static const uint8_t default_intra_quantizer_matrix[64] ATTR_ALIGN(16) = {
    8,
    16, 16,
    19, 16, 19,
    22, 22, 22, 22,
    22, 22, 26, 24, 26,
    27, 27, 27, 26, 26, 26,
    26, 27, 27, 27, 29, 29, 29,
    34, 34, 34, 29, 29, 29, 27, 27,
    29, 29, 32, 32, 34, 34, 37,
    38, 37, 35, 35, 34, 35,
    38, 38, 40, 40, 40,
    48, 48, 46, 46,
    56, 56, 58,
    69, 69,
    83
};

uint8_t mpeg2_scan_norm[64] ATTR_ALIGN(16) = {
    /* Zig-Zag scan pattern */
     0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};

uint8_t mpeg2_scan_alt[64] ATTR_ALIGN(16) = {
    /* Alternate scan pattern */
     0, 8,  16, 24,  1,  9,  2, 10, 17, 25, 32, 40, 48, 56, 57, 49,
    41, 33, 26, 18,  3, 11,  4, 12, 19, 27, 34, 42, 50, 58, 35, 43,
    51, 59, 20, 28,  5, 13,  6, 14, 21, 29, 36, 44, 52, 60, 37, 45,
    53, 61, 22, 30,  7, 15, 23, 31, 38, 46, 54, 62, 39, 47, 55, 63
};

mpeg2_state_t mpeg2_parse_header (mpeg2dec_t * mpeg2dec);
void mpeg2_header_state_init (mpeg2dec_t * mpeg2dec);
void mpeg2_reset_info (mpeg2_info_t * info);
int mpeg2_header_sequence (mpeg2dec_t * mpeg2dec);
int mpeg2_header_gop (mpeg2dec_t * mpeg2dec);
mpeg2_state_t mpeg2_header_picture_start (mpeg2dec_t * mpeg2dec);
int mpeg2_header_picture (mpeg2dec_t * mpeg2dec);
int mpeg2_header_extension (mpeg2dec_t * mpeg2dec);
int mpeg2_header_user_data (mpeg2dec_t * mpeg2dec);
void mpeg2_header_sequence_finalize (mpeg2dec_t * mpeg2dec);
void mpeg2_header_gop_finalize (mpeg2dec_t * mpeg2dec);
void mpeg2_header_picture_finalize (mpeg2dec_t * mpeg2dec, uint32_t accels);
mpeg2_state_t mpeg2_header_slice_start (mpeg2dec_t * mpeg2dec);
mpeg2_state_t mpeg2_header_end (mpeg2dec_t * mpeg2dec);
void mpeg2_set_fbuf (mpeg2dec_t * mpeg2dec, int b_type);
void mpeg2_mc_init (uint32_t accel);

struct mpeg2_mc_t {
    mpeg2_mc_fct *put[8];
    mpeg2_mc_fct *avg[8];
};

extern mpeg2_mc_t mpeg2_mc_c;


extern mpeg2_mc_t mpeg2_mc;
extern void (* mpeg2_idct_copy) (int16_t * block, uint8_t * dest, int stride);
extern void (* mpeg2_idct_add) (int last, int16_t * block,
				uint8_t * dest, int stride);

static void GETWORD(uint32_t &bit_buf, int shift, const uint8_t *&bit_ptr)
{
    bit_buf |= ((bit_ptr[0] << 8) | bit_ptr[1]) << shift;
    bit_ptr += 2;
}

static inline void bitstream_init(mpeg2_decoder_t *decoder, const uint8_t *start)
{ 
    decoder->bitstream_buf =
    (start[0] << 24) | (start[1] << 16) | (start[2] << 8) | start[3];
    decoder->bitstream_ptr = start + 4;
    decoder->bitstream_bits = -16;
} 

/* make sure that there are at least 16 valid bits in bit_buf */
static void NEEDBITS(uint32_t &bit_buf, int &bits, const uint8_t *&bit_ptr)
{
    if (unlikely(bits > 0))
    {
        GETWORD(bit_buf, bits, bit_ptr);
        bits -= 16;
    }
}

static void dumpbits(uint32_t &bit_buf, int &bits, size_t num)
{
    bit_buf <<= num;
    bits += num;
}

/* take num bits from the high part of bit_buf and zero extend them */
static uint32_t UBITS(uint32_t bit_buf, size_t n)
{
    return bit_buf >> 32 - n;
}

typedef struct {
    uint8_t modes;
    uint8_t len;
} MBtab;

typedef struct {
    uint8_t delta;
    uint8_t len;
} MVtab;

typedef struct {
    int8_t dmv;
    uint8_t len;
} DMVtab;

typedef struct {
    uint8_t cbp;
    uint8_t len;
} CBPtab;

typedef struct {
    uint8_t size;
    uint8_t len;
} DCtab;

typedef struct {
    uint8_t run;
    uint8_t level;
    uint8_t len;
} DCTtab;

typedef struct {
    uint8_t mba;
    uint8_t len;
} MBAtab;


#define INTRA MACROBLOCK_INTRA
#define QUANT MACROBLOCK_QUANT

static const MBtab MB_I [] = {
    {INTRA|QUANT, 2}, {INTRA, 1}
};

#define MC MACROBLOCK_MOTION_FORWARD
#define CODED MACROBLOCK_PATTERN

static const MBtab MB_P [] = {
    {INTRA|QUANT, 6}, {CODED|QUANT, 5}, {MC|CODED|QUANT, 5}, {INTRA,    5},
    {MC,          3}, {MC,          3}, {MC,             3}, {MC,       3},
    {CODED,       2}, {CODED,       2}, {CODED,          2}, {CODED,    2},
    {CODED,       2}, {CODED,       2}, {CODED,          2}, {CODED,    2},
    {MC|CODED,    1}, {MC|CODED,    1}, {MC|CODED,       1}, {MC|CODED, 1},
    {MC|CODED,    1}, {MC|CODED,    1}, {MC|CODED,       1}, {MC|CODED, 1},
    {MC|CODED,    1}, {MC|CODED,    1}, {MC|CODED,       1}, {MC|CODED, 1},
    {MC|CODED,    1}, {MC|CODED,    1}, {MC|CODED,       1}, {MC|CODED, 1}
};

#define FWD MACROBLOCK_MOTION_FORWARD
#define BWD MACROBLOCK_MOTION_BACKWARD
#define INTER MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD

static const MBtab MB_B [] = {
    {0,                 6}, {INTRA|QUANT,       6},
    {BWD|CODED|QUANT,   6}, {FWD|CODED|QUANT,   6},
    {INTER|CODED|QUANT, 5}, {INTER|CODED|QUANT, 5},
                    {INTRA,       5}, {INTRA,       5},
    {FWD,         4}, {FWD,         4}, {FWD,         4}, {FWD,         4},
    {FWD|CODED,   4}, {FWD|CODED,   4}, {FWD|CODED,   4}, {FWD|CODED,   4},
    {BWD,         3}, {BWD,         3}, {BWD,         3}, {BWD,         3},
    {BWD,         3}, {BWD,         3}, {BWD,         3}, {BWD,         3},
    {BWD|CODED,   3}, {BWD|CODED,   3}, {BWD|CODED,   3}, {BWD|CODED,   3},
    {BWD|CODED,   3}, {BWD|CODED,   3}, {BWD|CODED,   3}, {BWD|CODED,   3},
    {INTER,       2}, {INTER,       2}, {INTER,       2}, {INTER,       2},
    {INTER,       2}, {INTER,       2}, {INTER,       2}, {INTER,       2},
    {INTER,       2}, {INTER,       2}, {INTER,       2}, {INTER,       2},
    {INTER,       2}, {INTER,       2}, {INTER,       2}, {INTER,       2},
    {INTER|CODED, 2}, {INTER|CODED, 2}, {INTER|CODED, 2}, {INTER|CODED, 2},
    {INTER|CODED, 2}, {INTER|CODED, 2}, {INTER|CODED, 2}, {INTER|CODED, 2},
    {INTER|CODED, 2}, {INTER|CODED, 2}, {INTER|CODED, 2}, {INTER|CODED, 2},
    {INTER|CODED, 2}, {INTER|CODED, 2}, {INTER|CODED, 2}, {INTER|CODED, 2}
};

#undef INTRA
#undef QUANT
#undef MC
#undef CODED
#undef FWD
#undef BWD
#undef INTER


static const MVtab MV_4 [] = {
    { 3, 6}, { 2, 4}, { 1, 3}, { 1, 3}, { 0, 2}, { 0, 2}, { 0, 2}, { 0, 2}
};

static const MVtab MV_10 [] = {
    { 0,10}, { 0,10}, { 0,10}, { 0,10}, { 0,10}, { 0,10}, { 0,10}, { 0,10},
    { 0,10}, { 0,10}, { 0,10}, { 0,10}, {15,10}, {14,10}, {13,10}, {12,10},
    {11,10}, {10,10}, { 9, 9}, { 9, 9}, { 8, 9}, { 8, 9}, { 7, 9}, { 7, 9},
    { 6, 7}, { 6, 7}, { 6, 7}, { 6, 7}, { 6, 7}, { 6, 7}, { 6, 7}, { 6, 7},
    { 5, 7}, { 5, 7}, { 5, 7}, { 5, 7}, { 5, 7}, { 5, 7}, { 5, 7}, { 5, 7},
    { 4, 7}, { 4, 7}, { 4, 7}, { 4, 7}, { 4, 7}, { 4, 7}, { 4, 7}, { 4, 7}
};

static const DMVtab DMV_2 [] = {
    { 0, 1}, { 0, 1}, { 1, 2}, {-1, 2}
};


static const CBPtab CBP_7 [] = {
    {0x11, 7}, {0x12, 7}, {0x14, 7}, {0x18, 7},
    {0x21, 7}, {0x22, 7}, {0x24, 7}, {0x28, 7},
    {0x3f, 6}, {0x3f, 6}, {0x30, 6}, {0x30, 6},
    {0x09, 6}, {0x09, 6}, {0x06, 6}, {0x06, 6},
    {0x1f, 5}, {0x1f, 5}, {0x1f, 5}, {0x1f, 5},
    {0x10, 5}, {0x10, 5}, {0x10, 5}, {0x10, 5},
    {0x2f, 5}, {0x2f, 5}, {0x2f, 5}, {0x2f, 5},
    {0x20, 5}, {0x20, 5}, {0x20, 5}, {0x20, 5},
    {0x07, 5}, {0x07, 5}, {0x07, 5}, {0x07, 5},
    {0x0b, 5}, {0x0b, 5}, {0x0b, 5}, {0x0b, 5},
    {0x0d, 5}, {0x0d, 5}, {0x0d, 5}, {0x0d, 5},
    {0x0e, 5}, {0x0e, 5}, {0x0e, 5}, {0x0e, 5},
    {0x05, 5}, {0x05, 5}, {0x05, 5}, {0x05, 5},
    {0x0a, 5}, {0x0a, 5}, {0x0a, 5}, {0x0a, 5},
    {0x03, 5}, {0x03, 5}, {0x03, 5}, {0x03, 5},
    {0x0c, 5}, {0x0c, 5}, {0x0c, 5}, {0x0c, 5},
    {0x01, 4}, {0x01, 4}, {0x01, 4}, {0x01, 4},
    {0x01, 4}, {0x01, 4}, {0x01, 4}, {0x01, 4},
    {0x02, 4}, {0x02, 4}, {0x02, 4}, {0x02, 4},
    {0x02, 4}, {0x02, 4}, {0x02, 4}, {0x02, 4},
    {0x04, 4}, {0x04, 4}, {0x04, 4}, {0x04, 4},
    {0x04, 4}, {0x04, 4}, {0x04, 4}, {0x04, 4},
    {0x08, 4}, {0x08, 4}, {0x08, 4}, {0x08, 4},
    {0x08, 4}, {0x08, 4}, {0x08, 4}, {0x08, 4},
    {0x0f, 3}, {0x0f, 3}, {0x0f, 3}, {0x0f, 3},
    {0x0f, 3}, {0x0f, 3}, {0x0f, 3}, {0x0f, 3},
    {0x0f, 3}, {0x0f, 3}, {0x0f, 3}, {0x0f, 3},
    {0x0f, 3}, {0x0f, 3}, {0x0f, 3}, {0x0f, 3}
};

static const CBPtab CBP_9 [] = {
    {0,    9}, {0x00, 9}, {0x39, 9}, {0x36, 9},
    {0x37, 9}, {0x3b, 9}, {0x3d, 9}, {0x3e, 9},
    {0x17, 8}, {0x17, 8}, {0x1b, 8}, {0x1b, 8},
    {0x1d, 8}, {0x1d, 8}, {0x1e, 8}, {0x1e, 8},
    {0x27, 8}, {0x27, 8}, {0x2b, 8}, {0x2b, 8},
    {0x2d, 8}, {0x2d, 8}, {0x2e, 8}, {0x2e, 8},
    {0x19, 8}, {0x19, 8}, {0x16, 8}, {0x16, 8},
    {0x29, 8}, {0x29, 8}, {0x26, 8}, {0x26, 8},
    {0x35, 8}, {0x35, 8}, {0x3a, 8}, {0x3a, 8},
    {0x33, 8}, {0x33, 8}, {0x3c, 8}, {0x3c, 8},
    {0x15, 8}, {0x15, 8}, {0x1a, 8}, {0x1a, 8},
    {0x13, 8}, {0x13, 8}, {0x1c, 8}, {0x1c, 8},
    {0x25, 8}, {0x25, 8}, {0x2a, 8}, {0x2a, 8},
    {0x23, 8}, {0x23, 8}, {0x2c, 8}, {0x2c, 8},
    {0x31, 8}, {0x31, 8}, {0x32, 8}, {0x32, 8},
    {0x34, 8}, {0x34, 8}, {0x38, 8}, {0x38, 8}
};


static const DCtab DC_lum_5 [] = {
    {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2},
    {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2},
    {0, 3}, {0, 3}, {0, 3}, {0, 3}, {3, 3}, {3, 3}, {3, 3}, {3, 3},
    {4, 3}, {4, 3}, {4, 3}, {4, 3}, {5, 4}, {5, 4}, {6, 5}
};

static const DCtab DC_chrom_5 [] = {
    {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2},
    {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2},
    {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2},
    {3, 3}, {3, 3}, {3, 3}, {3, 3}, {4, 4}, {4, 4}, {5, 5}
};

static const DCtab DC_long [] = {
    {6, 5}, {6, 5}, {6, 5}, {6, 5}, {6, 5}, {6, 5}, { 6, 5}, { 6, 5},
    {6, 5}, {6, 5}, {6, 5}, {6, 5}, {6, 5}, {6, 5}, { 6, 5}, { 6, 5},
    {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, { 7, 6}, { 7, 6},
    {8, 7}, {8, 7}, {8, 7}, {8, 7}, {9, 8}, {9, 8}, {10, 9}, {11, 9}
};

static const DCTtab DCT_16 [] = {
    {129, 0, 0}, {129, 0, 0}, {129, 0, 0}, {129, 0, 0},
    {129, 0, 0}, {129, 0, 0}, {129, 0, 0}, {129, 0, 0},
    {129, 0, 0}, {129, 0, 0}, {129, 0, 0}, {129, 0, 0},
    {129, 0, 0}, {129, 0, 0}, {129, 0, 0}, {129, 0, 0},
    {  2,18, 0}, {  2,17, 0}, {  2,16, 0}, {  2,15, 0},
    {  7, 3, 0}, { 17, 2, 0}, { 16, 2, 0}, { 15, 2, 0},
    { 14, 2, 0}, { 13, 2, 0}, { 12, 2, 0}, { 32, 1, 0},
    { 31, 1, 0}, { 30, 1, 0}, { 29, 1, 0}, { 28, 1, 0}
};

static const DCTtab DCT_15 [] = {
    {  1,40,15}, {  1,39,15}, {  1,38,15}, {  1,37,15},
    {  1,36,15}, {  1,35,15}, {  1,34,15}, {  1,33,15},
    {  1,32,15}, {  2,14,15}, {  2,13,15}, {  2,12,15},
    {  2,11,15}, {  2,10,15}, {  2, 9,15}, {  2, 8,15},
    {  1,31,14}, {  1,31,14}, {  1,30,14}, {  1,30,14},
    {  1,29,14}, {  1,29,14}, {  1,28,14}, {  1,28,14},
    {  1,27,14}, {  1,27,14}, {  1,26,14}, {  1,26,14},
    {  1,25,14}, {  1,25,14}, {  1,24,14}, {  1,24,14},
    {  1,23,14}, {  1,23,14}, {  1,22,14}, {  1,22,14},
    {  1,21,14}, {  1,21,14}, {  1,20,14}, {  1,20,14},
    {  1,19,14}, {  1,19,14}, {  1,18,14}, {  1,18,14},
    {  1,17,14}, {  1,17,14}, {  1,16,14}, {  1,16,14}
};

static const DCTtab DCT_13 [] = {
    { 11, 2,13}, { 10, 2,13}, {  6, 3,13}, {  4, 4,13},
    {  3, 5,13}, {  2, 7,13}, {  2, 6,13}, {  1,15,13},
    {  1,14,13}, {  1,13,13}, {  1,12,13}, { 27, 1,13},
    { 26, 1,13}, { 25, 1,13}, { 24, 1,13}, { 23, 1,13},
    {  1,11,12}, {  1,11,12}, {  9, 2,12}, {  9, 2,12},
    {  5, 3,12}, {  5, 3,12}, {  1,10,12}, {  1,10,12},
    {  3, 4,12}, {  3, 4,12}, {  8, 2,12}, {  8, 2,12},
    { 22, 1,12}, { 22, 1,12}, { 21, 1,12}, { 21, 1,12},
    {  1, 9,12}, {  1, 9,12}, { 20, 1,12}, { 20, 1,12},
    { 19, 1,12}, { 19, 1,12}, {  2, 5,12}, {  2, 5,12},
    {  4, 3,12}, {  4, 3,12}, {  1, 8,12}, {  1, 8,12},
    {  7, 2,12}, {  7, 2,12}, { 18, 1,12}, { 18, 1,12}
};

static const DCTtab DCT_B14_10 [] = {
    { 17, 1,10}, {  6, 2,10}, {  1, 7,10}, {  3, 3,10},
    {  2, 4,10}, { 16, 1,10}, { 15, 1,10}, {  5, 2,10}
};

static const DCTtab DCT_B14_8 [] = {
    { 65, 0,12}, { 65, 0,12}, { 65, 0,12}, { 65, 0,12},
    {  3, 2, 7}, {  3, 2, 7}, { 10, 1, 7}, { 10, 1, 7},
    {  1, 4, 7}, {  1, 4, 7}, {  9, 1, 7}, {  9, 1, 7},
    {  8, 1, 6}, {  8, 1, 6}, {  8, 1, 6}, {  8, 1, 6},
    {  7, 1, 6}, {  7, 1, 6}, {  7, 1, 6}, {  7, 1, 6},
    {  2, 2, 6}, {  2, 2, 6}, {  2, 2, 6}, {  2, 2, 6},
    {  6, 1, 6}, {  6, 1, 6}, {  6, 1, 6}, {  6, 1, 6},
    { 14, 1, 8}, {  1, 6, 8}, { 13, 1, 8}, { 12, 1, 8},
    {  4, 2, 8}, {  2, 3, 8}, {  1, 5, 8}, { 11, 1, 8}
};

static const DCTtab DCT_B14AC_5 [] = {
    {  1, 3, 5}, {  5, 1, 5}, {  4, 1, 5},
    {  1, 2, 4}, {  1, 2, 4}, {  3, 1, 4}, {  3, 1, 4},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {129, 0, 2}, {129, 0, 2}, {129, 0, 2}, {129, 0, 2},
    {129, 0, 2}, {129, 0, 2}, {129, 0, 2}, {129, 0, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}
};

static const DCTtab DCT_B14DC_5 [] = {
    {  1, 3, 5}, {  5, 1, 5}, {  4, 1, 5},
    {  1, 2, 4}, {  1, 2, 4}, {  3, 1, 4}, {  3, 1, 4},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1},
    {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1},
    {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1},
    {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1}
};

static const DCTtab DCT_B15_10 [] = {
    {  6, 2, 9}, {  6, 2, 9}, { 15, 1, 9}, { 15, 1, 9},
    {  3, 4,10}, { 17, 1,10}, { 16, 1, 9}, { 16, 1, 9}
};

static const DCTtab DCT_B15_8 [] = {
    { 65, 0,12}, { 65, 0,12}, { 65, 0,12}, { 65, 0,12},
    {  8, 1, 7}, {  8, 1, 7}, {  9, 1, 7}, {  9, 1, 7},
    {  7, 1, 7}, {  7, 1, 7}, {  3, 2, 7}, {  3, 2, 7},
    {  1, 7, 6}, {  1, 7, 6}, {  1, 7, 6}, {  1, 7, 6},
    {  1, 6, 6}, {  1, 6, 6}, {  1, 6, 6}, {  1, 6, 6},
    {  5, 1, 6}, {  5, 1, 6}, {  5, 1, 6}, {  5, 1, 6},
    {  6, 1, 6}, {  6, 1, 6}, {  6, 1, 6}, {  6, 1, 6},
    {  2, 5, 8}, { 12, 1, 8}, {  1,11, 8}, {  1,10, 8},
    { 14, 1, 8}, { 13, 1, 8}, {  4, 2, 8}, {  2, 4, 8},
    {  3, 1, 5}, {  3, 1, 5}, {  3, 1, 5}, {  3, 1, 5},
    {  3, 1, 5}, {  3, 1, 5}, {  3, 1, 5}, {  3, 1, 5},
    {  2, 2, 5}, {  2, 2, 5}, {  2, 2, 5}, {  2, 2, 5},
    {  2, 2, 5}, {  2, 2, 5}, {  2, 2, 5}, {  2, 2, 5},
    {  4, 1, 5}, {  4, 1, 5}, {  4, 1, 5}, {  4, 1, 5},
    {  4, 1, 5}, {  4, 1, 5}, {  4, 1, 5}, {  4, 1, 5},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {129, 0, 4}, {129, 0, 4}, {129, 0, 4}, {129, 0, 4},
    {129, 0, 4}, {129, 0, 4}, {129, 0, 4}, {129, 0, 4},
    {129, 0, 4}, {129, 0, 4}, {129, 0, 4}, {129, 0, 4},
    {129, 0, 4}, {129, 0, 4}, {129, 0, 4}, {129, 0, 4},
    {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4},
    {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4},
    {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4},
    {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
    {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
    {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
    {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
    {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
    {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
    {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
    {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
    {  1, 4, 5}, {  1, 4, 5}, {  1, 4, 5}, {  1, 4, 5},
    {  1, 4, 5}, {  1, 4, 5}, {  1, 4, 5}, {  1, 4, 5},
    {  1, 5, 5}, {  1, 5, 5}, {  1, 5, 5}, {  1, 5, 5},
    {  1, 5, 5}, {  1, 5, 5}, {  1, 5, 5}, {  1, 5, 5},
    { 10, 1, 7}, { 10, 1, 7}, {  2, 3, 7}, {  2, 3, 7},
    { 11, 1, 7}, { 11, 1, 7}, {  1, 8, 7}, {  1, 8, 7},
    {  1, 9, 7}, {  1, 9, 7}, {  1,12, 8}, {  1,13, 8},
    {  3, 3, 8}, {  5, 2, 8}, {  1,14, 8}, {  1,15, 8}
};

static const MBAtab MBA_5 [] = {
    {6, 5}, {5, 5}, {4, 4}, {4, 4}, {3, 4}, {3, 4},
    {2, 3}, {2, 3}, {2, 3}, {2, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3},
    {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1},
    {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}
};

static const MBAtab MBA_11 [] = {
    {32, 11}, {31, 11}, {30, 11}, {29, 11},
    {28, 11}, {27, 11}, {26, 11}, {25, 11},
    {24, 11}, {23, 11}, {22, 11}, {21, 11},
    {20, 10}, {20, 10}, {19, 10}, {19, 10},
    {18, 10}, {18, 10}, {17, 10}, {17, 10},
    {16, 10}, {16, 10}, {15, 10}, {15, 10},
    {14,  8}, {14,  8}, {14,  8}, {14,  8},
    {14,  8}, {14,  8}, {14,  8}, {14,  8},
    {13,  8}, {13,  8}, {13,  8}, {13,  8},
    {13,  8}, {13,  8}, {13,  8}, {13,  8},
    {12,  8}, {12,  8}, {12,  8}, {12,  8},
    {12,  8}, {12,  8}, {12,  8}, {12,  8},
    {11,  8}, {11,  8}, {11,  8}, {11,  8},
    {11,  8}, {11,  8}, {11,  8}, {11,  8},
    {10,  8}, {10,  8}, {10,  8}, {10,  8},
    {10,  8}, {10,  8}, {10,  8}, {10,  8},
    { 9,  8}, { 9,  8}, { 9,  8}, { 9,  8},
    { 9,  8}, { 9,  8}, { 9,  8}, { 9,  8},
    { 8,  7}, { 8,  7}, { 8,  7}, { 8,  7},
    { 8,  7}, { 8,  7}, { 8,  7}, { 8,  7},
    { 8,  7}, { 8,  7}, { 8,  7}, { 8,  7},
    { 8,  7}, { 8,  7}, { 8,  7}, { 8,  7},
    { 7,  7}, { 7,  7}, { 7,  7}, { 7,  7},
    { 7,  7}, { 7,  7}, { 7,  7}, { 7,  7},
    { 7,  7}, { 7,  7}, { 7,  7}, { 7,  7},
    { 7,  7}, { 7,  7}, { 7,  7}, { 7,  7}
};

static inline int get_macroblock_modes (mpeg2_decoder_t * const decoder)
{
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)

    uint32_t *bit_buf = &decoder->bitstream_buf;
    int macroblock_modes;
    const MBtab * tab;

    switch (decoder->coding_type)
    {
    case I_TYPE:
        tab = MB_I + decoder->ubits(1);
        decoder->dumpbits(tab->len);
        macroblock_modes = tab->modes;

        if ((! (decoder->frame_pred_frame_dct)) &&
            (decoder->picture_structure == FRAME_PICTURE))
        {
            macroblock_modes |= decoder->ubits(1) * DCT_TYPE_INTERLACED;
            decoder->dumpbits(1);
        }

        return macroblock_modes;
    case P_TYPE:
        tab = MB_P + decoder->ubits(5);
        dumpbits (*bit_buf, bits, tab->len);
        macroblock_modes = tab->modes;

        if (decoder->picture_structure != FRAME_PICTURE) {
	    if (macroblock_modes & MACROBLOCK_MOTION_FORWARD) {
		macroblock_modes |= UBITS (*bit_buf, 2) << MOTION_TYPE_SHIFT;
		dumpbits (*bit_buf, bits, 2);
	    }
	    return macroblock_modes | MACROBLOCK_MOTION_FORWARD;
        } else if (decoder->frame_pred_frame_dct) {
	    if (macroblock_modes & MACROBLOCK_MOTION_FORWARD)
		macroblock_modes |= MC_FRAME << MOTION_TYPE_SHIFT;
	    return macroblock_modes | MACROBLOCK_MOTION_FORWARD;
        } else {
	    if (macroblock_modes & MACROBLOCK_MOTION_FORWARD) {
		macroblock_modes |= UBITS (*bit_buf, 2) << MOTION_TYPE_SHIFT;
		dumpbits (*bit_buf, bits, 2);
	    }
	    if (macroblock_modes & (MACROBLOCK_INTRA | MACROBLOCK_PATTERN)) {
		macroblock_modes |= UBITS (*bit_buf, 1) * DCT_TYPE_INTERLACED;
		dumpbits (*bit_buf, bits, 1);
	    }
	    return macroblock_modes | MACROBLOCK_MOTION_FORWARD;
        }
    case B_TYPE:
        tab = MB_B + decoder->ubits(6);
        dumpbits (*bit_buf, bits, tab->len);
        macroblock_modes = tab->modes;

        if (decoder->picture_structure != FRAME_PICTURE) {
            if (! (macroblock_modes & MACROBLOCK_INTRA))
            {
                macroblock_modes |= UBITS (*bit_buf, 2) << MOTION_TYPE_SHIFT;
                dumpbits (*bit_buf, bits, 2);
            }
            return macroblock_modes;
        } else if (decoder->frame_pred_frame_dct) {
	    /* if (! (macroblock_modes & MACROBLOCK_INTRA)) */
	    macroblock_modes |= MC_FRAME << MOTION_TYPE_SHIFT;
	    return macroblock_modes;
        } else
        {
	    if (macroblock_modes & MACROBLOCK_INTRA)
            goto intra;
        macroblock_modes |= UBITS (*bit_buf, 2) << MOTION_TYPE_SHIFT;
        dumpbits(*bit_buf, bits, 2);

	    if (macroblock_modes & (MACROBLOCK_INTRA | MACROBLOCK_PATTERN))
        {
intra:
            macroblock_modes |= UBITS (*bit_buf, 1) * DCT_TYPE_INTERLACED;
            dumpbits(*bit_buf, bits, 1);
	    }
	    return macroblock_modes;
        }
    case D_TYPE:
        dumpbits(*bit_buf, bits, 1);
        return MACROBLOCK_INTRA;
    default:
        return 0;
    }
#undef bits
#undef bit_ptr
}

static inline void get_quantizer_scale(mpeg2_decoder_t * const decoder)
{
    int quantizer_scale_code = decoder->ubits(5);
    decoder->dumpbits(5);
    decoder->quantizer_matrix[0] = decoder->quantizer_prescale[0][quantizer_scale_code];
    decoder->quantizer_matrix[1] = decoder->quantizer_prescale[1][quantizer_scale_code];
    decoder->quantizer_matrix[2] = decoder->chroma_quantizer[0][quantizer_scale_code];
    decoder->quantizer_matrix[3] = decoder->chroma_quantizer[1][quantizer_scale_code];
}

/* take num bits from the high part of bit_buf and sign extend them */
#define SBITS(bit_buf,num) (((int32_t)(bit_buf)) >> (32 - (num)))


static inline int get_motion_delta (mpeg2_decoder_t * const decoder,
				    const int f_code)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)

    int delta;
    int sign;
    const MVtab * tab;

    if (bit_buf & 0x80000000) {
        decoder->dumpbits(1);
        return 0;
    }

    if (bit_buf >= 0x0c000000)
    {
        tab = MV_4 + UBITS (bit_buf, 4);
        delta = (tab->delta << f_code) + 1;
        bits += tab->len + f_code + 1;
        bit_buf <<= tab->len;
        sign = SBITS (bit_buf, 1);
        bit_buf <<= 1;

        if (f_code)
            delta += decoder->ubits(f_code);

        bit_buf <<= f_code;
        return (delta ^ sign) - sign;

    }

	tab = MV_10 + UBITS (bit_buf, 10);
	delta = (tab->delta << f_code) + 1;
	bits += tab->len + 1;
	bit_buf <<= tab->len;

	sign = SBITS (bit_buf, 1);
	bit_buf <<= 1;

	if (f_code) {
	    NEEDBITS (bit_buf, bits, bit_ptr);
	    delta += UBITS (bit_buf, f_code);
	    dumpbits (bit_buf, bits, f_code);
	}

    return (delta ^ sign) - sign;
#undef bit_buf
#undef bits
#undef bit_ptr
}

static inline int bound_motion_vector (const int vector, const int f_code)
{
    return ((int32_t)vector << 27 - f_code) >> 27 - f_code;
}

static inline int get_dmv (mpeg2_decoder_t * const decoder)
{
    const DMVtab *tab = DMV_2 + decoder->ubits(2);
    decoder->dumpbits(tab->len);
    return tab->dmv;
}

static inline int get_coded_block_pattern (mpeg2_decoder_t * const decoder)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)

    const CBPtab * tab;

    NEEDBITS (bit_buf, bits, bit_ptr);

    if (bit_buf >= 0x20000000) {

        tab = CBP_7 + (decoder->ubits(7) - 16);
        decoder->dumpbits(tab->len);
        return tab->cbp;

    }

    tab = CBP_9 + decoder->ubits(9);
    decoder->dumpbits(tab->len);
	return tab->cbp;
#undef bit_buf
#undef bits
#undef bit_ptr
}

static inline int get_luma_dc_dct_diff (mpeg2_decoder_t * const decoder)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)
    const DCtab * tab;
    int size;
    int dc_diff;

    if (bit_buf < 0xf8000000)
    {
        tab = DC_lum_5 + UBITS (bit_buf, 5);
        size = tab->size;
        if (size) {
            bits += tab->len + size;
            bit_buf <<= tab->len;
            dc_diff = decoder->ubits(size) - UBITS (SBITS (~bit_buf, 1), size);
            bit_buf <<= size;
            return dc_diff << decoder->intra_dc_precision;
        }
        dumpbits (bit_buf, bits, 3);
        return 0;
    }
    tab = DC_long + (UBITS (bit_buf, 9) - 0x1e0);
    size = tab->size;
    dumpbits (bit_buf, bits, tab->len);
    NEEDBITS (bit_buf, bits, bit_ptr);
    dc_diff = UBITS (bit_buf, size) - UBITS (SBITS (~bit_buf, 1), size);
    dumpbits (bit_buf, bits, size);
    return dc_diff << decoder->intra_dc_precision;
#undef bit_buf
#undef bits
#undef bit_ptr
}

static inline int get_chroma_dc_dct_diff(mpeg2_decoder_t * const decoder)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)
    const DCtab * tab;
    int size;
    int dc_diff;

    if (bit_buf < 0xf8000000)
    {
        tab = DC_chrom_5 + UBITS (bit_buf, 5);
        size = tab->size;
        if (size)
        {
            bits += tab->len + size;
            bit_buf <<= tab->len;
            dc_diff = UBITS (bit_buf, size) - UBITS (SBITS (~bit_buf, 1), size);
            bit_buf <<= size;
            return dc_diff << decoder->intra_dc_precision;
        }
        dumpbits (bit_buf, bits, 2);
        return 0;
    }
    tab = DC_long + (UBITS (bit_buf, 10) - 0x3e0);
    size = tab->size;
    dumpbits (bit_buf, bits, tab->len + 1);
    NEEDBITS (bit_buf, bits, bit_ptr);
    dc_diff = UBITS (bit_buf, size) - UBITS (SBITS (~bit_buf, 1), size);
    dumpbits (bit_buf, bits, size);
    return dc_diff << decoder->intra_dc_precision;
   
#undef bit_buf
#undef bits
#undef bit_ptr
}

#define SATURATE(val)				\
do {						\
    val <<= 4;					\
    if (unlikely (val != (int16_t) val))	\
	val = (SBITS (val, 1) ^ 2047) << 4;	\
} while (0)

static void get_intra_block_B14 (mpeg2_decoder_t * const decoder,
				 const uint16_t * const quant_matrix)
{
    int j;
    int val;
    const uint8_t * const scan = decoder->scan;
    const DCTtab * tab;
    uint32_t bit_buf;
    int bits;
    const uint8_t * bit_ptr;
    int16_t * const dest = decoder->DCTblock;

    int i = 0;
    int mismatch = ~dest[0];

    bit_buf = decoder->bitstream_buf;
    bits = decoder->bitstream_bits;
    bit_ptr = decoder->bitstream_ptr;

    NEEDBITS (bit_buf, bits, bit_ptr);

    while (1) {
	if (bit_buf >= 0x28000000) {

	    tab = DCT_B14AC_5 + (UBITS (bit_buf, 5) - 5);

	    i += tab->run;
	    if (i >= 64)
		break;	/* end of block */
normal_code:
	    j = scan[i];
	    bit_buf <<= tab->len;
	    bits += tab->len + 1;
	    val = (tab->level * quant_matrix[j]) >> 4;

	    /* if (bitstream_get (1)) val = -val; */
	    val = (val ^ SBITS (bit_buf, 1)) - SBITS (bit_buf, 1);

	    SATURATE (val);
	    dest[j] = val;
	    mismatch ^= val;

	    bit_buf <<= 1;
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x04000000) {

	    tab = DCT_B14_8 + (UBITS (bit_buf, 8) - 4);

	    i += tab->run;
        if (i < 64)
            goto normal_code;

	    /* escape code */

	    i += UBITS (bit_buf << 6, 6) - 64;
	    if (i >= 64)
		break;	/* illegal, check needed to avoid buffer overflow */

	    j = scan[i];

	    dumpbits (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);
	    val = (SBITS (bit_buf, 12) * quant_matrix[j]) / 16;

	    SATURATE (val);
	    dest[j] = val;
	    mismatch ^= val;

	    dumpbits (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B14_10 + (UBITS (bit_buf, 10) - 8);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    GETWORD (bit_buf, bits + 16, bit_ptr);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	}
	break;	/* illegal, check needed to avoid buffer overflow */
    }
    dest[63] ^= mismatch & 16;
    dumpbits (bit_buf, bits, tab->len);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
    decoder->bitstream_ptr = bit_ptr;
}

static void get_intra_block_B15 (mpeg2_decoder_t * const decoder,
				 const uint16_t * const quant_matrix)
{
    int i;
    int j;
    int val;
    const uint8_t * const scan = decoder->scan;
    int mismatch;
    const DCTtab * tab;
    uint32_t bit_buf;
    int bits;
    const uint8_t * bit_ptr;
    int16_t * const dest = decoder->DCTblock;

    i = 0;
    mismatch = ~dest[0];

    bit_buf = decoder->bitstream_buf;
    bits = decoder->bitstream_bits;
    bit_ptr = decoder->bitstream_ptr;

    NEEDBITS (bit_buf, bits, bit_ptr);

    while (1) {
	if (bit_buf >= 0x04000000) {

	    tab = DCT_B15_8 + (UBITS (bit_buf, 8) - 4);

	    i += tab->run;
	    if (i < 64) {

normal_code:
		j = scan[i];
		bit_buf <<= tab->len;
		bits += tab->len + 1;
		val = (tab->level * quant_matrix[j]) >> 4;

		/* if (bitstream_get (1)) val = -val; */
		val = (val ^ SBITS (bit_buf, 1)) - SBITS (bit_buf, 1);

		SATURATE (val);
		dest[j] = val;
		mismatch ^= val;

		bit_buf <<= 1;
		NEEDBITS (bit_buf, bits, bit_ptr);

		continue;

	    } else {

		/* end of block. I commented out this code because if we */
		/* do not exit here we will still exit at the later test :) */

		/* if (i >= 128) break;	*/	/* end of block */

		/* escape code */

		i += UBITS (bit_buf << 6, 6) - 64;
		if (i >= 64)
		    break;	/* illegal, check against buffer overflow */

		j = scan[i];

		dumpbits (bit_buf, bits, 12);
		NEEDBITS (bit_buf, bits, bit_ptr);
		val = (SBITS (bit_buf, 12) * quant_matrix[j]) / 16;

		SATURATE (val);
		dest[j] = val;
		mismatch ^= val;

		dumpbits (bit_buf, bits, 12);
		NEEDBITS (bit_buf, bits, bit_ptr);

		continue;

	    }
	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B15_10 + (UBITS (bit_buf, 10) - 8);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    GETWORD (bit_buf, bits + 16, bit_ptr);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	}
	break;	/* illegal, check needed to avoid buffer overflow */
    }
    dest[63] ^= mismatch & 16;
    dumpbits (bit_buf, bits, tab->len);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
    decoder->bitstream_ptr = bit_ptr;
}

static int get_non_intra_block (mpeg2_decoder_t * const decoder,
				const uint16_t * const quant_matrix)
{
    int j;
    int val;
    const uint8_t * const scan = decoder->scan;
    int mismatch;
    const DCTtab * tab;
    uint32_t bit_buf;
    int bits;
    const uint8_t * bit_ptr;
    int16_t * const dest = decoder->DCTblock;

    int i = -1;
    mismatch = -1;

    bit_buf = decoder->bitstream_buf;
    bits = decoder->bitstream_bits;
    bit_ptr = decoder->bitstream_ptr;

    NEEDBITS (bit_buf, bits, bit_ptr);
    if (bit_buf >= 0x28000000) {
	tab = DCT_B14DC_5 + (UBITS (bit_buf, 5) - 5);
	goto entry_1;
    } else
	goto entry_2;

    while (1) {
	if (bit_buf >= 0x28000000) {

	    tab = DCT_B14AC_5 + (UBITS (bit_buf, 5) - 5);

	entry_1:
	    i += tab->run;
	    if (i >= 64)
		break;	/* end of block */

	normal_code:
	    j = scan[i];
	    bit_buf <<= tab->len;
	    bits += tab->len + 1;
	    val = ((2 * tab->level + 1) * quant_matrix[j]) >> 5;

	    /* if (bitstream_get (1)) val = -val; */
	    val = (val ^ SBITS (bit_buf, 1)) - SBITS (bit_buf, 1);

	    SATURATE (val);
	    dest[j] = val;
	    mismatch ^= val;

	    bit_buf <<= 1;
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	}

entry_2:
	if (bit_buf >= 0x04000000) {

	    tab = DCT_B14_8 + (UBITS (bit_buf, 8) - 4);

	    i += tab->run;
	    if (i < 64)
		goto normal_code;

	    /* escape code */

	    i += UBITS (bit_buf << 6, 6) - 64;
	    if (i >= 64)
		break;	/* illegal, check needed to avoid buffer overflow */

	    j = scan[i];

	    dumpbits (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);
	    val = 2 * (SBITS (bit_buf, 12) + SBITS (bit_buf, 1)) + 1;
	    val = (val * quant_matrix[j]) / 32;

	    SATURATE (val);
	    dest[j] = val;
	    mismatch ^= val;

	    dumpbits (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

    } else if (bit_buf >= 0x02000000) {
        tab = DCT_B14_10 + (UBITS (bit_buf, 10) - 8);
        i += tab->run;
        if (i < 64)
            goto normal_code;
    } else if (bit_buf >= 0x00800000) {
        tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
        i += tab->run;
        if (i < 64)
            goto normal_code;
    } else if (bit_buf >= 0x00200000) {
        tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
        i += tab->run;
        if (i < 64)
            goto normal_code;
    } else {
        tab = DCT_16 + UBITS (bit_buf, 16);
        bit_buf <<= 16;
        GETWORD (bit_buf, bits + 16, bit_ptr);
        i += tab->run;
        if (i < 64)
        goto normal_code;
    }
	break;	/* illegal, check needed to avoid buffer overflow */
    }
    dest[63] ^= mismatch & 16;
    dumpbits (bit_buf, bits, tab->len);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
    decoder->bitstream_ptr = bit_ptr;
    return i;
}

static void get_mpeg1_intra_block (mpeg2_decoder_t * const decoder)
{
    int j;
    int val;
    const uint8_t * const scan = decoder->scan;
    const uint16_t * const quant_matrix = decoder->quantizer_matrix[0];
    const DCTtab * tab;
    uint32_t bit_buf;
    int bits;
    const uint8_t * bit_ptr;
    int16_t * const dest = decoder->DCTblock;

    int i = 0;

    bit_buf = decoder->bitstream_buf;
    bits = decoder->bitstream_bits;
    bit_ptr = decoder->bitstream_ptr;

    NEEDBITS (bit_buf, bits, bit_ptr);

    while (1) {
	if (bit_buf >= 0x28000000) {

	    tab = DCT_B14AC_5 + (UBITS (bit_buf, 5) - 5);

	    i += tab->run;
	    if (i >= 64)
		break;	/* end of block */

	normal_code:
	    j = scan[i];
	    bit_buf <<= tab->len;
	    bits += tab->len + 1;
	    val = (tab->level * quant_matrix[j]) >> 4;

	    /* oddification */
	    val = (val - 1) | 1;

	    /* if (bitstream_get (1)) val = -val; */
	    val = (val ^ SBITS (bit_buf, 1)) - SBITS (bit_buf, 1);

	    SATURATE (val);
	    dest[j] = val;

	    bit_buf <<= 1;
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x04000000) {

	    tab = DCT_B14_8 + (UBITS (bit_buf, 8) - 4);

	    i += tab->run;
	    if (i < 64)
		goto normal_code;

	    /* escape code */

	    i += UBITS (bit_buf << 6, 6) - 64;
	    if (i >= 64)
		break;	/* illegal, check needed to avoid buffer overflow */

	    j = scan[i];

	    dumpbits (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);
	    val = SBITS (bit_buf, 8);
	    if (! (val & 0x7f)) {
		dumpbits (bit_buf, bits, 8);
		val = UBITS (bit_buf, 8) + 2 * val;
	    }
	    val = (val * quant_matrix[j]) / 16;

	    /* oddification */
	    val = (val + ~SBITS (val, 1)) | 1;

	    SATURATE (val);
	    dest[j] = val;

	    dumpbits (bit_buf, bits, 8);
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B14_10 + (UBITS (bit_buf, 10) - 8);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    GETWORD (bit_buf, bits + 16, bit_ptr);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	}
	break;	/* illegal, check needed to avoid buffer overflow */
    }
    dumpbits (bit_buf, bits, tab->len);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
    decoder->bitstream_ptr = bit_ptr;
}

static int get_mpeg1_non_intra_block (mpeg2_decoder_t * const decoder)
{
    int j;
    int val;
    const uint8_t * const scan = decoder->scan;
    const uint16_t * const quant_matrix = decoder->quantizer_matrix[1];
    const DCTtab * tab;
    uint32_t bit_buf;
    int bits;
    const uint8_t * bit_ptr;
    int16_t * const dest = decoder->DCTblock;

    int i = -1;

    bit_buf = decoder->bitstream_buf;
    bits = decoder->bitstream_bits;
    bit_ptr = decoder->bitstream_ptr;

    NEEDBITS (bit_buf, bits, bit_ptr);
    if (bit_buf >= 0x28000000) {
        tab = DCT_B14DC_5 + (UBITS (bit_buf, 5) - 5);
        goto entry_1;
    } else {
        goto entry_2;
    }

    while (1) {
	if (bit_buf >= 0x28000000) {

	    tab = DCT_B14AC_5 + (UBITS (bit_buf, 5) - 5);

entry_1:
	    i += tab->run;
	    if (i >= 64)
		break;	/* end of block */

normal_code:
	    j = scan[i];
	    bit_buf <<= tab->len;
	    bits += tab->len + 1;
	    val = ((2 * tab->level + 1) * quant_matrix[j]) >> 5;

	    /* oddification */
	    val = (val - 1) | 1;

	    val = (val ^ SBITS (bit_buf, 1)) - SBITS (bit_buf, 1);

	    SATURATE (val);
	    dest[j] = val;

	    bit_buf <<= 1;
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	}

    entry_2:
	if (bit_buf >= 0x04000000) {

	    tab = DCT_B14_8 + (UBITS (bit_buf, 8) - 4);

	    i += tab->run;
	    if (i < 64)
		goto normal_code;

	    /* escape code */

	    i += UBITS (bit_buf << 6, 6) - 64;
	    if (i >= 64)
		break;	/* illegal, check needed to avoid buffer overflow */

	    j = scan[i];

	    dumpbits (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);
	    val = SBITS (bit_buf, 8);
	    if (! (val & 0x7f)) {
		dumpbits (bit_buf, bits, 8);
		val = UBITS (bit_buf, 8) + 2 * val;
	    }
	    val = 2 * (val + SBITS (val, 1)) + 1;
	    val = (val * quant_matrix[j]) / 32;

	    /* oddification */
	    val = (val + ~SBITS (val, 1)) | 1;

	    SATURATE (val);
	    dest[j] = val;

	    dumpbits (bit_buf, bits, 8);
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B14_10 + (UBITS (bit_buf, 10) - 8);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    GETWORD (bit_buf, bits + 16, bit_ptr);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	}
	break;	/* illegal, check needed to avoid buffer overflow */
    }
    dumpbits (bit_buf, bits, tab->len);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
    decoder->bitstream_ptr = bit_ptr;
    return i;
}

static inline void slice_intra_DCT (mpeg2_decoder_t * const decoder,
				    const int cc,
				    uint8_t * const dest, const int stride)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)
    NEEDBITS (bit_buf, bits, bit_ptr);
    /* Get the intra DC coefficient and inverse quantize it */
    if (cc == 0)
	decoder->DCTblock[0] =
	    decoder->dc_dct_pred[0] += get_luma_dc_dct_diff (decoder);
    else
	decoder->DCTblock[0] =
	    decoder->dc_dct_pred[cc] += get_chroma_dc_dct_diff (decoder);

    if (decoder->mpeg1) {
	if (decoder->coding_type != D_TYPE)
	    get_mpeg1_intra_block (decoder);
    } else if (decoder->intra_vlc_format)
        get_intra_block_B15 (decoder, decoder->quantizer_matrix[cc ? 2 : 0]);
    else
        get_intra_block_B14 (decoder, decoder->quantizer_matrix[cc ? 2 : 0]);
    mpeg2_idct_copy (decoder->DCTblock, dest, stride);
#undef bit_buf
#undef bits
#undef bit_ptr
}

static inline void
slice_non_intra_DCT(mpeg2_decoder_t * const decoder,
        const int cc, uint8_t * const dest, const int stride)
{
    int last;

    if (decoder->mpeg1)
        last = get_mpeg1_non_intra_block (decoder);
    else
        last = get_non_intra_block(decoder, decoder->quantizer_matrix[cc ? 3 : 1]);

    mpeg2_idct_add (last, decoder->DCTblock, dest, stride);
}

#define MOTION_420(table,ref,motion_x,motion_y,size,y)			      \
    pos_x = 2 * decoder->offset + motion_x;				      \
    pos_y = 2 * decoder->v_offset + motion_y + 2 * y;			      \
    if (unlikely (pos_x > decoder->limit_x)) {				      \
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		      \
	motion_x = pos_x - 2 * decoder->offset;				      \
    }									      \
    if (unlikely (pos_y > decoder->limit_y_ ## size)) {			      \
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y_ ## size;	      \
	motion_y = pos_y - 2 * decoder->v_offset - 2 * y;		      \
    }									      \
    xy_half = ((pos_y & 1) << 1) | (pos_x & 1);				      \
    table[xy_half] (decoder->dest[0] + y * decoder->stride + decoder->offset, \
		    ref[0] + (pos_x >> 1) + (pos_y >> 1) * decoder->stride,   \
		    decoder->stride, size);				      \
    motion_x /= 2;	motion_y /= 2;					      \
    xy_half = ((motion_y & 1) << 1) | (motion_x & 1);			      \
    offset = (((decoder->offset + motion_x) >> 1) +			      \
	      ((((decoder->v_offset + motion_y) >> 1) + y/2) *		      \
	       decoder->uv_stride));					      \
    table[4+xy_half] (decoder->dest[1] + y/2 * decoder->uv_stride +	      \
		      (decoder->offset >> 1), ref[1] + offset,		      \
		      decoder->uv_stride, size/2);			      \
    table[4+xy_half] (decoder->dest[2] + y/2 * decoder->uv_stride +	      \
		      (decoder->offset >> 1), ref[2] + offset,		      \
		      decoder->uv_stride, size/2)

#define MOTION_FIELD_420(table,ref,motion_x,motion_y,dest_field,op,src_field) \
    pos_x = 2 * decoder->offset + motion_x;				      \
    pos_y = decoder->v_offset + motion_y;				      \
    if (unlikely (pos_x > decoder->limit_x)) {				      \
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		      \
	motion_x = pos_x - 2 * decoder->offset;				      \
    }									      \
    if (unlikely (pos_y > decoder->limit_y)) {				      \
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y;		      \
	motion_y = pos_y - decoder->v_offset;				      \
    }									      \
    xy_half = ((pos_y & 1) << 1) | (pos_x & 1);				      \
    table[xy_half] (decoder->dest[0] + dest_field * decoder->stride +	      \
		    decoder->offset,					      \
		    (ref[0] + (pos_x >> 1) +				      \
		     ((pos_y op) + src_field) * decoder->stride),	      \
		    2 * decoder->stride, 8);				      \
    motion_x /= 2;	motion_y /= 2;					      \
    xy_half = ((motion_y & 1) << 1) | (motion_x & 1);			      \
    offset = (((decoder->offset + motion_x) >> 1) +			      \
	      (((decoder->v_offset >> 1) + (motion_y op) + src_field) *	      \
	       decoder->uv_stride));					      \
    table[4+xy_half] (decoder->dest[1] + dest_field * decoder->uv_stride +    \
		      (decoder->offset >> 1), ref[1] + offset,		      \
		      2 * decoder->uv_stride, 4);			      \
    table[4+xy_half] (decoder->dest[2] + dest_field * decoder->uv_stride +    \
		      (decoder->offset >> 1), ref[2] + offset,		      \
		      2 * decoder->uv_stride, 4)

#define MOTION_DMV_420(table,ref,motion_x,motion_y)			      \
    pos_x = 2 * decoder->offset + motion_x;				      \
    pos_y = decoder->v_offset + motion_y;				      \
    if (unlikely (pos_x > decoder->limit_x)) {				      \
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		      \
	motion_x = pos_x - 2 * decoder->offset;				      \
    }									      \
    if (unlikely (pos_y > decoder->limit_y)) {				      \
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y;		      \
	motion_y = pos_y - decoder->v_offset;				      \
    }									      \
    xy_half = ((pos_y & 1) << 1) | (pos_x & 1);				      \
    offset = (pos_x >> 1) + (pos_y & ~1) * decoder->stride;		      \
    table[xy_half] (decoder->dest[0] + decoder->offset,			      \
		    ref[0] + offset, 2 * decoder->stride, 8);		      \
    table[xy_half] (decoder->dest[0] + decoder->stride + decoder->offset,     \
		    ref[0] + decoder->stride + offset,			      \
		    2 * decoder->stride, 8);				      \
    motion_x /= 2;	motion_y /= 2;					      \
    xy_half = ((motion_y & 1) << 1) | (motion_x & 1);			      \
    offset = (((decoder->offset + motion_x) >> 1) +			      \
	      (((decoder->v_offset >> 1) + (motion_y & ~1)) *		      \
	       decoder->uv_stride));					      \
    table[4+xy_half] (decoder->dest[1] + (decoder->offset >> 1),	      \
		      ref[1] + offset, 2 * decoder->uv_stride, 4);	      \
    table[4+xy_half] (decoder->dest[1] + decoder->uv_stride +		      \
		      (decoder->offset >> 1),				      \
		      ref[1] + decoder->uv_stride + offset,		      \
		      2 * decoder->uv_stride, 4);			      \
    table[4+xy_half] (decoder->dest[2] + (decoder->offset >> 1),	      \
		      ref[2] + offset, 2 * decoder->uv_stride, 4);	      \
    table[4+xy_half] (decoder->dest[2] + decoder->uv_stride +		      \
		      (decoder->offset >> 1),				      \
		      ref[2] + decoder->uv_stride + offset,		      \
		      2 * decoder->uv_stride, 4)

#define MOTION_ZERO_420(table,ref)					      \
    table[0] (decoder->dest[0] + decoder->offset,			      \
	      (ref[0] + decoder->offset +				      \
	       decoder->v_offset * decoder->stride), decoder->stride, 16);    \
    offset = ((decoder->offset >> 1) +					      \
	      (decoder->v_offset >> 1) * decoder->uv_stride);		      \
    table[4] (decoder->dest[1] + (decoder->offset >> 1),		      \
	      ref[1] + offset, decoder->uv_stride, 8);			      \
    table[4] (decoder->dest[2] + (decoder->offset >> 1),		      \
	      ref[2] + offset, decoder->uv_stride, 8)

#define MOTION_422(table,ref,motion_x,motion_y,size,y)			      \
    pos_x = 2 * decoder->offset + motion_x;				      \
    pos_y = 2 * decoder->v_offset + motion_y + 2 * y;			      \
    if (unlikely (pos_x > decoder->limit_x)) {				      \
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		      \
	motion_x = pos_x - 2 * decoder->offset;				      \
    }									      \
    if (unlikely (pos_y > decoder->limit_y_ ## size)) {			      \
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y_ ## size;	      \
	motion_y = pos_y - 2 * decoder->v_offset - 2 * y;		      \
    }									      \
    xy_half = ((pos_y & 1) << 1) | (pos_x & 1);				      \
    offset = (pos_x >> 1) + (pos_y >> 1) * decoder->stride;		      \
    table[xy_half] (decoder->dest[0] + y * decoder->stride + decoder->offset, \
		    ref[0] + offset, decoder->stride, size);		      \
    offset = (offset + (motion_x & (motion_x < 0))) >> 1;		      \
    motion_x /= 2;							      \
    xy_half = ((pos_y & 1) << 1) | (motion_x & 1);			      \
    table[4+xy_half] (decoder->dest[1] + y * decoder->uv_stride +	      \
		      (decoder->offset >> 1), ref[1] + offset,		      \
		      decoder->uv_stride, size);			      \
    table[4+xy_half] (decoder->dest[2] + y * decoder->uv_stride +	      \
		      (decoder->offset >> 1), ref[2] + offset,		      \
		      decoder->uv_stride, size)

#define MOTION_FIELD_422(table,ref,motion_x,motion_y,dest_field,op,src_field) \
    pos_x = 2 * decoder->offset + motion_x;				      \
    pos_y = decoder->v_offset + motion_y;				      \
    if (unlikely (pos_x > decoder->limit_x)) {				      \
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		      \
	motion_x = pos_x - 2 * decoder->offset;				      \
    }									      \
    if (unlikely (pos_y > decoder->limit_y)) {				      \
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y;		      \
	motion_y = pos_y - decoder->v_offset;				      \
    }									      \
    xy_half = ((pos_y & 1) << 1) | (pos_x & 1);				      \
    offset = (pos_x >> 1) + ((pos_y op) + src_field) * decoder->stride;	      \
    table[xy_half] (decoder->dest[0] + dest_field * decoder->stride +	      \
		    decoder->offset, ref[0] + offset,			      \
		    2 * decoder->stride, 8);				      \
    offset = (offset + (motion_x & (motion_x < 0))) >> 1;		      \
    motion_x /= 2;							      \
    xy_half = ((pos_y & 1) << 1) | (motion_x & 1);			      \
    table[4+xy_half] (decoder->dest[1] + dest_field * decoder->uv_stride +    \
		      (decoder->offset >> 1), ref[1] + offset,		      \
		      2 * decoder->uv_stride, 8);			      \
    table[4+xy_half] (decoder->dest[2] + dest_field * decoder->uv_stride +    \
		      (decoder->offset >> 1), ref[2] + offset,		      \
		      2 * decoder->uv_stride, 8)

#define MOTION_DMV_422(table,ref,motion_x,motion_y)			      \
    pos_x = 2 * decoder->offset + motion_x;				      \
    pos_y = decoder->v_offset + motion_y;				      \
    if (unlikely (pos_x > decoder->limit_x)) {				      \
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		      \
	motion_x = pos_x - 2 * decoder->offset;				      \
    }									      \
    if (unlikely (pos_y > decoder->limit_y)) {				      \
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y;		      \
	motion_y = pos_y - decoder->v_offset;				      \
    }									      \
    xy_half = ((pos_y & 1) << 1) | (pos_x & 1);				      \
    offset = (pos_x >> 1) + (pos_y & ~1) * decoder->stride;		      \
    table[xy_half] (decoder->dest[0] + decoder->offset,			      \
		    ref[0] + offset, 2 * decoder->stride, 8);		      \
    table[xy_half] (decoder->dest[0] + decoder->stride + decoder->offset,     \
		    ref[0] + decoder->stride + offset,			      \
		    2 * decoder->stride, 8);				      \
    offset = (offset + (motion_x & (motion_x < 0))) >> 1;		      \
    motion_x /= 2;							      \
    xy_half = ((pos_y & 1) << 1) | (motion_x & 1);			      \
    table[4+xy_half] (decoder->dest[1] + (decoder->offset >> 1),	      \
		      ref[1] + offset, 2 * decoder->uv_stride, 8);	      \
    table[4+xy_half] (decoder->dest[1] + decoder->uv_stride +		      \
		      (decoder->offset >> 1),				      \
		      ref[1] + decoder->uv_stride + offset,		      \
		      2 * decoder->uv_stride, 8);			      \
    table[4+xy_half] (decoder->dest[2] + (decoder->offset >> 1),	      \
		      ref[2] + offset, 2 * decoder->uv_stride, 8);	      \
    table[4+xy_half] (decoder->dest[2] + decoder->uv_stride +		      \
		      (decoder->offset >> 1),				      \
		      ref[2] + decoder->uv_stride + offset,		      \
		      2 * decoder->uv_stride, 8)

#define MOTION_ZERO_422(table,ref)					      \
    offset = decoder->offset + decoder->v_offset * decoder->stride;	      \
    table[0] (decoder->dest[0] + decoder->offset,			      \
	      ref[0] + offset, decoder->stride, 16);			      \
    offset >>= 1;							      \
    table[4] (decoder->dest[1] + (decoder->offset >> 1),		      \
	      ref[1] + offset, decoder->uv_stride, 16);			      \
    table[4] (decoder->dest[2] + (decoder->offset >> 1),		      \
	      ref[2] + offset, decoder->uv_stride, 16)

#define MOTION_444(table,ref,motion_x,motion_y,size,y)			      \
    pos_x = 2 * decoder->offset + motion_x;				      \
    pos_y = 2 * decoder->v_offset + motion_y + 2 * y;			      \
    if (unlikely (pos_x > decoder->limit_x)) {				      \
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		      \
	motion_x = pos_x - 2 * decoder->offset;				      \
    }									      \
    if (unlikely (pos_y > decoder->limit_y_ ## size)) {			      \
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y_ ## size;	      \
	motion_y = pos_y - 2 * decoder->v_offset - 2 * y;		      \
    }									      \
    xy_half = ((pos_y & 1) << 1) | (pos_x & 1);				      \
    offset = (pos_x >> 1) + (pos_y >> 1) * decoder->stride;		      \
    table[xy_half] (decoder->dest[0] + y * decoder->stride + decoder->offset, \
		    ref[0] + offset, decoder->stride, size);		      \
    table[xy_half] (decoder->dest[1] + y * decoder->stride + decoder->offset, \
		    ref[1] + offset, decoder->stride, size);		      \
    table[xy_half] (decoder->dest[2] + y * decoder->stride + decoder->offset, \
		    ref[2] + offset, decoder->stride, size)

#define MOTION_FIELD_444(table,ref,motion_x,motion_y,dest_field,op,src_field) \
    pos_x = 2 * decoder->offset + motion_x;				      \
    pos_y = decoder->v_offset + motion_y;				      \
    if (unlikely (pos_x > decoder->limit_x)) {				      \
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		      \
	motion_x = pos_x - 2 * decoder->offset;				      \
    }									      \
    if (unlikely (pos_y > decoder->limit_y)) {				      \
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y;		      \
	motion_y = pos_y - decoder->v_offset;				      \
    }									      \
    xy_half = ((pos_y & 1) << 1) | (pos_x & 1);				      \
    offset = (pos_x >> 1) + ((pos_y op) + src_field) * decoder->stride;	      \
    table[xy_half] (decoder->dest[0] + dest_field * decoder->stride +	      \
		    decoder->offset, ref[0] + offset,			      \
		    2 * decoder->stride, 8);				      \
    table[xy_half] (decoder->dest[1] + dest_field * decoder->stride +	      \
		    decoder->offset, ref[1] + offset,			      \
		    2 * decoder->stride, 8);				      \
    table[xy_half] (decoder->dest[2] + dest_field * decoder->stride +	      \
		    decoder->offset, ref[2] + offset,			      \
		    2 * decoder->stride, 8)


#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)

static void motion_mp1 (mpeg2_decoder_t * const decoder,
    motion_t * const motion, mpeg2_mc_fct * const * const table)
{
    int motion_x, motion_y;
    unsigned int pos_x, pos_y, xy_half, offset;

    NEEDBITS (bit_buf, bits, bit_ptr);

    motion_x = (motion->pmv[0][0] +
        (get_motion_delta (decoder, motion->f_code[0]) << motion->f_code[1]));

    motion_x = bound_motion_vector (motion_x,
				    motion->f_code[0] + motion->f_code[1]);
    motion->pmv[0][0] = motion_x;

    NEEDBITS (bit_buf, bits, bit_ptr);
    motion_y = (motion->pmv[0][1] +
		(get_motion_delta (decoder,
				   motion->f_code[0]) << motion->f_code[1]));
    motion_y = bound_motion_vector (motion_y,
				    motion->f_code[0] + motion->f_code[1]);
    motion->pmv[0][1] = motion_y;

    MOTION_420 (table, motion->ref[0], motion_x, motion_y, 16, 0);
}

#define MOTION_DMV_444(table,ref,motion_x,motion_y)			      \
    pos_x = 2 * decoder->offset + motion_x;				      \
    pos_y = decoder->v_offset + motion_y;				      \
    if (unlikely (pos_x > decoder->limit_x)) {				      \
	pos_x = ((int)pos_x < 0) ? 0 : decoder->limit_x;		      \
	motion_x = pos_x - 2 * decoder->offset;				      \
    }									      \
    if (unlikely (pos_y > decoder->limit_y)) {				      \
	pos_y = ((int)pos_y < 0) ? 0 : decoder->limit_y;		      \
	motion_y = pos_y - decoder->v_offset;				      \
    }									      \
    xy_half = ((pos_y & 1) << 1) | (pos_x & 1);				      \
    offset = (pos_x >> 1) + (pos_y & ~1) * decoder->stride;		      \
    table[xy_half] (decoder->dest[0] + decoder->offset,			      \
		    ref[0] + offset, 2 * decoder->stride, 8);		      \
    table[xy_half] (decoder->dest[0] + decoder->stride + decoder->offset,     \
		    ref[0] + decoder->stride + offset,			      \
		    2 * decoder->stride, 8);				      \
    table[xy_half] (decoder->dest[1] + decoder->offset,			      \
		    ref[1] + offset, 2 * decoder->stride, 8);		      \
    table[xy_half] (decoder->dest[1] + decoder->stride + decoder->offset,     \
		    ref[1] + decoder->stride + offset,			      \
		    2 * decoder->stride, 8);				      \
    table[xy_half] (decoder->dest[2] + decoder->offset,			      \
		    ref[2] + offset, 2 * decoder->stride, 8);		      \
    table[xy_half] (decoder->dest[2] + decoder->stride + decoder->offset,     \
		    ref[2] + decoder->stride + offset,			      \
		    2 * decoder->stride, 8)

#define MOTION_ZERO_444(table,ref)					      \
    offset = decoder->offset + decoder->v_offset * decoder->stride;	      \
    table[0] (decoder->dest[0] + decoder->offset,			      \
	      ref[0] + offset, decoder->stride, 16);			      \
    table[4] (decoder->dest[1] + decoder->offset,			      \
	      ref[1] + offset, decoder->stride, 16);			      \
    table[4] (decoder->dest[2] + decoder->offset,			      \
	      ref[2] + offset, decoder->stride, 16)


#define MOTION_FUNCTIONS(FORMAT,MOTION,MOTION_FIELD,MOTION_DMV,MOTION_ZERO)   \
									      \
static void motion_fr_frame_##FORMAT (mpeg2_decoder_t * const decoder,	      \
				      motion_t * const motion,		      \
				      mpeg2_mc_fct * const * const table)     \
{									      \
    int motion_x, motion_y;						      \
    unsigned int pos_x, pos_y, xy_half, offset;				      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    motion_x = motion->pmv[0][0] + get_motion_delta (decoder,		      \
						     motion->f_code[0]);      \
    motion_x = bound_motion_vector (motion_x, motion->f_code[0]);	      \
    motion->pmv[1][0] = motion->pmv[0][0] = motion_x;			      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    motion_y = motion->pmv[0][1] + get_motion_delta (decoder,		      \
						     motion->f_code[1]);      \
    motion_y = bound_motion_vector (motion_y, motion->f_code[1]);	      \
    motion->pmv[1][1] = motion->pmv[0][1] = motion_y;			      \
									      \
    MOTION (table, motion->ref[0], motion_x, motion_y, 16, 0);		      \
}									      \
									      \
static void motion_fr_field_##FORMAT (mpeg2_decoder_t * const decoder,	      \
				      motion_t * const motion,		      \
				      mpeg2_mc_fct * const * const table)     \
{									      \
    int motion_x, motion_y, field;					      \
    unsigned int pos_x, pos_y, xy_half, offset;				      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    field = UBITS (bit_buf, 1);						      \
    dumpbits (bit_buf, bits, 1);					      \
									      \
    motion_x = motion->pmv[0][0] + get_motion_delta (decoder,		      \
						     motion->f_code[0]);      \
    motion_x = bound_motion_vector (motion_x, motion->f_code[0]);	      \
    motion->pmv[0][0] = motion_x;					      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    motion_y = ((motion->pmv[0][1] >> 1) +				      \
		get_motion_delta (decoder, motion->f_code[1]));		      \
    /* motion_y = bound_motion_vector (motion_y, motion->f_code[1]); */	      \
    motion->pmv[0][1] = motion_y << 1;					      \
									      \
    MOTION_FIELD (table, motion->ref[0], motion_x, motion_y, 0, & ~1, field); \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    field = UBITS (bit_buf, 1);						      \
    dumpbits (bit_buf, bits, 1);					      \
									      \
    motion_x = motion->pmv[1][0] + get_motion_delta (decoder,		      \
						     motion->f_code[0]);      \
    motion_x = bound_motion_vector (motion_x, motion->f_code[0]);	      \
    motion->pmv[1][0] = motion_x;					      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    motion_y = ((motion->pmv[1][1] >> 1) +				      \
		get_motion_delta (decoder, motion->f_code[1]));		      \
    /* motion_y = bound_motion_vector (motion_y, motion->f_code[1]); */	      \
    motion->pmv[1][1] = motion_y << 1;					      \
									      \
    MOTION_FIELD (table, motion->ref[0], motion_x, motion_y, 1, & ~1, field); \
}									      \
									      \
static void motion_fr_dmv_##FORMAT (mpeg2_decoder_t * const decoder,	      \
				    motion_t * const motion,		      \
				    mpeg2_mc_fct * const * const table)	      \
{									      \
    int motion_x, motion_y, dmv_x, dmv_y, m, other_x, other_y;		      \
    unsigned int pos_x, pos_y, xy_half, offset;				      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    motion_x = motion->pmv[0][0] + get_motion_delta (decoder,		      \
						     motion->f_code[0]);      \
    motion_x = bound_motion_vector (motion_x, motion->f_code[0]);	      \
    motion->pmv[1][0] = motion->pmv[0][0] = motion_x;			      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    dmv_x = get_dmv (decoder);						      \
									      \
    motion_y = ((motion->pmv[0][1] >> 1) +				      \
		get_motion_delta (decoder, motion->f_code[1]));		      \
    /* motion_y = bound_motion_vector (motion_y, motion->f_code[1]); */	      \
    motion->pmv[1][1] = motion->pmv[0][1] = motion_y << 1;		      \
    dmv_y = get_dmv (decoder);						      \
									      \
    m = decoder->top_field_first ? 1 : 3;				      \
    other_x = ((motion_x * m + (motion_x > 0)) >> 1) + dmv_x;		      \
    other_y = ((motion_y * m + (motion_y > 0)) >> 1) + dmv_y - 1;	      \
    MOTION_FIELD (mpeg2_mc.put, motion->ref[0], other_x, other_y, 0, | 1, 0); \
									      \
    m = decoder->top_field_first ? 3 : 1;				      \
    other_x = ((motion_x * m + (motion_x > 0)) >> 1) + dmv_x;		      \
    other_y = ((motion_y * m + (motion_y > 0)) >> 1) + dmv_y + 1;	      \
    MOTION_FIELD (mpeg2_mc.put, motion->ref[0], other_x, other_y, 1, & ~1, 0);\
									      \
    MOTION_DMV (mpeg2_mc.avg, motion->ref[0], motion_x, motion_y);	      \
}									      \
									      \
static void motion_reuse_##FORMAT (mpeg2_decoder_t * const decoder,	      \
				   motion_t * const motion,		      \
				   mpeg2_mc_fct * const * const table)	      \
{									      \
    int motion_x, motion_y;						      \
    unsigned int pos_x, pos_y, xy_half, offset;				      \
									      \
    motion_x = motion->pmv[0][0];					      \
    motion_y = motion->pmv[0][1];					      \
									      \
    MOTION (table, motion->ref[0], motion_x, motion_y, 16, 0);		      \
}									      \
									      \
static void motion_zero_##FORMAT (mpeg2_decoder_t * const decoder,	      \
				  motion_t * const motion,		      \
				  mpeg2_mc_fct * const * const table)	      \
{									      \
    unsigned int offset;						      \
									      \
    motion->pmv[0][0] = motion->pmv[0][1] = 0;				      \
    motion->pmv[1][0] = motion->pmv[1][1] = 0;				      \
									      \
    MOTION_ZERO (table, motion->ref[0]);				      \
}									      \
									      \
static void motion_fi_field_##FORMAT (mpeg2_decoder_t * const decoder,	      \
				      motion_t * const motion,		      \
				      mpeg2_mc_fct * const * const table)     \
{									      \
    int motion_x, motion_y;						      \
    uint8_t ** ref_field;						      \
    unsigned int pos_x, pos_y, xy_half, offset;				      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    ref_field = motion->ref2[UBITS (bit_buf, 1)];			      \
    dumpbits (bit_buf, bits, 1);					      \
									      \
    motion_x = motion->pmv[0][0] + get_motion_delta (decoder,		      \
						     motion->f_code[0]);      \
    motion_x = bound_motion_vector (motion_x, motion->f_code[0]);	      \
    motion->pmv[1][0] = motion->pmv[0][0] = motion_x;			      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    motion_y = motion->pmv[0][1] + get_motion_delta (decoder,		      \
						     motion->f_code[1]);      \
    motion_y = bound_motion_vector (motion_y, motion->f_code[1]);	      \
    motion->pmv[1][1] = motion->pmv[0][1] = motion_y;			      \
									      \
    MOTION (table, ref_field, motion_x, motion_y, 16, 0);		      \
}									      \
									      \
static void motion_fi_16x8_##FORMAT (mpeg2_decoder_t * const decoder,	      \
				     motion_t * const motion,		      \
				     mpeg2_mc_fct * const * const table)      \
{									      \
    int motion_x, motion_y;						      \
    uint8_t ** ref_field;						      \
    unsigned int pos_x, pos_y, xy_half, offset;				      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    ref_field = motion->ref2[UBITS (bit_buf, 1)];			      \
    dumpbits (bit_buf, bits, 1);					      \
									      \
    motion_x = motion->pmv[0][0] + get_motion_delta (decoder,		      \
						     motion->f_code[0]);      \
    motion_x = bound_motion_vector (motion_x, motion->f_code[0]);	      \
    motion->pmv[0][0] = motion_x;					      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    motion_y = motion->pmv[0][1] + get_motion_delta (decoder,		      \
						     motion->f_code[1]);      \
    motion_y = bound_motion_vector (motion_y, motion->f_code[1]);	      \
    motion->pmv[0][1] = motion_y;					      \
									      \
    MOTION (table, ref_field, motion_x, motion_y, 8, 0);		      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    ref_field = motion->ref2[UBITS (bit_buf, 1)];			      \
    dumpbits (bit_buf, bits, 1);					      \
									      \
    motion_x = motion->pmv[1][0] + get_motion_delta (decoder,		      \
						     motion->f_code[0]);      \
    motion_x = bound_motion_vector (motion_x, motion->f_code[0]);	      \
    motion->pmv[1][0] = motion_x;					      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    motion_y = motion->pmv[1][1] + get_motion_delta (decoder,		      \
						     motion->f_code[1]);      \
    motion_y = bound_motion_vector (motion_y, motion->f_code[1]);	      \
    motion->pmv[1][1] = motion_y;					      \
									      \
    MOTION (table, ref_field, motion_x, motion_y, 8, 8);		      \
}									      \
									      \
static void motion_fi_dmv_##FORMAT (mpeg2_decoder_t * const decoder,	      \
				    motion_t * const motion,		      \
				    mpeg2_mc_fct * const * const table)	      \
{									      \
    int motion_x, motion_y, other_x, other_y;				      \
    unsigned int pos_x, pos_y, xy_half, offset;				      \
									      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    motion_x = motion->pmv[0][0] + get_motion_delta (decoder,		      \
						     motion->f_code[0]);      \
    motion_x = bound_motion_vector (motion_x, motion->f_code[0]);	      \
    motion->pmv[1][0] = motion->pmv[0][0] = motion_x;			      \
    NEEDBITS (bit_buf, bits, bit_ptr);					      \
    other_x = ((motion_x + (motion_x > 0)) >> 1) + get_dmv (decoder);	      \
									      \
    motion_y = motion->pmv[0][1] + get_motion_delta (decoder,		      \
						     motion->f_code[1]);      \
    motion_y = bound_motion_vector (motion_y, motion->f_code[1]);	      \
    motion->pmv[1][1] = motion->pmv[0][1] = motion_y;			      \
    other_y = (((motion_y + (motion_y > 0)) >> 1) + get_dmv (decoder) +	      \
	       decoder->dmv_offset);					      \
									      \
    MOTION (mpeg2_mc.put, motion->ref[0], motion_x, motion_y, 16, 0);	      \
    MOTION (mpeg2_mc.avg, motion->ref[1], other_x, other_y, 16, 0);	      \
}									      \

MOTION_FUNCTIONS (420, MOTION_420, MOTION_FIELD_420, MOTION_DMV_420, MOTION_ZERO_420)
MOTION_FUNCTIONS (422, MOTION_422, MOTION_FIELD_422, MOTION_DMV_422, MOTION_ZERO_422)
MOTION_FUNCTIONS (444, MOTION_444, MOTION_FIELD_444, MOTION_DMV_444, MOTION_ZERO_444)

/* like motion_frame, but parsing without actual motion compensation */
static void motion_fr_conceal (mpeg2_decoder_t * const decoder)
{
    int tmp;

    NEEDBITS (bit_buf, bits, bit_ptr);
    tmp = (decoder->f_motion.pmv[0][0] +
	   get_motion_delta (decoder, decoder->f_motion.f_code[0]));
    tmp = bound_motion_vector (tmp, decoder->f_motion.f_code[0]);
    decoder->f_motion.pmv[1][0] = decoder->f_motion.pmv[0][0] = tmp;

    NEEDBITS (bit_buf, bits, bit_ptr);
    tmp = (decoder->f_motion.pmv[0][1] +
	   get_motion_delta (decoder, decoder->f_motion.f_code[1]));
    tmp = bound_motion_vector (tmp, decoder->f_motion.f_code[1]);
    decoder->f_motion.pmv[1][1] = decoder->f_motion.pmv[0][1] = tmp;

    dumpbits (bit_buf, bits, 1); /* remove marker_bit */
}

static void motion_fi_conceal (mpeg2_decoder_t * const decoder)
{
    int tmp;

    NEEDBITS (bit_buf, bits, bit_ptr);
    dumpbits (bit_buf, bits, 1); /* remove field_select */

    tmp = (decoder->f_motion.pmv[0][0] +
	   get_motion_delta (decoder, decoder->f_motion.f_code[0]));
    tmp = bound_motion_vector (tmp, decoder->f_motion.f_code[0]);
    decoder->f_motion.pmv[1][0] = decoder->f_motion.pmv[0][0] = tmp;

    NEEDBITS (bit_buf, bits, bit_ptr);
    tmp = (decoder->f_motion.pmv[0][1] +
	   get_motion_delta (decoder, decoder->f_motion.f_code[1]));
    tmp = bound_motion_vector (tmp, decoder->f_motion.f_code[1]);
    decoder->f_motion.pmv[1][1] = decoder->f_motion.pmv[0][1] = tmp;

    dumpbits (bit_buf, bits, 1); /* remove marker_bit */
}

#undef bit_buf
#undef bits
#undef bit_ptr

#define MOTION_CALL(routine,direction)				\
do {								\
    if ((direction) & MACROBLOCK_MOTION_FORWARD)		\
	routine (decoder, &(decoder->f_motion), mpeg2_mc.put);	\
    if ((direction) & MACROBLOCK_MOTION_BACKWARD)		\
	routine (decoder, &(decoder->b_motion),			\
		 ((direction) & MACROBLOCK_MOTION_FORWARD ?	\
		  mpeg2_mc.avg : mpeg2_mc.put));		\
} while (0)

#define NEXT_MACROBLOCK							\
do {									\
    decoder->offset += 16;						\
    if (decoder->offset == decoder->width) {				\
	do { /* just so we can use the break statement */		\
	    if (decoder->convert) {					\
		decoder->convert (decoder->convert_id, decoder->dest,	\
				  decoder->v_offset);			\
		if (decoder->coding_type == B_TYPE)			\
		    break;						\
	    }								\
	    decoder->dest[0] += decoder->slice_stride;			\
	    decoder->dest[1] += decoder->slice_uv_stride;		\
	    decoder->dest[2] += decoder->slice_uv_stride;		\
	} while (0);							\
	decoder->v_offset += 16;					\
	if (decoder->v_offset > decoder->limit_y) {			\
	    return;							\
	}								\
	decoder->offset = 0;						\
    }									\
} while (0)

/**
 * Dummy motion decoding function, to avoid calling NULL in
 * case of malformed streams.
 */
static void motion_dummy (mpeg2_decoder_t * const decoder,
                          motion_t * const motion,
                          mpeg2_mc_fct * const * const table)
{
}

void mpeg2_init_fbuf (mpeg2_decoder_t * decoder, uint8_t * current_fbuf[3],
		      uint8_t * forward_fbuf[3], uint8_t * backward_fbuf[3])
{
    int offset, stride, height, bottom_field;

    stride = decoder->stride_frame;
    bottom_field = (decoder->picture_structure == BOTTOM_FIELD);
    offset = bottom_field ? stride : 0;
    height = decoder->height;

    decoder->picture_dest[0] = current_fbuf[0] + offset;
    decoder->picture_dest[1] = current_fbuf[1] + (offset >> 1);
    decoder->picture_dest[2] = current_fbuf[2] + (offset >> 1);

    decoder->f_motion.ref[0][0] = forward_fbuf[0] + offset;
    decoder->f_motion.ref[0][1] = forward_fbuf[1] + (offset >> 1);
    decoder->f_motion.ref[0][2] = forward_fbuf[2] + (offset >> 1);

    decoder->b_motion.ref[0][0] = backward_fbuf[0] + offset;
    decoder->b_motion.ref[0][1] = backward_fbuf[1] + (offset >> 1);
    decoder->b_motion.ref[0][2] = backward_fbuf[2] + (offset >> 1);

    if (decoder->picture_structure != FRAME_PICTURE)
    {
        decoder->dmv_offset = bottom_field ? 1 : -1;
        decoder->f_motion.ref2[0] = decoder->f_motion.ref[bottom_field];
        decoder->f_motion.ref2[1] = decoder->f_motion.ref[!bottom_field];
        decoder->b_motion.ref2[0] = decoder->b_motion.ref[bottom_field];
        decoder->b_motion.ref2[1] = decoder->b_motion.ref[!bottom_field];
        offset = stride - offset;

        if (decoder->second_field && (decoder->coding_type != B_TYPE))
            forward_fbuf = current_fbuf;

        decoder->f_motion.ref[1][0] = forward_fbuf[0] + offset;
        decoder->f_motion.ref[1][1] = forward_fbuf[1] + (offset >> 1);
        decoder->f_motion.ref[1][2] = forward_fbuf[2] + (offset >> 1);

        decoder->b_motion.ref[1][0] = backward_fbuf[0] + offset;
        decoder->b_motion.ref[1][1] = backward_fbuf[1] + (offset >> 1);
        decoder->b_motion.ref[1][2] = backward_fbuf[2] + (offset >> 1);

        stride <<= 1;
        height >>= 1;
    }

    decoder->stride = stride;
    decoder->uv_stride = stride >> 1;
    decoder->slice_stride = 16 * stride;
    decoder->slice_uv_stride =
	decoder->slice_stride >> (2 - decoder->chroma_format);
    decoder->limit_x = 2 * decoder->width - 32;
    decoder->limit_y_16 = 2 * height - 32;
    decoder->limit_y_8 = 2 * height - 16;
    decoder->limit_y = height - 16;

    if (decoder->mpeg1)
    {
        decoder->motion_parser[0] = motion_zero_420;
        decoder->motion_parser[MC_FIELD] = motion_dummy;
        decoder->motion_parser[MC_FRAME] = motion_mp1;
        decoder->motion_parser[MC_DMV] = motion_dummy;
        decoder->motion_parser[4] = motion_reuse_420;
    }
    else if (decoder->picture_structure == FRAME_PICTURE)
    {
	if (decoder->chroma_format == 0) {
	    decoder->motion_parser[0] = motion_zero_420;
	    decoder->motion_parser[MC_FIELD] = motion_fr_field_420;
	    decoder->motion_parser[MC_FRAME] = motion_fr_frame_420;
	    decoder->motion_parser[MC_DMV] = motion_fr_dmv_420;
	    decoder->motion_parser[4] = motion_reuse_420;
	} else if (decoder->chroma_format == 1) {
	    decoder->motion_parser[0] = motion_zero_422;
	    decoder->motion_parser[MC_FIELD] = motion_fr_field_422;
	    decoder->motion_parser[MC_FRAME] = motion_fr_frame_422;
	    decoder->motion_parser[MC_DMV] = motion_fr_dmv_422;
	    decoder->motion_parser[4] = motion_reuse_422;
	} else {
	    decoder->motion_parser[0] = motion_zero_444;
	    decoder->motion_parser[MC_FIELD] = motion_fr_field_444;
	    decoder->motion_parser[MC_FRAME] = motion_fr_frame_444;
	    decoder->motion_parser[MC_DMV] = motion_fr_dmv_444;
	    decoder->motion_parser[4] = motion_reuse_444;
	}
    } else {
        if (decoder->chroma_format == 0) {
            decoder->motion_parser[0] = motion_zero_420;
            decoder->motion_parser[MC_FIELD] = motion_fi_field_420;
            decoder->motion_parser[MC_16X8] = motion_fi_16x8_420;
            decoder->motion_parser[MC_DMV] = motion_fi_dmv_420;
            decoder->motion_parser[4] = motion_reuse_420;
        } else if (decoder->chroma_format == 1) {
            decoder->motion_parser[0] = motion_zero_422;
            decoder->motion_parser[MC_FIELD] = motion_fi_field_422;
            decoder->motion_parser[MC_16X8] = motion_fi_16x8_422;
            decoder->motion_parser[MC_DMV] = motion_fi_dmv_422;
            decoder->motion_parser[4] = motion_reuse_422;
        } else {
            decoder->motion_parser[0] = motion_zero_444;
            decoder->motion_parser[MC_FIELD] = motion_fi_field_444;
            decoder->motion_parser[MC_16X8] = motion_fi_16x8_444;
            decoder->motion_parser[MC_DMV] = motion_fi_dmv_444;
            decoder->motion_parser[4] = motion_reuse_444;
        }
    }
}

static inline int slice_init (mpeg2_decoder_t * const decoder, int code)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)
    int offset;
    const MBAtab * mba;

    decoder->dc_dct_pred[0] = decoder->dc_dct_pred[1] =
	decoder->dc_dct_pred[2] = 16384;

    decoder->f_motion.pmv[0][0] = decoder->f_motion.pmv[0][1] = 0;
    decoder->f_motion.pmv[1][0] = decoder->f_motion.pmv[1][1] = 0;
    decoder->b_motion.pmv[0][0] = decoder->b_motion.pmv[0][1] = 0;
    decoder->b_motion.pmv[1][0] = decoder->b_motion.pmv[1][1] = 0;

    if (decoder->vertical_position_extension) {
	code += UBITS (bit_buf, 3) << 7;
	dumpbits (bit_buf, bits, 3);
    }
    decoder->v_offset = (code - 1) * 16;
    offset = 0;
    if (!(decoder->convert) || decoder->coding_type != B_TYPE)
	offset = (code - 1) * decoder->slice_stride;

    decoder->dest[0] = decoder->picture_dest[0] + offset;
    offset >>= (2 - decoder->chroma_format);
    decoder->dest[1] = decoder->picture_dest[1] + offset;
    decoder->dest[2] = decoder->picture_dest[2] + offset;

    get_quantizer_scale (decoder);

    /* ignore intra_slice and all the extra data */
    while (bit_buf & 0x80000000) {
	dumpbits (bit_buf, bits, 9);
	NEEDBITS (bit_buf, bits, bit_ptr);
    }

    /* decode initial macroblock address increment */
    offset = 0;
    while (1) {
	if (bit_buf >= 0x08000000) {
	    mba = MBA_5 + (UBITS (bit_buf, 6) - 2);
	    break;
	} else if (bit_buf >= 0x01800000) {
	    mba = MBA_11 + (UBITS (bit_buf, 12) - 24);
	    break;
	} else switch (UBITS (bit_buf, 12)) {
	case 8:		/* macroblock_escape */
	    offset += 33;
	    dumpbits (bit_buf, bits, 11);
	    NEEDBITS (bit_buf, bits, bit_ptr);
	    continue;
	case 15:	/* macroblock_stuffing (MPEG1 only) */
	    bit_buf &= 0xfffff;
	    dumpbits (bit_buf, bits, 11);
	    NEEDBITS (bit_buf, bits, bit_ptr);
	    continue;
	default:	/* error */
	    return 1;
	}
    }
    dumpbits (bit_buf, bits, mba->len + 1);
    decoder->offset = (offset + mba->mba) << 4;

    while (decoder->offset - decoder->width >= 0) {
	decoder->offset -= decoder->width;
	if (!(decoder->convert) || decoder->coding_type != B_TYPE) {
	    decoder->dest[0] += decoder->slice_stride;
	    decoder->dest[1] += decoder->slice_uv_stride;
	    decoder->dest[2] += decoder->slice_uv_stride;
	}
	decoder->v_offset += 16;
    }
    if (decoder->v_offset > decoder->limit_y)
	return 1;

    return 0;
#undef bit_buf
#undef bits
#undef bit_ptr
}

void mpeg2_slice (mpeg2_decoder_t * const decoder, const int code,
		  const uint8_t * const buffer)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)
    cpu_state_t cpu_state;
    bitstream_init(decoder, buffer);

    if (slice_init(decoder, code))
        return;

    while (1) {
	int macroblock_modes;
	int mba_inc;
	const MBAtab * mba;

	NEEDBITS (bit_buf, bits, bit_ptr);

	macroblock_modes = get_macroblock_modes (decoder);

	/* maybe integrate MACROBLOCK_QUANT test into get_macroblock_modes ? */
	if (macroblock_modes & MACROBLOCK_QUANT)
	    get_quantizer_scale (decoder);

	if (macroblock_modes & MACROBLOCK_INTRA) {

	    int DCT_offset, DCT_stride;
	    int offset;
	    uint8_t * dest_y;

	    if (decoder->concealment_motion_vectors) {
		if (decoder->picture_structure == FRAME_PICTURE)
		    motion_fr_conceal (decoder);
		else
		    motion_fi_conceal (decoder);
	    } else {
		decoder->f_motion.pmv[0][0] = decoder->f_motion.pmv[0][1] = 0;
		decoder->f_motion.pmv[1][0] = decoder->f_motion.pmv[1][1] = 0;
		decoder->b_motion.pmv[0][0] = decoder->b_motion.pmv[0][1] = 0;
		decoder->b_motion.pmv[1][0] = decoder->b_motion.pmv[1][1] = 0;
	    }

	    if (macroblock_modes & DCT_TYPE_INTERLACED) {
		DCT_offset = decoder->stride;
		DCT_stride = decoder->stride * 2;
	    } else {
		DCT_offset = decoder->stride * 8;
		DCT_stride = decoder->stride;
	    }

	    offset = decoder->offset;
	    dest_y = decoder->dest[0] + offset;
	    slice_intra_DCT (decoder, 0, dest_y, DCT_stride);
	    slice_intra_DCT (decoder, 0, dest_y + 8, DCT_stride);
	    slice_intra_DCT (decoder, 0, dest_y + DCT_offset, DCT_stride);
	    slice_intra_DCT (decoder, 0, dest_y + DCT_offset + 8, DCT_stride);
	    if (likely (decoder->chroma_format == 0)) {
		slice_intra_DCT (decoder, 1, decoder->dest[1] + (offset >> 1),
				 decoder->uv_stride);
		slice_intra_DCT (decoder, 2, decoder->dest[2] + (offset >> 1),
				 decoder->uv_stride);
		if (decoder->coding_type == D_TYPE) {
		    NEEDBITS (bit_buf, bits, bit_ptr);
		    dumpbits (bit_buf, bits, 1);
		}
	    } else if (likely (decoder->chroma_format == 1)) {
		uint8_t * dest_u = decoder->dest[1] + (offset >> 1);
		uint8_t * dest_v = decoder->dest[2] + (offset >> 1);
		DCT_stride >>= 1;
		DCT_offset >>= 1;
		slice_intra_DCT (decoder, 1, dest_u, DCT_stride);
		slice_intra_DCT (decoder, 2, dest_v, DCT_stride);
		slice_intra_DCT (decoder, 1, dest_u + DCT_offset, DCT_stride);
		slice_intra_DCT (decoder, 2, dest_v + DCT_offset, DCT_stride);
	    } else {
		uint8_t * dest_u = decoder->dest[1] + offset;
		uint8_t * dest_v = decoder->dest[2] + offset;
		slice_intra_DCT (decoder, 1, dest_u, DCT_stride);
		slice_intra_DCT (decoder, 2, dest_v, DCT_stride);
		slice_intra_DCT (decoder, 1, dest_u + DCT_offset, DCT_stride);
		slice_intra_DCT (decoder, 2, dest_v + DCT_offset, DCT_stride);
		slice_intra_DCT (decoder, 1, dest_u + 8, DCT_stride);
		slice_intra_DCT (decoder, 2, dest_v + 8, DCT_stride);
		slice_intra_DCT (decoder, 1, dest_u + DCT_offset + 8,
				 DCT_stride);
		slice_intra_DCT (decoder, 2, dest_v + DCT_offset + 8,
				 DCT_stride);
	    }
	} else {

	    motion_parser_t * parser;

	    if (   ((macroblock_modes >> MOTION_TYPE_SHIFT) < 0)
                || ((macroblock_modes >> MOTION_TYPE_SHIFT) >=
                    (int)(sizeof(decoder->motion_parser) 
                          / sizeof(decoder->motion_parser[0])))
	       ) {
		break; // Illegal !
	    }

	    parser =
		decoder->motion_parser[macroblock_modes >> MOTION_TYPE_SHIFT];
	    MOTION_CALL (parser, macroblock_modes);

	    if (macroblock_modes & MACROBLOCK_PATTERN) {
		int coded_block_pattern;
		int DCT_offset, DCT_stride;

		if (macroblock_modes & DCT_TYPE_INTERLACED) {
		    DCT_offset = decoder->stride;
		    DCT_stride = decoder->stride * 2;
		} else {
		    DCT_offset = decoder->stride * 8;
		    DCT_stride = decoder->stride;
		}

		coded_block_pattern = get_coded_block_pattern (decoder);

        if (likely (decoder->chroma_format == 0))
        {
		    int offset = decoder->offset;
		    uint8_t * dest_y = decoder->dest[0] + offset;

		    if (coded_block_pattern & 1)
                slice_non_intra_DCT (decoder, 0, dest_y, DCT_stride);

		    if (coded_block_pattern & 2)
			slice_non_intra_DCT (decoder, 0, dest_y + 8,
					     DCT_stride);
		    if (coded_block_pattern & 4)
			slice_non_intra_DCT (decoder, 0, dest_y + DCT_offset,
					     DCT_stride);
		    if (coded_block_pattern & 8)
			slice_non_intra_DCT (decoder, 0,
					     dest_y + DCT_offset + 8,
					     DCT_stride);
		    if (coded_block_pattern & 16)
			slice_non_intra_DCT (decoder, 1,
					     decoder->dest[1] + (offset >> 1),
					     decoder->uv_stride);
		    if (coded_block_pattern & 32)
			slice_non_intra_DCT (decoder, 2,
					     decoder->dest[2] + (offset >> 1),
					     decoder->uv_stride);
		} else if (likely (decoder->chroma_format == 1)) {
		    int offset;
		    uint8_t * dest_y;

		    coded_block_pattern |= bit_buf & (3 << 30);
		    dumpbits (bit_buf, bits, 2);

		    offset = decoder->offset;
		    dest_y = decoder->dest[0] + offset;
		    if (coded_block_pattern & 1)
			slice_non_intra_DCT (decoder, 0, dest_y, DCT_stride);
		    if (coded_block_pattern & 2)
			slice_non_intra_DCT (decoder, 0, dest_y + 8,
					     DCT_stride);
		    if (coded_block_pattern & 4)
			slice_non_intra_DCT (decoder, 0, dest_y + DCT_offset,
					     DCT_stride);
		    if (coded_block_pattern & 8)
			slice_non_intra_DCT (decoder, 0,
					     dest_y + DCT_offset + 8,
					     DCT_stride);

		    DCT_stride >>= 1;
		    DCT_offset = (DCT_offset + offset) >> 1;
		    if (coded_block_pattern & 16)
			slice_non_intra_DCT (decoder, 1,
					     decoder->dest[1] + (offset >> 1),
					     DCT_stride);
		    if (coded_block_pattern & 32)
			slice_non_intra_DCT (decoder, 2,
					     decoder->dest[2] + (offset >> 1),
					     DCT_stride);
		    if (coded_block_pattern & (2 << 30))
			slice_non_intra_DCT (decoder, 1,
					     decoder->dest[1] + DCT_offset,
					     DCT_stride);
		    if (coded_block_pattern & (1 << 30))
			slice_non_intra_DCT (decoder, 2,
					     decoder->dest[2] + DCT_offset,
					     DCT_stride);
		} else {
		    int offset;
		    uint8_t * dest_y, * dest_u, * dest_v;

		    coded_block_pattern |= bit_buf & (63 << 26);
		    dumpbits (bit_buf, bits, 6);

		    offset = decoder->offset;
		    dest_y = decoder->dest[0] + offset;
		    dest_u = decoder->dest[1] + offset;
		    dest_v = decoder->dest[2] + offset;

		    if (coded_block_pattern & 1)
			slice_non_intra_DCT (decoder, 0, dest_y, DCT_stride);
		    if (coded_block_pattern & 2)
			slice_non_intra_DCT (decoder, 0, dest_y + 8,
					     DCT_stride);
		    if (coded_block_pattern & 4)
                slice_non_intra_DCT (decoder, 0, dest_y + DCT_offset, DCT_stride);
		    if (coded_block_pattern & 8)
                slice_non_intra_DCT (decoder, 0, dest_y + DCT_offset + 8, DCT_stride);

		    if (coded_block_pattern & 16)
			slice_non_intra_DCT (decoder, 1, dest_u, DCT_stride);
		    if (coded_block_pattern & 32)
			slice_non_intra_DCT (decoder, 2, dest_v, DCT_stride);
		    if (coded_block_pattern & (32 << 26))
			slice_non_intra_DCT (decoder, 1, dest_u + DCT_offset,
					     DCT_stride);
		    if (coded_block_pattern & (16 << 26))
			slice_non_intra_DCT (decoder, 2, dest_v + DCT_offset,
					     DCT_stride);
		    if (coded_block_pattern & (8 << 26))
			slice_non_intra_DCT (decoder, 1, dest_u + 8,
					     DCT_stride);
		    if (coded_block_pattern & (4 << 26))
			slice_non_intra_DCT (decoder, 2, dest_v + 8,
					     DCT_stride);
		    if (coded_block_pattern & (2 << 26))
                slice_non_intra_DCT (decoder, 1, dest_u + DCT_offset + 8, DCT_stride);
		    if (coded_block_pattern & (1 << 26))
			slice_non_intra_DCT (decoder, 2,
					     dest_v + DCT_offset + 8,
					     DCT_stride);
		}
	    }

	    decoder->dc_dct_pred[0] = decoder->dc_dct_pred[1] =
		decoder->dc_dct_pred[2] = 16384;
	}

	NEXT_MACROBLOCK;

	NEEDBITS (bit_buf, bits, bit_ptr);
	mba_inc = 0;
	while (1) {
	    if (bit_buf >= 0x10000000) {
		mba = MBA_5 + (UBITS (bit_buf, 5) - 2);
		break;
	    } else if (bit_buf >= 0x03000000) {
		mba = MBA_11 + (UBITS (bit_buf, 11) - 24);
		break;
	    } else switch (UBITS (bit_buf, 11)) {
	    case 8:		/* macroblock_escape */
		mba_inc += 33;
		/* pass through */
	    case 15:	/* macroblock_stuffing (MPEG1 only) */
            dumpbits (bit_buf, bits, 11);
            NEEDBITS (bit_buf, bits, bit_ptr);
            continue;
	    default:	/* end of slice, or error */
            return;
	    }
	}
	dumpbits (bit_buf, bits, mba->len);
	mba_inc += mba->mba;

	if (mba_inc) {
	    decoder->dc_dct_pred[0] = decoder->dc_dct_pred[1] =
		decoder->dc_dct_pred[2] = 16384;

	    if (decoder->coding_type == P_TYPE) {
		do {
		    MOTION_CALL (decoder->motion_parser[0],
				 MACROBLOCK_MOTION_FORWARD);
		    NEXT_MACROBLOCK;
		} while (--mba_inc);
	    } else {
		do {
		    MOTION_CALL (decoder->motion_parser[4], macroblock_modes);
		    NEXT_MACROBLOCK;
		} while (--mba_inc);
	    }
	}
    }
#undef bit_buf
#undef bits
#undef bit_ptr
}



static int mpeg2_accels = 0;
    
#define BUFFER_SIZE (1194 * 1024)
        
const mpeg2_info_t * mpeg2_info (mpeg2dec_t * mpeg2dec)
{   
    return &(mpeg2dec->info);
}

static inline int skip_chunk (mpeg2dec_t * mpeg2dec, int bytes)
{   
    uint8_t * current;
    uint32_t shift;
    uint8_t * limit;
    uint8_t byte;
    
    if (!bytes)
    return 0;

    current = mpeg2dec->buf_start;
    shift = mpeg2dec->shift;
    limit = current + bytes;

    do {
    byte = *current++;
    if (shift == 0x00000100) {
        int skipped;

        mpeg2dec->shift = 0xffffff00;
        skipped = current - mpeg2dec->buf_start;
        mpeg2dec->buf_start = current;
        return skipped;
    }
    shift = (shift | byte) << 8;
    } while (current < limit);

    mpeg2dec->shift = shift;
    mpeg2dec->buf_start = current;
    return 0;
}

static inline int copy_chunk (mpeg2dec_t * mpeg2dec, int bytes)
{
    uint8_t * current;
    uint32_t shift;
    uint8_t * chunk_ptr;
    uint8_t * limit;
    uint8_t byte;

    if (!bytes)
        return 0;

    current = mpeg2dec->buf_start;
    shift = mpeg2dec->shift;
    chunk_ptr = mpeg2dec->chunk_ptr;
    limit = current + bytes;

    do {
    byte = *current++;
    if (shift == 0x00000100) {
        int copied;

        mpeg2dec->shift = 0xffffff00;
        mpeg2dec->chunk_ptr = chunk_ptr + 1;
        copied = current - mpeg2dec->buf_start;
        mpeg2dec->buf_start = current;
        return copied;
    }
    shift = (shift | byte) << 8;
    *chunk_ptr++ = byte;
    } while (current < limit);

    mpeg2dec->shift = shift;
    mpeg2dec->buf_start = current;
    return 0;
}

void mpeg2_buffer (mpeg2dec_t * mpeg2dec, uint8_t * start, uint8_t * end)
{
    mpeg2dec->buf_start = start;
    mpeg2dec->buf_end = end;
}

int mpeg2_getpos (mpeg2dec_t * mpeg2dec)
{
    return mpeg2dec->buf_end - mpeg2dec->buf_start;
}

static inline mpeg2_state_t seek_chunk (mpeg2dec_t * mpeg2dec)
{
    int size, skipped;

    size = mpeg2dec->buf_end - mpeg2dec->buf_start;
    skipped = skip_chunk (mpeg2dec, size);
    if (!skipped) {
    mpeg2dec->bytes_since_tag += size;
    return STATE_BUFFER;
    }
    mpeg2dec->bytes_since_tag += skipped;
    mpeg2dec->code = mpeg2dec->buf_start[-1];
    return STATE_INTERNAL_NORETURN;
}

mpeg2_state_t mpeg2_seek_header (mpeg2dec_t * mpeg2dec)
{
    while (!(mpeg2dec->code == 0xb3 ||
         ((mpeg2dec->code == 0xb7 || mpeg2dec->code == 0xb8 ||
           !mpeg2dec->code) && mpeg2dec->sequence.width != (unsigned)-1)))
    if (seek_chunk (mpeg2dec) == STATE_BUFFER)
        return STATE_BUFFER;
    mpeg2dec->chunk_start = mpeg2dec->chunk_ptr = mpeg2dec->chunk_buffer;
    mpeg2dec->user_data_len = 0;
    return ((mpeg2dec->code == 0xb7) ?
        mpeg2_header_end (mpeg2dec) : mpeg2_parse_header (mpeg2dec));
}

#define RECEIVED(code,state) (((state) << 8) + (code))

mpeg2_state_t mpeg2_parse (mpeg2dec_t * mpeg2dec)
{
    int size_buffer, size_chunk, copied;

    if (mpeg2dec->action) {
    mpeg2_state_t state;

    state = mpeg2dec->action (mpeg2dec);
    if ((int)state > (int)STATE_INTERNAL_NORETURN)
        return state;
    }

    while (1)
    {
        while ((unsigned) (mpeg2dec->code - mpeg2dec->first_decode_slice) <
           mpeg2dec->nb_decode_slices)
        {
            size_buffer = mpeg2dec->buf_end - mpeg2dec->buf_start;
            size_chunk = (mpeg2dec->chunk_buffer + BUFFER_SIZE - mpeg2dec->chunk_ptr);
            if (size_buffer <= size_chunk)
            {
                copied = copy_chunk (mpeg2dec, size_buffer);
                if (!copied)
                {
                    mpeg2dec->bytes_since_tag += size_buffer;
                    mpeg2dec->chunk_ptr += size_buffer;
                    return STATE_BUFFER;
                }
            } else
            {
                copied = copy_chunk (mpeg2dec, size_chunk);
                if (!copied)
                {
                    /* filled the chunk buffer without finding a start code */
                    mpeg2dec->bytes_since_tag += size_chunk;
                    mpeg2dec->action = seek_chunk;
                    return STATE_INVALID;
                }
            }
            mpeg2dec->bytes_since_tag += copied;

            mpeg2_slice (&(mpeg2dec->decoder), mpeg2dec->code, mpeg2dec->chunk_start);
            mpeg2dec->code = mpeg2dec->buf_start[-1];
            mpeg2dec->chunk_ptr = mpeg2dec->chunk_start;
        }
        if ((unsigned) (mpeg2dec->code - 1) >= 0xb0 - 1)
            break;
        if (seek_chunk (mpeg2dec) == STATE_BUFFER)
            return STATE_BUFFER;
    }

    mpeg2dec->action = mpeg2_seek_header;
    switch (mpeg2dec->code) {
    case 0x00:
    return mpeg2dec->state;
    case 0xb3:
    case 0xb7:
    case 0xb8:
    return (mpeg2dec->state == STATE_SLICE) ? STATE_SLICE : STATE_INVALID;
    default:
    mpeg2dec->action = seek_chunk;
    return STATE_INVALID;
    }
}

mpeg2_state_t mpeg2_parse_header (mpeg2dec_t * mpeg2dec)
{
    static int (* process_header[]) (mpeg2dec_t * mpeg2dec) = {
    mpeg2_header_picture, mpeg2_header_extension, mpeg2_header_user_data,
    mpeg2_header_sequence, NULL, NULL, NULL, NULL, mpeg2_header_gop
    };
    int size_buffer, size_chunk, copied;

    mpeg2dec->action = mpeg2_parse_header;
    mpeg2dec->info.user_data = NULL;    mpeg2dec->info.user_data_len = 0;
    while (1) {
    size_buffer = mpeg2dec->buf_end - mpeg2dec->buf_start;
    size_chunk = (mpeg2dec->chunk_buffer + BUFFER_SIZE -
              mpeg2dec->chunk_ptr);
    if (size_buffer <= size_chunk) {
        copied = copy_chunk (mpeg2dec, size_buffer);
        if (!copied) {
        mpeg2dec->bytes_since_tag += size_buffer;
        mpeg2dec->chunk_ptr += size_buffer;
        return STATE_BUFFER;
        }
    } else {
        copied = copy_chunk (mpeg2dec, size_chunk);
        if (!copied) {
        /* filled the chunk buffer without finding a start code */
        mpeg2dec->bytes_since_tag += size_chunk;
        mpeg2dec->code = 0xb4;
        mpeg2dec->action = mpeg2_seek_header;
        return STATE_INVALID;
        }
    }
    mpeg2dec->bytes_since_tag += copied;

    if (process_header[mpeg2dec->code & 0x0b] (mpeg2dec)) {
        mpeg2dec->code = mpeg2dec->buf_start[-1];
        mpeg2dec->action = mpeg2_seek_header;
        return STATE_INVALID;
    }

    mpeg2dec->code = mpeg2dec->buf_start[-1];
    switch (RECEIVED (mpeg2dec->code, mpeg2dec->state)) {

    /* state transition after a sequence header */
    case RECEIVED (0x00, STATE_SEQUENCE):
    case RECEIVED (0xb8, STATE_SEQUENCE):
        mpeg2_header_sequence_finalize (mpeg2dec);
        break;

    /* other legal state transitions */
    case RECEIVED (0x00, STATE_GOP):
        mpeg2_header_gop_finalize (mpeg2dec);
        break;
    case RECEIVED (0x01, STATE_PICTURE):
    case RECEIVED (0x01, STATE_PICTURE_2ND):
        mpeg2_header_picture_finalize (mpeg2dec, mpeg2_accels);
        mpeg2dec->action = mpeg2_header_slice_start;
        break;

    /* legal headers within a given state */
    case RECEIVED (0xb2, STATE_SEQUENCE):
    case RECEIVED (0xb2, STATE_GOP):
    case RECEIVED (0xb2, STATE_PICTURE):
    case RECEIVED (0xb2, STATE_PICTURE_2ND):
    case RECEIVED (0xb5, STATE_SEQUENCE):
    case RECEIVED (0xb5, STATE_PICTURE):
    case RECEIVED (0xb5, STATE_PICTURE_2ND):
        mpeg2dec->chunk_ptr = mpeg2dec->chunk_start;
        continue;

    default:
        mpeg2dec->action = mpeg2_seek_header;
        return STATE_INVALID;
    }

    mpeg2dec->chunk_start = mpeg2dec->chunk_ptr = mpeg2dec->chunk_buffer;
    mpeg2dec->user_data_len = 0;
    return mpeg2dec->state;
    }
}

int mpeg2_convert (mpeg2dec_t * mpeg2dec, mpeg2_convert_t convert, void * arg)
{
    mpeg2_convert_init_t convert_init;
    int error;

    error = convert (MPEG2_CONVERT_SET, NULL, &(mpeg2dec->sequence), 0,
             mpeg2_accels, arg, &convert_init);
    if (!error) {
    mpeg2dec->convert = convert;
    mpeg2dec->convert_arg = arg;
    mpeg2dec->convert_id_size = convert_init.id_size;
    mpeg2dec->convert_stride = 0;
    }
    return error;
}

int mpeg2_stride (mpeg2dec_t * mpeg2dec, int stride)
{
    if (!mpeg2dec->convert) {
    if (stride < (int) mpeg2dec->sequence.width)
        stride = mpeg2dec->sequence.width;
    mpeg2dec->decoder.stride_frame = stride;
    } else {
    mpeg2_convert_init_t convert_init;

    stride = mpeg2dec->convert (MPEG2_CONVERT_STRIDE, NULL,
                    &(mpeg2dec->sequence), stride,
                    mpeg2_accels, mpeg2dec->convert_arg,
                    &convert_init);
    mpeg2dec->convert_id_size = convert_init.id_size;
    mpeg2dec->convert_stride = stride;
    }
    return stride;
}

void mpeg2_set_buf (mpeg2dec_t * mpeg2dec, uint8_t * buf[3], void * id)
{
    mpeg2_fbuf_t * fbuf;

    if (mpeg2dec->custom_fbuf) {
    if (mpeg2dec->state == STATE_SEQUENCE) {
        mpeg2dec->fbuf[2] = mpeg2dec->fbuf[1];
        mpeg2dec->fbuf[1] = mpeg2dec->fbuf[0];
    }
    mpeg2_set_fbuf (mpeg2dec, (mpeg2dec->decoder.coding_type ==
                   PIC_FLAG_CODING_TYPE_B));
    fbuf = mpeg2dec->fbuf[0];
    } else {
    fbuf = &(mpeg2dec->fbuf_alloc[mpeg2dec->alloc_index].fbuf);
    mpeg2dec->alloc_index_user = ++mpeg2dec->alloc_index;
    }
    fbuf->buf[0] = buf[0];
    fbuf->buf[1] = buf[1];
    fbuf->buf[2] = buf[2];
    fbuf->id = id;
}

void mpeg2_custom_fbuf (mpeg2dec_t * mpeg2dec, int custom_fbuf)
{
    mpeg2dec->custom_fbuf = custom_fbuf;
}

void mpeg2_skip (mpeg2dec_t * mpeg2dec, int skip)
{
    mpeg2dec->first_decode_slice = 1;
    mpeg2dec->nb_decode_slices = skip ? 0 : (0xb0 - 1);
}

void mpeg2_slice_region (mpeg2dec_t * mpeg2dec, int start, int end)
{
    start = (start < 1) ? 1 : (start > 0xb0) ? 0xb0 : start;
    end = (end < start) ? start : (end > 0xb0) ? 0xb0 : end;
    mpeg2dec->first_decode_slice = start;
    mpeg2dec->nb_decode_slices = end - start;
}

void mpeg2_tag_picture (mpeg2dec_t * mpeg2dec, uint32_t tag, uint32_t tag2)
{
    mpeg2dec->tag_previous = mpeg2dec->tag_current;
    mpeg2dec->tag2_previous = mpeg2dec->tag2_current;
    mpeg2dec->tag_current = tag;
    mpeg2dec->tag2_current = tag2;
    mpeg2dec->num_tags++;
    mpeg2dec->bytes_since_tag = 0;
}

/*
 * In legal streams, the IDCT output should be between -384 and +384.
 * In corrupted streams, it is possible to force the IDCT output to go
 * to +-3826 - this is the worst case for a column IDCT where the
 * column inputs are 16-bit values.
 */
uint8_t mpeg2_clip[3840 * 2 + 256];
#define CLIP(i) ((mpeg2_clip + 3840)[i])

#define W1 2841 /* 2048 * sqrt (2) * cos (1 * pi / 16) */
#define W2 2676 /* 2048 * sqrt (2) * cos (2 * pi / 16) */
#define W3 2408 /* 2048 * sqrt (2) * cos (3 * pi / 16) */
#define W5 1609 /* 2048 * sqrt (2) * cos (5 * pi / 16) */
#define W6 1108 /* 2048 * sqrt (2) * cos (6 * pi / 16) */
#define W7 565  /* 2048 * sqrt (2) * cos (7 * pi / 16) */

/* idct main entry point  */
void (* mpeg2_idct_copy) (int16_t * block, uint8_t * dest, int stride);
void (* mpeg2_idct_add) (int last, int16_t * block,
			 uint8_t * dest, int stride);



#if 0
#define BUTTERFLY(t0,t1,W0,W1,d0,d1)	\
do {					\
    t0 = W0 * d0 + W1 * d1;		\
    t1 = W0 * d1 - W1 * d0;		\
} while (0)
#else
#define BUTTERFLY(t0,t1,W0,W1,d0,d1)	\
do {					\
    int tmp = W0 * (d0 + d1);		\
    t0 = tmp + (W1 - W0) * d1;		\
    t1 = tmp - (W1 + W0) * d0;		\
} while (0)
#endif

static inline void idct_row (int16_t * const block)
{
    int d0, d1, d2, d3;
    int a0, a1, a2, a3, b0, b1, b2, b3;
    int t0, t1, t2, t3;

    /* shortcut */
    if (likely (!(block[1] | ((int32_t *)block)[1] | ((int32_t *)block)[2] |
		  ((int32_t *)block)[3]))) {
	uint32_t tmp = (uint16_t) (block[0] >> 1);
	tmp |= tmp << 16;
	((int32_t *)block)[0] = tmp;
	((int32_t *)block)[1] = tmp;
	((int32_t *)block)[2] = tmp;
	((int32_t *)block)[3] = tmp;
	return;
    }

    d0 = (block[0] << 11) + 2048;
    d1 = block[1];
    d2 = block[2] << 11;
    d3 = block[3];
    t0 = d0 + d2;
    t1 = d0 - d2;
    BUTTERFLY (t2, t3, W6, W2, d3, d1);
    a0 = t0 + t2;
    a1 = t1 + t3;
    a2 = t1 - t3;
    a3 = t0 - t2;

    d0 = block[4];
    d1 = block[5];
    d2 = block[6];
    d3 = block[7];
    BUTTERFLY (t0, t1, W7, W1, d3, d0);
    BUTTERFLY (t2, t3, W3, W5, d1, d2);
    b0 = t0 + t2;
    b3 = t1 + t3;
    t0 -= t2;
    t1 -= t3;
    b1 = ((t0 + t1) >> 8) * 181;
    b2 = ((t0 - t1) >> 8) * 181;

    block[0] = a0 + b0 >> 12;
    block[1] = a1 + b1 >> 12;
    block[2] = a2 + b2 >> 12;
    block[3] = a3 + b3 >> 12;
    block[4] = a3 - b3 >> 12;
    block[5] = a2 - b2 >> 12;
    block[6] = a1 - b1 >> 12;
    block[7] = a0 - b0 >> 12;
}

static inline void idct_col (int16_t * const block)
{
    int d0, d1, d2, d3;
    int a0, a1, a2, a3, b0, b1, b2, b3;
    int t0, t1, t2, t3;

    d0 = (block[8*0] << 11) + 65536;
    d1 = block[8*1];
    d2 = block[8*2] << 11;
    d3 = block[8*3];
    t0 = d0 + d2;
    t1 = d0 - d2;
    BUTTERFLY (t2, t3, W6, W2, d3, d1);
    a0 = t0 + t2;
    a1 = t1 + t3;
    a2 = t1 - t3;
    a3 = t0 - t2;

    d0 = block[8*4];
    d1 = block[8*5];
    d2 = block[8*6];
    d3 = block[8*7];
    BUTTERFLY (t0, t1, W7, W1, d3, d0);
    BUTTERFLY (t2, t3, W3, W5, d1, d2);
    b0 = t0 + t2;
    b3 = t1 + t3;
    t0 -= t2;
    t1 -= t3;
    b1 = ((t0 + t1) >> 8) * 181;
    b2 = ((t0 - t1) >> 8) * 181;

    block[8*0] = (a0 + b0) >> 17;
    block[8*1] = (a1 + b1) >> 17;
    block[8*2] = (a2 + b2) >> 17;
    block[8*3] = (a3 + b3) >> 17;
    block[8*4] = (a3 - b3) >> 17;
    block[8*5] = (a2 - b2) >> 17;
    block[8*6] = (a1 - b1) >> 17;
    block[8*7] = (a0 - b0) >> 17;
}

static void mpeg2_idct_add_c (const int last, int16_t * block, uint8_t * dest, const int stride)
{
    int i;

    if (last != 129 || (block[0] & (7 << 4)) == (4 << 4))
    {
        for (i = 0; i < 8; i++)
	        idct_row (block + 8 * i);
        for (i = 0; i < 8; i++)
    	    idct_col (block + i);
        do {
            dest[0] = CLIP (block[0] + dest[0]);
            dest[1] = CLIP (block[1] + dest[1]);
            dest[2] = CLIP (block[2] + dest[2]);
            dest[3] = CLIP (block[3] + dest[3]);
            dest[4] = CLIP (block[4] + dest[4]);
            dest[5] = CLIP (block[5] + dest[5]);
            dest[6] = CLIP (block[6] + dest[6]);
            dest[7] = CLIP (block[7] + dest[7]);

            ((int32_t *)block)[0] = 0;
            ((int32_t *)block)[1] = 0;
            ((int32_t *)block)[2] = 0;
            ((int32_t *)block)[3] = 0;

            dest += stride;
            block += 8;
        } while (--i);
    } else
    {
        int DC = block[0] + 64 >> 7;
        block[0] = block[63] = 0;
        i = 8;
        do {
	    dest[0] = CLIP (DC + dest[0]);
	    dest[1] = CLIP (DC + dest[1]);
	    dest[2] = CLIP (DC + dest[2]);
	    dest[3] = CLIP (DC + dest[3]);
	    dest[4] = CLIP (DC + dest[4]);
	    dest[5] = CLIP (DC + dest[5]);
	    dest[6] = CLIP (DC + dest[6]);
	    dest[7] = CLIP (DC + dest[7]);
	    dest += stride;
        } while (--i);
    }
}

static void mpeg2_idct_copy_c(int16_t *block, uint8_t *dest, const int stride)
{
    int i;

    for (i = 0; i < 8; i++)
        idct_row (block + 8 * i);
    for (i = 0; i < 8; i++)
        idct_col (block + i);
    do {
	dest[0] = CLIP (block[0]);
	dest[1] = CLIP (block[1]);
	dest[2] = CLIP (block[2]);
	dest[3] = CLIP (block[3]);
	dest[4] = CLIP (block[4]);
	dest[5] = CLIP (block[5]);
	dest[6] = CLIP (block[6]);
	dest[7] = CLIP (block[7]);

	((int32_t *)block)[0] = 0;	((int32_t *)block)[1] = 0;
	((int32_t *)block)[2] = 0;	((int32_t *)block)[3] = 0;

	dest += stride;
	block += 8;
    } while (--i);
}

static void mpeg2_idct_init (uint32_t accel)
{
	mpeg2_idct_copy = mpeg2_idct_copy_c;
	mpeg2_idct_add = mpeg2_idct_add_c;
	for (int i = -3840; i < 3840 + 256; i++)
	    CLIP(i) = (i < 0) ? 0 : ((i > 255) ? 255 : i);

	for (int i = 0; i < 64; i++) {
        int j;
	    j = mpeg2_scan_norm[i];
	    mpeg2_scan_norm[i] = (j & 0x36) >> 1 | (j & 0x09) << 2;
	    j = mpeg2_scan_alt[i];
	    mpeg2_scan_alt[i] = ((j & 0x36) >> 1) | ((j & 0x09) << 2);
	}
}

uint32_t mpeg2_accel (uint32_t accel)
{
    if (!mpeg2_accels) {
    //mpeg2_accels = mpeg2_detect_accel (accel) | MPEG2_ACCEL_DETECT;
    mpeg2_accels = accel | MPEG2_ACCEL_DETECT;
    mpeg2_idct_init (mpeg2_accels);
    mpeg2_mc_init (mpeg2_accels);
    }
    return mpeg2_accels & ~MPEG2_ACCEL_DETECT;
}

void mpeg2_reset (mpeg2dec_t * mpeg2dec, int full_reset)
{
    mpeg2dec->buf_start = mpeg2dec->buf_end = NULL;
    mpeg2dec->num_tags = 0;
    mpeg2dec->shift = 0xffffff00;
    mpeg2dec->code = 0xb4;
    mpeg2dec->action = mpeg2_seek_header;
    mpeg2dec->state = STATE_INVALID;
    mpeg2dec->first = 1;

    mpeg2_reset_info(&(mpeg2dec->info));
    mpeg2dec->info.gop = NULL;
    mpeg2dec->info.user_data = NULL;
    mpeg2dec->info.user_data_len = 0;
    if (full_reset) {
        mpeg2dec->info.sequence = NULL;
        mpeg2_header_state_init (mpeg2dec);
    }

}

mpeg2dec_t * mpeg2_init (void)
{
    mpeg2dec_t * mpeg2dec;
    mpeg2_accel (MPEG2_ACCEL_DETECT);

    mpeg2dec = (mpeg2dec_t *) mpeg2_malloc (sizeof (mpeg2dec_t),
                        MPEG2_ALLOC_MPEG2DEC);

    if (mpeg2dec == NULL)
        return NULL;

    memset (mpeg2dec->decoder.DCTblock, 0, 64 * sizeof (int16_t));
    memset (mpeg2dec->quantizer_matrix, 0, 4 * 64 * sizeof (uint8_t));

    mpeg2dec->chunk_buffer = (uint8_t *) mpeg2_malloc (BUFFER_SIZE + 4,
                               MPEG2_ALLOC_CHUNK);

    mpeg2dec->sequence.width = (unsigned)-1;
    mpeg2_reset (mpeg2dec, 1);
    return mpeg2dec;
}

void mpeg2_close (mpeg2dec_t * mpeg2dec)
{
    mpeg2_header_state_init (mpeg2dec);
    mpeg2_free (mpeg2dec->chunk_buffer);
    mpeg2_free (mpeg2dec);
}



void mpeg2_header_state_init (mpeg2dec_t * mpeg2dec)
{
    if (mpeg2dec->sequence.width != (unsigned)-1) {
	int i;

	mpeg2dec->sequence.width = (unsigned)-1;
	if (!mpeg2dec->custom_fbuf)
	    for (i = mpeg2dec->alloc_index_user;
		 i < mpeg2dec->alloc_index; i++) {
		mpeg2_free (mpeg2dec->fbuf_alloc[i].fbuf.buf[0]);
		mpeg2_free (mpeg2dec->fbuf_alloc[i].fbuf.buf[1]);
		mpeg2_free (mpeg2dec->fbuf_alloc[i].fbuf.buf[2]);
	    }
	if (mpeg2dec->convert_start)
	    for (i = 0; i < 3; i++) {
		mpeg2_free (mpeg2dec->yuv_buf[i][0]);
		mpeg2_free (mpeg2dec->yuv_buf[i][1]);
		mpeg2_free (mpeg2dec->yuv_buf[i][2]);
	    }
	if (mpeg2dec->decoder.convert_id)
	    mpeg2_free (mpeg2dec->decoder.convert_id);
    }
    mpeg2dec->decoder.coding_type = I_TYPE;
    mpeg2dec->decoder.convert = NULL;
    mpeg2dec->decoder.convert_id = NULL;
    mpeg2dec->picture = mpeg2dec->pictures;
    mpeg2dec->fbuf[0] = &mpeg2dec->fbuf_alloc[0].fbuf;
    mpeg2dec->fbuf[1] = &mpeg2dec->fbuf_alloc[1].fbuf;
    mpeg2dec->fbuf[2] = &mpeg2dec->fbuf_alloc[2].fbuf;
    mpeg2dec->first = 1;
    mpeg2dec->alloc_index = 0;
    mpeg2dec->alloc_index_user = 0;
    mpeg2dec->first_decode_slice = 1;
    mpeg2dec->nb_decode_slices = 0xb0 - 1;
    mpeg2dec->convert = NULL;
    mpeg2dec->convert_start = NULL;
    mpeg2dec->custom_fbuf = 0;
    mpeg2dec->yuv_index = 0;
}

void mpeg2_reset_info (mpeg2_info_t * info)
{
    info->current_picture = info->current_picture_2nd = NULL;
    info->display_picture = info->display_picture_2nd = NULL;
    info->current_fbuf = info->display_fbuf = info->discard_fbuf = NULL;
}

static void info_user_data (mpeg2dec_t * mpeg2dec)
{
    if (mpeg2dec->user_data_len) {
	mpeg2dec->info.user_data = mpeg2dec->chunk_buffer;
	mpeg2dec->info.user_data_len = mpeg2dec->user_data_len - 3;
    }
}

int mpeg2_header_sequence (mpeg2dec_t * mpeg2dec)
{
    uint8_t * buffer = mpeg2dec->chunk_start;
    mpeg2_sequence_t * sequence = &(mpeg2dec->new_sequence);
    static unsigned int frame_period[16] = {
	0, 1126125, 1125000, 1080000, 900900, 900000, 540000, 450450, 450000,
	/* unofficial: xing 15 fps */
	1800000,
	/* unofficial: libmpeg3 "Unofficial economy rates" 5/10/12/15 fps */
	5400000, 2700000, 2250000, 1800000, 0, 0
    };
    int i;

    if ((buffer[6] & 0x20) != 0x20)	/* missing marker_bit */
	return 1;

    i = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
    if (! (sequence->display_width = sequence->picture_width = i >> 12))
	return 1;
    if (! (sequence->display_height = sequence->picture_height = i & 0xfff))
	return 1;
    sequence->width = (sequence->picture_width + 15) & ~15;
    sequence->height = (sequence->picture_height + 15) & ~15;
    sequence->chroma_width = sequence->width >> 1;
    sequence->chroma_height = sequence->height >> 1;

    sequence->flags = (SEQ_FLAG_PROGRESSIVE_SEQUENCE |
		       SEQ_VIDEO_FORMAT_UNSPECIFIED);

    sequence->pixel_width = buffer[3] >> 4;	/* aspect ratio */
    sequence->frame_period = frame_period[buffer[3] & 15];

    sequence->byte_rate = (buffer[4]<<10) | (buffer[5]<<2) | (buffer[6]>>6);

    sequence->vbv_buffer_size = ((buffer[6]<<16)|(buffer[7]<<8))&0x1ff800;

    if (buffer[7] & 4)
	sequence->flags |= SEQ_FLAG_CONSTRAINED_PARAMETERS;

    mpeg2dec->copy_matrix = 3;
    if (buffer[7] & 2) {
	for (i = 0; i < 64; i++)
	    mpeg2dec->new_quantizer_matrix[0][mpeg2_scan_norm[i]] =
		(buffer[i+7] << 7) | (buffer[i+8] >> 1);
	buffer += 64;
    } else
	for (i = 0; i < 64; i++)
	    mpeg2dec->new_quantizer_matrix[0][mpeg2_scan_norm[i]] =
		default_intra_quantizer_matrix[i];

    if (buffer[7] & 1)
	for (i = 0; i < 64; i++)
	    mpeg2dec->new_quantizer_matrix[1][mpeg2_scan_norm[i]] =
		buffer[i+8];
    else
	memset (mpeg2dec->new_quantizer_matrix[1], 16, 64);

    sequence->profile_level_id = 0x80;
    sequence->colour_primaries = 0;
    sequence->transfer_characteristics = 0;
    sequence->matrix_coefficients = 0;

    mpeg2dec->ext_state = SEQ_EXT;
    mpeg2dec->state = STATE_SEQUENCE;
    mpeg2dec->display_offset_x = mpeg2dec->display_offset_y = 0;

    return 0;
}

static int sequence_ext (mpeg2dec_t * mpeg2dec)
{
    uint8_t * buffer = mpeg2dec->chunk_start;
    mpeg2_sequence_t * sequence = &(mpeg2dec->new_sequence);
    uint32_t flags;

    if (!(buffer[3] & 1))
	return 1;

    sequence->profile_level_id = (buffer[0] << 4) | (buffer[1] >> 4);

    sequence->display_width = sequence->picture_width +=
	((buffer[1] << 13) | (buffer[2] << 5)) & 0x3000;
    sequence->display_height = sequence->picture_height +=
	(buffer[2] << 7) & 0x3000;
    sequence->width = (sequence->picture_width + 15) & ~15;
    sequence->height = (sequence->picture_height + 15) & ~15;
    flags = sequence->flags | SEQ_FLAG_MPEG2;
    if (!(buffer[1] & 8)) {
	flags &= ~SEQ_FLAG_PROGRESSIVE_SEQUENCE;
	sequence->height = (sequence->height + 31) & ~31;
    }
    if (buffer[5] & 0x80)
	flags |= SEQ_FLAG_LOW_DELAY;
    sequence->flags = flags;
    sequence->chroma_width = sequence->width;
    sequence->chroma_height = sequence->height;
    switch (buffer[1] & 6) {
    case 0:	/* invalid */
	return 1;
    case 2:	/* 4:2:0 */
	sequence->chroma_height >>= 1;
    case 4:	/* 4:2:2 */
	sequence->chroma_width >>= 1;
    }

    sequence->byte_rate += ((buffer[2]<<25) | (buffer[3]<<17)) & 0x3ffc0000;

    sequence->vbv_buffer_size |= buffer[4] << 21;

    sequence->frame_period =
	sequence->frame_period * ((buffer[5]&31)+1) / (((buffer[5]>>5)&3)+1);

    mpeg2dec->ext_state = SEQ_DISPLAY_EXT;

    return 0;
}

static int sequence_display_ext (mpeg2dec_t * mpeg2dec)
{
    uint8_t * buffer = mpeg2dec->chunk_start;
    mpeg2_sequence_t * sequence = &(mpeg2dec->new_sequence);

    sequence->flags = ((sequence->flags & ~SEQ_MASK_VIDEO_FORMAT) |
		       ((buffer[0]<<4) & SEQ_MASK_VIDEO_FORMAT));
    if (buffer[0] & 1) {
	sequence->flags |= SEQ_FLAG_COLOUR_DESCRIPTION;
	sequence->colour_primaries = buffer[1];
	sequence->transfer_characteristics = buffer[2];
	sequence->matrix_coefficients = buffer[3];
	buffer += 3;
    }

    if (!(buffer[2] & 2))	/* missing marker_bit */
	return 1;

    if( (buffer[1] << 6) | (buffer[2] >> 2) )
	sequence->display_width = (buffer[1] << 6) | (buffer[2] >> 2);
    if( ((buffer[2]& 1 ) << 13) | (buffer[3] << 5) | (buffer[4] >> 3) )
	sequence->display_height =
	    ((buffer[2]& 1 ) << 13) | (buffer[3] << 5) | (buffer[4] >> 3);

    return 0;
}

static inline void simplify (unsigned int * u, unsigned int * v)
{
    unsigned int a, b, tmp;

    a = *u;
    b = *v;
    while (a) {	/* find greatest common divisor */
	    tmp = a;
        a = b % tmp;
        b = tmp;
    }
    *u /= b;
    *v /= b;
}

static inline void finalize_sequence (mpeg2_sequence_t * sequence)
{
    int width;
    int height;

    sequence->byte_rate *= 50;

    if (sequence->flags & SEQ_FLAG_MPEG2)
    {
        switch (sequence->pixel_width)
        {
        case 1:		/* square pixels */
        sequence->pixel_width = sequence->pixel_height = 1;
        return;
        case 2:		/* 4:3 aspect ratio */
        width = 4; height = 3;
        break;
        case 3:		/* 16:9 aspect ratio */
        width = 16; height = 9;
        break;
        case 4:		/* 2.21:1 aspect ratio */
        width = 221; height = 100;
        break;
        default:	/* illegal */
	    sequence->pixel_width = sequence->pixel_height = 0;
        return;
        }
        width *= sequence->display_height;
        height *= sequence->display_width;

    } else {
	if (sequence->byte_rate == 50 * 0x3ffff) 
	    sequence->byte_rate = 0;        /* mpeg-1 VBR */ 

	switch (sequence->pixel_width) {
    case 0:
    case 15:	/* illegal */
	    sequence->pixel_width = sequence->pixel_height = 0;
        return;
    case 1:	/* square pixels */
	    sequence->pixel_width = sequence->pixel_height = 1;		return;
    case 3:	/* 720x576 16:9 */
	    sequence->pixel_width = 64;	sequence->pixel_height = 45;	return;
    case 6:	/* 720x480 16:9 */
	    sequence->pixel_width = 32;	sequence->pixel_height = 27;	return;
    case 8: /* BT.601 625 lines 4:3 */
	    sequence->pixel_width = 59;	sequence->pixel_height = 54;	return;
    case 12: /* BT.601 525 lines 4:3 */
	    sequence->pixel_width = 10;	sequence->pixel_height = 11;	return;
    default:
	    height = 88 * sequence->pixel_width + 1171;
	    width = 2000;
	}
    }

    sequence->pixel_width = width;
    sequence->pixel_height = height;
    simplify (&sequence->pixel_width, &sequence->pixel_height);
}

int mpeg2_guess_aspect (const mpeg2_sequence_t * sequence,
			unsigned int * pixel_width,
			unsigned int * pixel_height)
{
    static struct {
	unsigned int width, height;
    } video_modes[] = {
	{720, 576}, /* 625 lines, 13.5 MHz (D1, DV, DVB, DVD) */
	{704, 576}, /* 625 lines, 13.5 MHz (1/1 D1, DVB, DVD, 4CIF) */
	{544, 576}, /* 625 lines, 10.125 MHz (DVB, laserdisc) */
	{528, 576}, /* 625 lines, 10.125 MHz (3/4 D1, DVB, laserdisc) */
	{480, 576}, /* 625 lines, 9 MHz (2/3 D1, DVB, SVCD) */
	{352, 576}, /* 625 lines, 6.75 MHz (D2, 1/2 D1, CVD, DVB, DVD) */
	{352, 288}, /* 625 lines, 6.75 MHz, 1 field (D4, VCD, DVB, DVD, CIF) */
	{176, 144}, /* 625 lines, 3.375 MHz, half field (QCIF) */
	{720, 486}, /* 525 lines, 13.5 MHz (D1) */
	{704, 486}, /* 525 lines, 13.5 MHz */
	{720, 480}, /* 525 lines, 13.5 MHz (DV, DSS, DVD) */
	{704, 480}, /* 525 lines, 13.5 MHz (1/1 D1, ATSC, DVD) */
	{544, 480}, /* 525 lines. 10.125 MHz (DSS, laserdisc) */
	{528, 480}, /* 525 lines. 10.125 MHz (3/4 D1, laserdisc) */
	{480, 480}, /* 525 lines, 9 MHz (2/3 D1, SVCD) */
	{352, 480}, /* 525 lines, 6.75 MHz (D2, 1/2 D1, CVD, DVD) */
	{352, 240}  /* 525  lines. 6.75 MHz, 1 field (D4, VCD, DSS, DVD) */
    };
    unsigned int width, height, pix_width, pix_height, i, DAR_16_9;

    *pixel_width = sequence->pixel_width;
    *pixel_height = sequence->pixel_height;
    width = sequence->picture_width;
    height = sequence->picture_height;
    for (i = 0; i < sizeof (video_modes) / sizeof (video_modes[0]); i++)
	if (width == video_modes[i].width && height == video_modes[i].height)
	    break;
    if (i == sizeof (video_modes) / sizeof (video_modes[0]) ||
	(sequence->pixel_width == 1 && sequence->pixel_height == 1) ||
	width != sequence->display_width || height != sequence->display_height)
	return 0;

    for (pix_height = 1; height * pix_height < 480; pix_height <<= 1);
    height *= pix_height;
    for (pix_width = 1; width * pix_width <= 352; pix_width <<= 1);
    width *= pix_width;

    if (! (sequence->flags & SEQ_FLAG_MPEG2)) {
	static unsigned int mpeg1_check[2][2] = {{11, 54}, {27, 45}};
	DAR_16_9 = (sequence->pixel_height == 27 ||
		    sequence->pixel_height == 45);
	if (width < 704 ||
	    sequence->pixel_height != mpeg1_check[DAR_16_9][height == 576])
	    return 0;
    } else {
	DAR_16_9 = (3 * sequence->picture_width * sequence->pixel_width >
		    4 * sequence->picture_height * sequence->pixel_height);
	switch (width) {
	case 528: case 544:	pix_width *= 4; pix_height *= 3; break;
	case 480:		pix_width *= 3; pix_height *= 2; break;
	}
    }
    if (DAR_16_9) {
	pix_width *= 4; pix_height *= 3;
    }
    if (height == 576) {
	pix_width *= 59; pix_height *= 54;
    } else {
	pix_width *= 10; pix_height *= 11;
    }
    *pixel_width = pix_width;
    *pixel_height = pix_height;
    simplify (pixel_width, pixel_height);
    return (height == 576) ? 1 : 2;
}

static void copy_matrix (mpeg2dec_t * mpeg2dec, int idx)
{
    if (memcmp (mpeg2dec->quantizer_matrix[idx],
		mpeg2dec->new_quantizer_matrix[idx], 64)) {
	memcpy (mpeg2dec->quantizer_matrix[idx],
		mpeg2dec->new_quantizer_matrix[idx], 64);
	mpeg2dec->scaled[idx] = -1;
    }
}

static void finalize_matrix (mpeg2dec_t * mpeg2dec)
{
    mpeg2_decoder_t * decoder = &(mpeg2dec->decoder);
    int i;

    for (i = 0; i < 2; i++) {
	if (mpeg2dec->copy_matrix & (1 << i))
	    copy_matrix (mpeg2dec, i);
	if ((mpeg2dec->copy_matrix & (4 << i)) &&
	    memcmp (mpeg2dec->quantizer_matrix[i],
		    mpeg2dec->new_quantizer_matrix[i+2], 64)) {
	    copy_matrix (mpeg2dec, i + 2);
	    decoder->chroma_quantizer[i] = decoder->quantizer_prescale[i+2];
	} else if (mpeg2dec->copy_matrix & (5 << i))
	    decoder->chroma_quantizer[i] = decoder->quantizer_prescale[i];
    }
}

static mpeg2_state_t invalid_end_action (mpeg2dec_t * mpeg2dec)
{
    mpeg2_reset_info (&(mpeg2dec->info));
    mpeg2dec->info.gop = NULL;
    info_user_data (mpeg2dec);
    mpeg2_header_state_init (mpeg2dec);
    mpeg2dec->sequence = mpeg2dec->new_sequence;
    mpeg2dec->action = mpeg2_seek_header;
    mpeg2dec->state = STATE_SEQUENCE;
    return STATE_SEQUENCE;
}

void mpeg2_header_sequence_finalize (mpeg2dec_t * mpeg2dec)
{
    mpeg2_sequence_t * sequence = &(mpeg2dec->new_sequence);
    mpeg2_decoder_t * decoder = &(mpeg2dec->decoder);

    finalize_sequence (sequence);
    finalize_matrix (mpeg2dec);

    decoder->mpeg1 = !(sequence->flags & SEQ_FLAG_MPEG2);
    decoder->width = sequence->width;
    decoder->height = sequence->height;
    decoder->vertical_position_extension = (sequence->picture_height > 2800);
    decoder->chroma_format = ((sequence->chroma_width == sequence->width) +
			      (sequence->chroma_height == sequence->height));

    if (mpeg2dec->sequence.width != (unsigned)-1) {
	/*
	 * According to 6.1.1.6, repeat sequence headers should be
	 * identical to the original. However some encoders do not
	 * respect that and change various fields (including bitrate
	 * and aspect ratio) in the repeat sequence headers. So we
	 * choose to be as conservative as possible and only restart
	 * the decoder if the width, height, chroma_width,
	 * chroma_height or low_delay flag are modified.
	 */
	if (sequence->width != mpeg2dec->sequence.width ||
	    sequence->height != mpeg2dec->sequence.height ||
	    sequence->chroma_width != mpeg2dec->sequence.chroma_width ||
	    sequence->chroma_height != mpeg2dec->sequence.chroma_height ||
	    ((sequence->flags ^ mpeg2dec->sequence.flags) &
	     SEQ_FLAG_LOW_DELAY)) {
	    decoder->stride_frame = sequence->width;
	    mpeg2_header_end (mpeg2dec);
	    mpeg2dec->action = invalid_end_action;
	    mpeg2dec->state = STATE_INVALID_END;
	    return;
	}
	mpeg2dec->state = (memcmp (&(mpeg2dec->sequence), sequence,
				   sizeof (mpeg2_sequence_t)) ?
			   STATE_SEQUENCE_MODIFIED : STATE_SEQUENCE_REPEATED);
    } else
	decoder->stride_frame = sequence->width;
    mpeg2dec->sequence = *sequence;
    mpeg2_reset_info (&(mpeg2dec->info));
    mpeg2dec->info.sequence = &(mpeg2dec->sequence);
    mpeg2dec->info.gop = NULL;
    info_user_data (mpeg2dec);
}

int mpeg2_header_gop (mpeg2dec_t * mpeg2dec)
{
    uint8_t * buffer = mpeg2dec->chunk_start;
    mpeg2_gop_t * gop = &(mpeg2dec->new_gop);

    if (! (buffer[1] & 8))
        return 1;

    gop->hours = (buffer[0] >> 2) & 31;
    gop->minutes = ((buffer[0] << 4) | (buffer[1] >> 4)) & 63;
    gop->seconds = ((buffer[1] << 3) | (buffer[2] >> 5)) & 63;
    gop->pictures = ((buffer[2] << 1) | (buffer[3] >> 7)) & 63;
    gop->flags = (buffer[0] >> 7) | ((buffer[3] >> 4) & 6);
    mpeg2dec->state = STATE_GOP;
    return 0;
}

void mpeg2_header_gop_finalize (mpeg2dec_t * mpeg2dec)
{
    mpeg2dec->gop = mpeg2dec->new_gop;
    mpeg2_reset_info (&(mpeg2dec->info));
    mpeg2dec->info.gop = &(mpeg2dec->gop);
    info_user_data (mpeg2dec);
}

void mpeg2_set_fbuf (mpeg2dec_t * mpeg2dec, int b_type)
{
    int i;

    for (i = 0; i < 3; i++)
	if (mpeg2dec->fbuf[1] != &mpeg2dec->fbuf_alloc[i].fbuf &&
	    mpeg2dec->fbuf[2] != &mpeg2dec->fbuf_alloc[i].fbuf) {
	    mpeg2dec->fbuf[0] = &mpeg2dec->fbuf_alloc[i].fbuf;
	    mpeg2dec->info.current_fbuf = mpeg2dec->fbuf[0];
	    if (b_type || (mpeg2dec->sequence.flags & SEQ_FLAG_LOW_DELAY)) {
		if (b_type || mpeg2dec->convert)
		    mpeg2dec->info.discard_fbuf = mpeg2dec->fbuf[0];
		mpeg2dec->info.display_fbuf = mpeg2dec->fbuf[0];
	    }
	    break;
	}
}

int mpeg2_header_picture (mpeg2dec_t * mpeg2dec)
{
    uint8_t * buffer = mpeg2dec->chunk_start;
    mpeg2_picture_t * picture = &(mpeg2dec->new_picture);
    mpeg2_decoder_t * decoder = &(mpeg2dec->decoder);
    int type;

    mpeg2dec->state = ((mpeg2dec->state != STATE_SLICE_1ST) ?
		       STATE_PICTURE : STATE_PICTURE_2ND);
    mpeg2dec->ext_state = PIC_CODING_EXT;

    picture->temporal_reference = (buffer[0] << 2) | (buffer[1] >> 6);

    type = (buffer [1] >> 3) & 7;
    if (type == PIC_FLAG_CODING_TYPE_P || type == PIC_FLAG_CODING_TYPE_B) {
	/* forward_f_code and backward_f_code - used in mpeg1 only */
	decoder->f_motion.f_code[1] = (buffer[3] >> 2) & 1;
	decoder->f_motion.f_code[0] =
	    (((buffer[3] << 1) | (buffer[4] >> 7)) & 7) - 1;
	decoder->b_motion.f_code[1] = (buffer[4] >> 6) & 1;
	decoder->b_motion.f_code[0] = ((buffer[4] >> 3) & 7) - 1;
    }

    picture->flags = PIC_FLAG_PROGRESSIVE_FRAME | type;
    picture->tag = picture->tag2 = 0;
    if (mpeg2dec->num_tags) {
	if (mpeg2dec->bytes_since_tag >= mpeg2dec->chunk_ptr - buffer + 4) {
	    mpeg2dec->num_tags = 0;
	    picture->tag = mpeg2dec->tag_current;
	    picture->tag2 = mpeg2dec->tag2_current;
	    picture->flags |= PIC_FLAG_TAGS;
	} else if (mpeg2dec->num_tags > 1) {
	    mpeg2dec->num_tags = 1;
	    picture->tag = mpeg2dec->tag_previous;
	    picture->tag2 = mpeg2dec->tag2_previous;
	    picture->flags |= PIC_FLAG_TAGS;
	}
    }
    picture->nb_fields = 2;
    picture->display_offset[0].x = picture->display_offset[1].x =
	picture->display_offset[2].x = mpeg2dec->display_offset_x;
    picture->display_offset[0].y = picture->display_offset[1].y =
	picture->display_offset[2].y = mpeg2dec->display_offset_y;

    /* XXXXXX decode extra_information_picture as well */

    decoder->q_scale_type = 0;
    decoder->intra_dc_precision = 7;
    decoder->frame_pred_frame_dct = 1;
    decoder->concealment_motion_vectors = 0;
    decoder->scan = mpeg2_scan_norm;
    decoder->picture_structure = FRAME_PICTURE;
    mpeg2dec->copy_matrix = 0;

    return 0;
}

static int picture_coding_ext (mpeg2dec_t * mpeg2dec)
{
    uint8_t * buffer = mpeg2dec->chunk_start;
    mpeg2_picture_t * picture = &(mpeg2dec->new_picture);
    mpeg2_decoder_t * decoder = &(mpeg2dec->decoder);
    uint32_t flags;

    /* pre subtract 1 for use later in compute_motion_vector */
    decoder->f_motion.f_code[0] = (buffer[0] & 15) - 1;
    decoder->f_motion.f_code[1] = (buffer[1] >> 4) - 1;
    decoder->b_motion.f_code[0] = (buffer[1] & 15) - 1;
    decoder->b_motion.f_code[1] = (buffer[2] >> 4) - 1;

    flags = picture->flags;
    decoder->intra_dc_precision = 7 - ((buffer[2] >> 2) & 3);
    decoder->picture_structure = buffer[2] & 3;
    switch (decoder->picture_structure) {
    case TOP_FIELD:
	flags |= PIC_FLAG_TOP_FIELD_FIRST;
    case BOTTOM_FIELD:
	picture->nb_fields = 1;
	break;
    case FRAME_PICTURE:
	if (!(mpeg2dec->sequence.flags & SEQ_FLAG_PROGRESSIVE_SEQUENCE)) {
	    picture->nb_fields = (buffer[3] & 2) ? 3 : 2;
	    flags |= (buffer[3] & 128) ? PIC_FLAG_TOP_FIELD_FIRST : 0;
	    flags |= (buffer[3] &   2) ? PIC_FLAG_REPEAT_FIRST_FIELD : 0;
	} else
	    picture->nb_fields = (buffer[3]&2) ? ((buffer[3]&128) ? 6 : 4) : 2;
	break;
    default:
	return 1;
    }
    decoder->top_field_first = buffer[3] >> 7;
    decoder->frame_pred_frame_dct = (buffer[3] >> 6) & 1;
    decoder->concealment_motion_vectors = (buffer[3] >> 5) & 1;
    decoder->q_scale_type = buffer[3] & 16;
    decoder->intra_vlc_format = (buffer[3] >> 3) & 1;
    decoder->scan = (buffer[3] & 4) ? mpeg2_scan_alt : mpeg2_scan_norm;
    if (!(buffer[4] & 0x80))
	flags &= ~PIC_FLAG_PROGRESSIVE_FRAME;
    if (buffer[4] & 0x40)
	flags |= (((buffer[4]<<26) | (buffer[5]<<18) | (buffer[6]<<10)) &
		  PIC_MASK_COMPOSITE_DISPLAY) | PIC_FLAG_COMPOSITE_DISPLAY;
    picture->flags = flags;

    mpeg2dec->ext_state = PIC_DISPLAY_EXT | COPYRIGHT_EXT | QUANT_MATRIX_EXT;

    return 0;
}

static int picture_display_ext (mpeg2dec_t * mpeg2dec)
{
    uint8_t * buffer = mpeg2dec->chunk_start;
    mpeg2_picture_t * picture = &(mpeg2dec->new_picture);
    int i, nb_pos;

    nb_pos = picture->nb_fields;
    if (mpeg2dec->sequence.flags & SEQ_FLAG_PROGRESSIVE_SEQUENCE)
	nb_pos >>= 1;

    for (i = 0; i < nb_pos; i++) {
	int x, y;

	x = ((buffer[4*i] << 24) | (buffer[4*i+1] << 16) |
	     (buffer[4*i+2] << 8) | buffer[4*i+3]) >> (11-2*i);
	y = ((buffer[4*i+2] << 24) | (buffer[4*i+3] << 16) |
	     (buffer[4*i+4] << 8) | buffer[4*i+5]) >> (10-2*i);
	if (! (x & y & 1))
	    return 1;
	picture->display_offset[i].x = mpeg2dec->display_offset_x = x >> 1;
	picture->display_offset[i].y = mpeg2dec->display_offset_y = y >> 1;
    }
    for (; i < 3; i++) {
	picture->display_offset[i].x = mpeg2dec->display_offset_x;
	picture->display_offset[i].y = mpeg2dec->display_offset_y;
    }
    return 0;
}

void mpeg2_header_picture_finalize (mpeg2dec_t * mpeg2dec, uint32_t accels)
{
    mpeg2_decoder_t * decoder = &(mpeg2dec->decoder);
    int old_type_b = (decoder->coding_type == B_TYPE);
    int low_delay = mpeg2dec->sequence.flags & SEQ_FLAG_LOW_DELAY;

    finalize_matrix (mpeg2dec);
    decoder->coding_type = mpeg2dec->new_picture.flags & PIC_MASK_CODING_TYPE;

    if (mpeg2dec->state == STATE_PICTURE) {
	mpeg2_picture_t * picture;
	mpeg2_picture_t * other;

	decoder->second_field = 0;

	picture = other = mpeg2dec->pictures;
	if (old_type_b ^ (mpeg2dec->picture < mpeg2dec->pictures + 2))
	    picture += 2;
	else
	    other += 2;
	mpeg2dec->picture = picture;
	*picture = mpeg2dec->new_picture;

	if (!old_type_b) {
	    mpeg2dec->fbuf[2] = mpeg2dec->fbuf[1];
	    mpeg2dec->fbuf[1] = mpeg2dec->fbuf[0];
	}
	mpeg2dec->fbuf[0] = NULL;
	mpeg2_reset_info (&(mpeg2dec->info));
	mpeg2dec->info.current_picture = picture;
	mpeg2dec->info.display_picture = picture;
	if (decoder->coding_type != B_TYPE) {
	    if (!low_delay) {
		if (mpeg2dec->first) {
		    mpeg2dec->info.display_picture = NULL;
		    mpeg2dec->first = 0;
		} else {
		    mpeg2dec->info.display_picture = other;
		    if (other->nb_fields == 1)
			mpeg2dec->info.display_picture_2nd = other + 1;
		    mpeg2dec->info.display_fbuf = mpeg2dec->fbuf[1];
		}
	    }
	    if (!low_delay + !mpeg2dec->convert)
		mpeg2dec->info.discard_fbuf =
		    mpeg2dec->fbuf[!low_delay + !mpeg2dec->convert];
	}
	if (mpeg2dec->convert) {
	    mpeg2_convert_init_t convert_init;
	    if (!mpeg2dec->convert_start) {
		int y_size, uv_size;

		mpeg2dec->decoder.convert_id =
		    mpeg2_malloc (mpeg2dec->convert_id_size,
				  MPEG2_ALLOC_CONVERT_ID);
		mpeg2dec->convert (MPEG2_CONVERT_START,
				   mpeg2dec->decoder.convert_id,
				   &(mpeg2dec->sequence),
				   mpeg2dec->convert_stride, accels,
				   mpeg2dec->convert_arg, &convert_init);
		mpeg2dec->convert_start = convert_init.start;
		mpeg2dec->decoder.convert = convert_init.copy;

		y_size = decoder->stride_frame * mpeg2dec->sequence.height;
		uv_size = y_size >> (2 - mpeg2dec->decoder.chroma_format);
		mpeg2dec->yuv_buf[0][0] =
		    (uint8_t *) mpeg2_malloc (y_size, MPEG2_ALLOC_YUV);
		mpeg2dec->yuv_buf[0][1] =
		    (uint8_t *) mpeg2_malloc (uv_size, MPEG2_ALLOC_YUV);
		mpeg2dec->yuv_buf[0][2] =
		    (uint8_t *) mpeg2_malloc (uv_size, MPEG2_ALLOC_YUV);
		mpeg2dec->yuv_buf[1][0] =
		    (uint8_t *) mpeg2_malloc (y_size, MPEG2_ALLOC_YUV);
		mpeg2dec->yuv_buf[1][1] =
		    (uint8_t *) mpeg2_malloc (uv_size, MPEG2_ALLOC_YUV);
		mpeg2dec->yuv_buf[1][2] =
		    (uint8_t *) mpeg2_malloc (uv_size, MPEG2_ALLOC_YUV);
		y_size = decoder->stride_frame * 32;
		uv_size = y_size >> (2 - mpeg2dec->decoder.chroma_format);
		mpeg2dec->yuv_buf[2][0] =
		    (uint8_t *) mpeg2_malloc (y_size, MPEG2_ALLOC_YUV);
		mpeg2dec->yuv_buf[2][1] =
		    (uint8_t *) mpeg2_malloc (uv_size, MPEG2_ALLOC_YUV);
		mpeg2dec->yuv_buf[2][2] =
		    (uint8_t *) mpeg2_malloc (uv_size, MPEG2_ALLOC_YUV);
	    }
	    if (!mpeg2dec->custom_fbuf) {
		while (mpeg2dec->alloc_index < 3) {
		    mpeg2_fbuf_t * fbuf;

		    fbuf = &mpeg2dec->fbuf_alloc[mpeg2dec->alloc_index++].fbuf;
		    fbuf->id = NULL;
		    fbuf->buf[0] =
			(uint8_t *) mpeg2_malloc (convert_init.buf_size[0],
						  MPEG2_ALLOC_CONVERTED);
		    fbuf->buf[1] =
			(uint8_t *) mpeg2_malloc (convert_init.buf_size[1],
						  MPEG2_ALLOC_CONVERTED);
		    fbuf->buf[2] =
			(uint8_t *) mpeg2_malloc (convert_init.buf_size[2],
						  MPEG2_ALLOC_CONVERTED);
		}
		mpeg2_set_fbuf (mpeg2dec, (decoder->coding_type == B_TYPE));
	    }
	} else if (!mpeg2dec->custom_fbuf) {
	    while (mpeg2dec->alloc_index < 3) {
		mpeg2_fbuf_t * fbuf;
		int y_size, uv_size;

		fbuf = &(mpeg2dec->fbuf_alloc[mpeg2dec->alloc_index++].fbuf);
		fbuf->id = NULL;
		y_size = decoder->stride_frame * mpeg2dec->sequence.height;
		uv_size = y_size >> (2 - decoder->chroma_format);
		fbuf->buf[0] = (uint8_t *) mpeg2_malloc (y_size,
							 MPEG2_ALLOC_YUV);
		fbuf->buf[1] = (uint8_t *) mpeg2_malloc (uv_size,
							 MPEG2_ALLOC_YUV);
		fbuf->buf[2] = (uint8_t *) mpeg2_malloc (uv_size,
							 MPEG2_ALLOC_YUV);
	    }
	    mpeg2_set_fbuf (mpeg2dec, (decoder->coding_type == B_TYPE));
	}
    } else {
	decoder->second_field = 1;
	mpeg2dec->picture++;	/* second field picture */
	*(mpeg2dec->picture) = mpeg2dec->new_picture;
	mpeg2dec->info.current_picture_2nd = mpeg2dec->picture;
	if (low_delay || decoder->coding_type == B_TYPE)
	    mpeg2dec->info.display_picture_2nd = mpeg2dec->picture;
    }

    info_user_data (mpeg2dec);
}

static int copyright_ext (mpeg2dec_t * mpeg2dec)
{
    return 0;
}

static int quant_matrix_ext (mpeg2dec_t * mpeg2dec)
{
    uint8_t * buffer = mpeg2dec->chunk_start;
    int i, j;

    for (i = 0; i < 4; i++)
    {
        if (buffer[0] & (8 >> i))
        {
	        for (j = 0; j < 64; j++)
            {
            mpeg2dec->new_quantizer_matrix[i][mpeg2_scan_norm[j]] =
                (buffer[j] << (i+5)) | (buffer[j+1] >> (3-i));
            }
            mpeg2dec->copy_matrix |= 1 << i;
            buffer += 64;
        }
    }
    return 0;
}

int mpeg2_header_extension (mpeg2dec_t * mpeg2dec)
{
    static int (* parser[]) (mpeg2dec_t *) = {
	0, sequence_ext, sequence_display_ext, quant_matrix_ext,
	copyright_ext, 0, 0, picture_display_ext, picture_coding_ext
    };
    int ext, ext_bit;

    ext = mpeg2dec->chunk_start[0] >> 4;
    ext_bit = 1 << ext;

    if (!(mpeg2dec->ext_state & ext_bit))
	return 0;	/* ignore illegal extensions */
    mpeg2dec->ext_state &= ~ext_bit;
    return parser[ext] (mpeg2dec);
}

int mpeg2_header_user_data (mpeg2dec_t * mpeg2dec)
{
    mpeg2dec->user_data_len += mpeg2dec->chunk_ptr - 1 - mpeg2dec->chunk_start;
    mpeg2dec->chunk_start = mpeg2dec->chunk_ptr - 1;
    
    return 0;
}

static void prescale (mpeg2dec_t * mpeg2dec, int idx)
{
    static int non_linear_scale [] = {
	 0,  1,  2,  3,  4,  5,   6,   7,
	 8, 10, 12, 14, 16, 18,  20,  22,
	24, 28, 32, 36, 40, 44,  48,  52,
	56, 64, 72, 80, 88, 96, 104, 112
    };
    int i, j, k;
    mpeg2_decoder_t * decoder = &(mpeg2dec->decoder);

    if (mpeg2dec->scaled[idx] != decoder->q_scale_type)
    {
        mpeg2dec->scaled[idx] = decoder->q_scale_type;
        for (i = 0; i < 32; i++)
        {
            k = decoder->q_scale_type ? non_linear_scale[i] : (i << 1);
	        for (j = 0; j < 64; j++)
                decoder->quantizer_prescale[idx][i][j] = k * mpeg2dec->quantizer_matrix[idx][j];
        }
    }
}

mpeg2_state_t mpeg2_header_slice_start (mpeg2dec_t * mpeg2dec)
{
    mpeg2_decoder_t * decoder = &(mpeg2dec->decoder);

    mpeg2dec->info.user_data = NULL;	mpeg2dec->info.user_data_len = 0;
    mpeg2dec->state = ((mpeg2dec->picture->nb_fields > 1 ||
			mpeg2dec->state == STATE_PICTURE_2ND) ?
		       STATE_SLICE : STATE_SLICE_1ST);

    if (mpeg2dec->decoder.coding_type != D_TYPE) {
	prescale (mpeg2dec, 0);
	if (decoder->chroma_quantizer[0] == decoder->quantizer_prescale[2])
	    prescale (mpeg2dec, 2);
	if (mpeg2dec->decoder.coding_type != I_TYPE) {
	    prescale (mpeg2dec, 1);
	    if (decoder->chroma_quantizer[1] == decoder->quantizer_prescale[3])
		prescale (mpeg2dec, 3);
	}
    }

    if (!(mpeg2dec->nb_decode_slices))
	mpeg2dec->picture->flags |= PIC_FLAG_SKIP;
    else if (mpeg2dec->convert_start) {
	mpeg2dec->convert_start (decoder->convert_id, mpeg2dec->fbuf[0],
				 mpeg2dec->picture, mpeg2dec->info.gop);

	if (mpeg2dec->decoder.coding_type == B_TYPE)
	    mpeg2_init_fbuf (&(mpeg2dec->decoder), mpeg2dec->yuv_buf[2],
			     mpeg2dec->yuv_buf[mpeg2dec->yuv_index ^ 1],
			     mpeg2dec->yuv_buf[mpeg2dec->yuv_index]);
	else {
	    mpeg2_init_fbuf (&(mpeg2dec->decoder),
			     mpeg2dec->yuv_buf[mpeg2dec->yuv_index ^ 1],
			     mpeg2dec->yuv_buf[mpeg2dec->yuv_index],
			     mpeg2dec->yuv_buf[mpeg2dec->yuv_index]);
	    if (mpeg2dec->state == STATE_SLICE)
		mpeg2dec->yuv_index ^= 1;
	}
    } else {
	int b_type;

	b_type = (mpeg2dec->decoder.coding_type == B_TYPE);
	mpeg2_init_fbuf (&(mpeg2dec->decoder), mpeg2dec->fbuf[0]->buf,
			 mpeg2dec->fbuf[b_type + 1]->buf,
			 mpeg2dec->fbuf[b_type]->buf);
    }
    mpeg2dec->action = NULL;
    return STATE_INTERNAL_NORETURN;
}

static mpeg2_state_t seek_sequence (mpeg2dec_t * mpeg2dec)
{
    mpeg2_reset_info (&(mpeg2dec->info));
    mpeg2dec->info.sequence = NULL;
    mpeg2dec->info.gop = NULL;
    mpeg2_header_state_init (mpeg2dec);
    mpeg2dec->action = mpeg2_seek_header;
    return mpeg2_seek_header (mpeg2dec);
}

mpeg2_state_t mpeg2_header_end (mpeg2dec_t * mpeg2dec)
{
    mpeg2_picture_t * picture;
    int b_type;

    b_type = (mpeg2dec->decoder.coding_type == B_TYPE);
    picture = mpeg2dec->pictures;
    if ((mpeg2dec->picture >= picture + 2) ^ b_type)
	picture = mpeg2dec->pictures + 2;

    mpeg2_reset_info (&(mpeg2dec->info));
    if (!(mpeg2dec->sequence.flags & SEQ_FLAG_LOW_DELAY)) {
	mpeg2dec->info.display_picture = picture;
	if (picture->nb_fields == 1)
	    mpeg2dec->info.display_picture_2nd = picture + 1;
	mpeg2dec->info.display_fbuf = mpeg2dec->fbuf[b_type];
	if (!mpeg2dec->convert)
	    mpeg2dec->info.discard_fbuf = mpeg2dec->fbuf[b_type + 1];
    } else if (!mpeg2dec->convert)
	mpeg2dec->info.discard_fbuf = mpeg2dec->fbuf[b_type];
    mpeg2dec->action = seek_sequence;
    return STATE_END;
}

mpeg2_mc_t mpeg2_mc;

void mpeg2_mc_init (uint32_t accel)
{
	mpeg2_mc = mpeg2_mc_c;
}

#define avg2(a,b) ((a+b+1)>>1)
#define avg4(a,b,c,d) ((a+b+c+d+2)>>2)

#define predict_o(i) (ref[i])
#define predict_x(i) (avg2 (ref[i], ref[i+1]))
#define predict_y(i) (avg2 (ref[i], (ref+stride)[i]))
#define predict_xy(i) (avg4 (ref[i], ref[i+1], \
			     (ref+stride)[i], (ref+stride)[i+1]))

#define put(predictor,i) dest[i] = predictor (i)
#define avg(predictor,i) dest[i] = avg2 (predictor (i), dest[i])

#define MC_FUNC(op,xy)							\
static void MC_##op##_##xy##_16_c (uint8_t * dest, const uint8_t * ref,	\
				   const int stride, int height)	\
{									\
    do {								\
	op (predict_##xy, 0);						\
	op (predict_##xy, 1);						\
	op (predict_##xy, 2);						\
	op (predict_##xy, 3);						\
	op (predict_##xy, 4);						\
	op (predict_##xy, 5);						\
	op (predict_##xy, 6);						\
	op (predict_##xy, 7);						\
	op (predict_##xy, 8);						\
	op (predict_##xy, 9);						\
	op (predict_##xy, 10);						\
	op (predict_##xy, 11);						\
	op (predict_##xy, 12);						\
	op (predict_##xy, 13);						\
	op (predict_##xy, 14);						\
	op (predict_##xy, 15);						\
	ref += stride;							\
	dest += stride;							\
    } while (--height);							\
}									\
static void MC_##op##_##xy##_8_c (uint8_t * dest, const uint8_t * ref,	\
				  const int stride, int height)		\
{									\
    do {								\
	op (predict_##xy, 0);						\
	op (predict_##xy, 1);						\
	op (predict_##xy, 2);						\
	op (predict_##xy, 3);						\
	op (predict_##xy, 4);						\
	op (predict_##xy, 5);						\
	op (predict_##xy, 6);						\
	op (predict_##xy, 7);						\
	ref += stride;							\
	dest += stride;							\
    } while (--height);							\
}

MC_FUNC (put,o)
MC_FUNC (avg,o)
MC_FUNC (put,x)
MC_FUNC (avg,x)
MC_FUNC (put,y)
MC_FUNC (avg,y)
MC_FUNC (put,xy)
MC_FUNC (avg,xy)

mpeg2_mc_t mpeg2_mc_c = {
    {MC_put_o_16_c, MC_put_x_16_c, MC_put_y_16_c, MC_put_xy_16_c, \
     MC_put_o_8_c,  MC_put_x_8_c,  MC_put_y_8_c,  MC_put_xy_8_c}, \
    {MC_avg_o_16_c, MC_avg_x_16_c, MC_avg_y_16_c, MC_avg_xy_16_c, \
     MC_avg_o_8_c,  MC_avg_x_8_c,  MC_avg_y_8_c,  MC_avg_xy_8_c} 
};

