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

#include <math.h>
#include <algorithm>
#include "kjmp2.h"

static int initialized = 0;
static int bit_window;
static int bits_in_window;
static const unsigned char *frame_pos;

//#define show_bits(bit_count) (bit_window >> (24 - (bit_count)))

static int show_bits(int bit_count)
{
    return bit_window >> (24 - bit_count);
}

static int get_bits(int bit_count)
{
    int result = show_bits(bit_count);
    bit_window = (bit_window << bit_count) & 0xFFFFFF;
    bits_in_window -= bit_count;

    while (bits_in_window < 16)
    {
        bit_window |= (*frame_pos++) << (16 - bits_in_window);
        bits_in_window += 8;
    }

    return result;
}

static int N[64][32];  // N[i][j] as 8-bit fixed-point

// kjmp2_init: This function must be called once to initialize each kjmp2
// decoder instance.
void kjmp2_init(kjmp2_context_t *mp2)
{
    int i, j;
    // check if global initialization is required
    if (!initialized) {
        int *nptr = &N[0][0];
        // compute N[i][j]
        for (i = 0;  i < 64;  ++i)
            for (j = 0;  j < 32;  ++j)
                *nptr++ = (int) (256.0 * cos(((16 + i) * ((j << 1) + 1)) * 0.0490873852123405));
        initialized = 1;
    }

    // perform local initialization: clean the context and put the magic in it
    for (i = 0;  i < 2;  ++i)
        for (j = 1023;  j >= 0;  --j)
            mp2->V[i][j] = 0;
    mp2->Voffs = 0;
    mp2->id = KJMP2_MAGIC;
}

// kjmp2_get_sample_rate: Returns the sample rate of a MP2 stream.
// frame: Points to at least the first three bytes of a frame from the
//        stream.
// return value: The sample rate of the stream in Hz, or zero if the stream
//               isn't valid.
int kjmp2_get_sample_rate(const uint8_t *frame)
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

// DECODE HELPER FUNCTIONS
static const struct quantizer_spec *read_allocation(int sb, int b2_table)
{
    int table_idx = quant_lut_step3[b2_table][sb];
    table_idx = quant_lut_step4[table_idx & 15][get_bits(table_idx >> 4)];
    return table_idx ? (&quantizer_table[table_idx - 1]) : 0;
}


static void read_samples(const struct quantizer_spec *q, int scalefactor, int *sample)
{
    int idx, adj, scale;
    int val;

    if (!q)
    {
        // no bits allocated for this subband
        sample[0] = sample[1] = sample[2] = 0;
        return;
    }

    // resolve scalefactor
    if (scalefactor == 63) {
        scalefactor = 0;
    } else {
        adj = scalefactor / 3;
        scalefactor = (scf_base[scalefactor % 3] + ((1 << adj) >> 1)) >> adj;
    }

    // decode samples
    adj = q->nlevels;

    if (q->grouping)
    {
        // decode grouped samples
        val = get_bits(q->cw_bits);
        sample[0] = val % adj;
        val /= adj;
        sample[1] = val % adj;
        sample[2] = val / adj;
    } else {
        // decode direct samples
        for(idx = 0;  idx < 3;  ++idx)
            sample[idx] = get_bits(q->cw_bits);
    }

    // postmultiply samples
    scale = 65536 / (adj + 1);
    adj = ((adj + 1) >> 1) - 1;

    for (idx = 0;  idx < 3;  ++idx)
    {
        // step 1: renormalization to [-1..1]
        val = (adj - sample[idx]) * scale;
        // step 2: apply scalefactor
        sample[idx] = ( val * (scalefactor >> 12)                  // upper part
                    + ((val * (scalefactor & 4095) + 2048) >> 12)) // lower part
                    >> 12;  // scale adjust
    }
}

