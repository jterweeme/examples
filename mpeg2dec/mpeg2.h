/*
 * mpeg2.h
 * Copyright (C) 2000-2004 Michel Lespinasse <walken@zoy.org>
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

#ifndef LIBMPEG2_MPEG2_H
#define LIBMPEG2_MPEG2_H

#include <stddef.h>

#define MPEG2_VERSION(a,b,c) (((a)<<16)|((b)<<8)|(c))
#define MPEG2_RELEASE MPEG2_VERSION (0, 5, 1)	/* 0.5.1 */

typedef struct mpeg2_sequence_s {
    unsigned int width, height;
    unsigned int chroma_width, chroma_height;
    unsigned int byte_rate;
    unsigned int vbv_buffer_size;
    uint32_t flags;

    unsigned int picture_width, picture_height;
    unsigned int display_width, display_height;
    unsigned int pixel_width, pixel_height;
    unsigned int frame_period;

    uint8_t profile_level_id;
    uint8_t colour_primaries;
    uint8_t transfer_characteristics;
    uint8_t matrix_coefficients;
} mpeg2_sequence_t;

#define GOP_FLAG_DROP_FRAME 1
#define GOP_FLAG_BROKEN_LINK 2
#define GOP_FLAG_CLOSED_GOP 4

struct mpeg2_gop_t {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t pictures;
    uint32_t flags;
};

typedef struct mpeg2_picture_s {
    unsigned int temporal_reference;
    unsigned int nb_fields;
    uint32_t tag, tag2;
    uint32_t flags;
    struct {
	int x, y;
    } display_offset[3];
} mpeg2_picture_t;

typedef struct mpeg2_fbuf_s {
    uint8_t * buf[3];
    void * id;
} mpeg2_fbuf_t;

typedef struct mpeg2_info_s {
    const mpeg2_sequence_t * sequence;
    const mpeg2_gop_t * gop;
    const mpeg2_picture_t * current_picture;
    const mpeg2_picture_t * current_picture_2nd;
    const mpeg2_fbuf_t * current_fbuf;
    const mpeg2_picture_t * display_picture;
    const mpeg2_picture_t * display_picture_2nd;
    const mpeg2_fbuf_t * display_fbuf;
    const mpeg2_fbuf_t * discard_fbuf;
    const uint8_t * user_data;
    unsigned int user_data_len;
} mpeg2_info_t;

typedef struct mpeg2dec_s mpeg2dec_t;

typedef enum {
    STATE_BUFFER = 0,
    STATE_SEQUENCE = 1,
    STATE_SEQUENCE_REPEATED = 2,
    STATE_GOP = 3,
    STATE_PICTURE = 4,
    STATE_SLICE_1ST = 5,
    STATE_PICTURE_2ND = 6,
    STATE_SLICE = 7,
    STATE_END = 8,
    STATE_INVALID = 9,
    STATE_INVALID_END = 10,
    STATE_SEQUENCE_MODIFIED = 11
} mpeg2_state_t;

typedef struct mpeg2_convert_init_s {
    unsigned int id_size;
    unsigned int buf_size[3];
    void (* start) (void * id, const mpeg2_fbuf_t * fbuf,
		    const mpeg2_picture_t * picture, const mpeg2_gop_t * gop);
    void (* copy) (void * id, uint8_t * const * src, unsigned int v_offset);
} mpeg2_convert_init_t;
typedef enum {
    MPEG2_CONVERT_SET = 0,
    MPEG2_CONVERT_STRIDE = 1,
    MPEG2_CONVERT_START = 2
} mpeg2_convert_stage_t;
typedef int mpeg2_convert_t (int stage, void * id,
			     const mpeg2_sequence_t * sequence, int stride,
			     uint32_t accel, void * arg,
			     mpeg2_convert_init_t * result);
int mpeg2_convert (mpeg2dec_t * mpeg2dec, mpeg2_convert_t convert, void * arg);
int mpeg2_stride (mpeg2dec_t * mpeg2dec, int stride);
void mpeg2_set_buf (mpeg2dec_t * mpeg2dec, uint8_t * buf[3], void * id);
void mpeg2_custom_fbuf (mpeg2dec_t * mpeg2dec, int custom_fbuf);

#define MPEG2_ACCEL_DETECT 0x80000000

uint32_t mpeg2_accel (uint32_t accel);
mpeg2dec_t * mpeg2_init (void);
const mpeg2_info_t * mpeg2_info (mpeg2dec_t * mpeg2dec);
void mpeg2_close (mpeg2dec_t * mpeg2dec);

void mpeg2_buffer (mpeg2dec_t * mpeg2dec, uint8_t * start, uint8_t * end);
int mpeg2_getpos (mpeg2dec_t * mpeg2dec);
mpeg2_state_t mpeg2_parse (mpeg2dec_t * mpeg2dec);

void mpeg2_reset (mpeg2dec_t * mpeg2dec, int full_reset);
void mpeg2_skip (mpeg2dec_t * mpeg2dec, int skip);
void mpeg2_slice_region (mpeg2dec_t * mpeg2dec, int start, int end);