static const quantizer_spec *allocation[2][32];
static int scfsi[2][32];
static int scalefactor[2][32][3];
static int sample[2][32][3];
static int U[512];

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
unsigned long kjmp2_decode_frame(
    kjmp2_context_t *mp2,
    const uint8_t *frame,
    int16_t *pcm)
{
    unsigned long frame_size;
    int table_idx;

    // general sanity check
    if (!initialized || !mp2 || (mp2->id != KJMP2_MAGIC) || !frame)
        return 0;

    // check for valid header: syncword OK, MPEG-Audio Layer 2
    if ((frame[0] != 0xFF) || ((frame[1] & 0xF6) != 0xF4))
        return 0;

    // set up the bitstream reader
    bit_window = frame[2] << 16;
    bits_in_window = 8;
    frame_pos = &frame[3];

    // read the rest of the header
    unsigned bit_rate_index_minus1 = get_bits(4) - 1;

    if (bit_rate_index_minus1 > 13)
        return 0;  // invalid bit rate or 'free format'

    unsigned sampling_frequency = get_bits(2);

    if (sampling_frequency == 3)
        return 0;

    if ((frame[1] & 0x08) == 0) {  // MPEG-2
        sampling_frequency += 4;
        bit_rate_index_minus1 += 14;
    }
    unsigned padding_bit = get_bits(1);
    get_bits(1);  // discard private_bit
    unsigned mode = get_bits(2);
    int bound, sblimit;

    // parse the mode_extension, set up the stereo bound
    if (mode == JOINT_STEREO)
    {
        bound = (get_bits(2) + 1) << 2;
    }
    else
    {
        get_bits(2);
        bound = (mode == MONO) ? 0 : 32;
    }

    // discard the last 4 bits of the header and the CRC value, if present
    get_bits(4);

    if ((frame[1] & 1) == 0)
        get_bits(16);

    // compute the frame size
    frame_size = (144000 * bitrates[bit_rate_index_minus1]
               / sample_rates[sampling_frequency]) + padding_bit;

    if (!pcm)
        return frame_size;  // no decoding

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
            allocation[ch][sb] = read_allocation(sb, table_idx);

    for (int sb = bound;  sb < sblimit;  ++sb)
        allocation[0][sb] = allocation[1][sb] = read_allocation(sb, table_idx);

    // read scale factor selector information
    int nch = (mode == MONO) ? 1 : 2;

    for (int sb = 0;  sb < sblimit;  ++sb)
    {
        for (int ch = 0;  ch < nch;  ++ch)
            if (allocation[ch][sb])
                scfsi[ch][sb] = get_bits(2);

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
                    scalefactor[ch][sb][0] = get_bits(6);
                    scalefactor[ch][sb][1] = get_bits(6);
                    scalefactor[ch][sb][2] = get_bits(6);
                    break;
                case 1:
                    scalefactor[ch][sb][0] = scalefactor[ch][sb][1] = get_bits(6);
                    scalefactor[ch][sb][2] = get_bits(6);
                    break;
                case 2:
                    scalefactor[ch][sb][0] = scalefactor[ch][sb][1] = scalefactor[ch][sb][2] = get_bits(6);
                    break;
                case 3:
                    scalefactor[ch][sb][0] = get_bits(6);
                    scalefactor[ch][sb][1] = scalefactor[ch][sb][2] = get_bits(6);
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
                    read_samples(allocation[ch][sb], scalefactor[ch][sb][part], &sample[ch][sb][0]);

            for (int sb = bound;  sb < sblimit;  ++sb)
            {
                read_samples(allocation[0][sb], scalefactor[0][sb][part], &sample[0][sb][0]);

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
                mp2->Voffs = table_idx = (mp2->Voffs - 64) & 1023;

                for (int ch = 0;  ch < 2;  ++ch)
                {
                    // matrixing
                    for (int i = 0;  i < 64;  ++i)
                    {
                        int sum = 0;

                        for (int j = 0;  j < 32;  ++j)
                            sum += N[i][j] * sample[ch][j][idx];  // 8b*15b=23b

                        // intermediate value is 28 bit (23 + 5), clamp to 14b
                        mp2->V[ch][table_idx + i] = (sum + 8192) >> 14;
                    }

                    // construction of U
                    for (int i = 0;  i < 8;  ++i)
                    {
                        for (int j = 0;  j < 32;  ++j)
                        {
                            U[(i << 6) + j]      = mp2->V[ch][(table_idx + (i << 7) + j     ) & 1023];
                            U[(i << 6) + j + 32] = mp2->V[ch][(table_idx + (i << 7) + j + 96) & 1023];
                        }
                    }

                    // apply window
                    for (int i = 0;  i < 512;  ++i)
                        U[i] = (U[i] * D[i] + 32) >> 6;

                    // output samples
                    for (int j = 0; j < 32; ++j)
                    {
                        int sum = 0;

                        for (int i = 0;  i < 16;  ++i)
                            sum -= U[(i << 5) + j];

                        sum = (sum + 8) >> 4;
                        sum = std::max(sum, -32768);
                        sum = std::min(sum, 32767);
                        pcm[(idx << 6) | (j << 1) | ch] = (signed short) sum;
                    }
                } // end of synthesis channel loop
            } // end of synthesis sub-block loop

            // adjust PCM output pointer: decoded 3 * 32 = 96 stereo samples
            pcm += 192;

        } // decoding of the granule finished
    }
    return frame_size;
}