void mpeg2_tag_picture (mpeg2dec_t * mpeg2dec, uint32_t tag, uint32_t tag2);


int mpeg2_guess_aspect (const mpeg2_sequence_t * sequence,
			unsigned int * pixel_width,
			unsigned int * pixel_height);

typedef enum {
    MPEG2_ALLOC_MPEG2DEC = 0,
    MPEG2_ALLOC_CHUNK = 1,
    MPEG2_ALLOC_YUV = 2,
    MPEG2_ALLOC_CONVERT_ID = 3,
    MPEG2_ALLOC_CONVERTED = 4
} mpeg2_alloc_t;

void * mpeg2_malloc (unsigned size, mpeg2_alloc_t reason);
void mpeg2_free (void * buf);
void mpeg2_malloc_hooks (void * malloc (unsigned, mpeg2_alloc_t), int free (void *));

/* use gcc attribs to align critical data structures */
#ifdef ATTRIBUTE_ALIGNED_MAX
#define ATTR_ALIGN(align) __attribute__ ((__aligned__ ((ATTRIBUTE_ALIGNED_MAX < align) ? ATTRIBUTE_ALIGNED_MAX : align)))
#else
#define ATTR_ALIGN(align)
#endif
    
#ifdef HAVE_BUILTIN_EXPECT
#define likely(x) __builtin_expect ((x) != 0, 1)
#define unlikely(x) __builtin_expect ((x) != 0, 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif
    
#define STATE_INTERNAL_NORETURN ((mpeg2_state_t)-1)

typedef void mpeg2_mc_fct (uint8_t *, const uint8_t *, int, int);

typedef struct {
    uint8_t * ref[2][3];
    uint8_t ** ref2[2];
    int pmv[2][2];
    int f_code[2];
} motion_t;
    
struct mpeg2_decoder_t;

typedef void motion_parser_t (mpeg2_decoder_t * decoder,
                  motion_t * motion,
                  mpeg2_mc_fct * const * table);

class mpeg2_decoder_t
{
public:
    /* first, state that carries information from one macroblock to the */
    /* next inside a slice, and is never used outside of mpeg2_slice() */
    
    /* bit parsing stuff */
    uint32_t bitstream_buf;     /* current 32 bit working set */
    int bitstream_bits;         /* used bits in working set */
    const uint8_t * bitstream_ptr;  /* buffer with stream data */

    uint8_t * dest[3];

    int offset;
    int stride;
    int uv_stride;
    int slice_stride;
    int slice_uv_stride;
    int stride_frame;
    unsigned limit_x;
    unsigned limit_y_16;
    unsigned limit_y_8;
    unsigned limit_y;

    /* Motion vectors */
    /* The f_ and b_ correspond to the forward and backward motion */
    /* predictors */
    motion_t b_motion;
    motion_t f_motion;
    motion_parser_t * motion_parser[5];

    /* predictor for DC coefficients in intra blocks */
    int16_t dc_dct_pred[3];

    /* DCT coefficients */
    int16_t DCTblock[64] ATTR_ALIGN(64);

    uint8_t * picture_dest[3];
    void (* convert) (void * convert_id, uint8_t * const * src,
              unsigned int v_offset);
    void * convert_id;

    int dmv_offset;
    unsigned int v_offset;
    /* now non-slice-specific information */

    /* sequence header stuff */
    uint16_t * quantizer_matrix[4];
    uint16_t (* chroma_quantizer[2])[64];
    uint16_t quantizer_prescale[4][32][64];

    /* The width and height of the picture snapped to macroblock units */
    int width;
    int height;
    int vertical_position_extension;
    int chroma_format;

    /* picture header stuff */

    /* what type of picture this is (I, P, B, D) */
    int coding_type;

    /* picture coding extension stuff */

    /* quantization factor for intra dc coefficients */
    int intra_dc_precision;
    /* top/bottom/both fields */
    int picture_structure;
    /* bool to indicate all predictions are frame based */
    int frame_pred_frame_dct;
    /* bool to indicate whether intra blocks have motion vectors */
    /* (for concealment) */
    int concealment_motion_vectors;
    /* bool to use different vlc tables */
    int intra_vlc_format;
    /* used for DMV MC */
    int top_field_first;

    /* stuff derived from bitstream */

    /* pointer to the zigzag scan we're supposed to be using */
    const uint8_t * scan;

    int second_field;

    int mpeg1;
    /* XXX: stuff due to xine shit */
    int8_t q_scale_type;
    uint32_t ubits(size_t n) { return bitstream_buf >> 32 - n; }
    void dumpbits(size_t n) { bitstream_buf <<= n, bitstream_bits += n; }
};



void mpeg2_init_fbuf (mpeg2_decoder_t * decoder, uint8_t * current_fbuf[3],
		      uint8_t * forward_fbuf[3], uint8_t * backward_fbuf[3]);

#endif



