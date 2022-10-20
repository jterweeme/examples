/*
 * a52dec.c
 * Copyright (C) 2000-2002 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of a52dec, a free ATSC A-52 stream decoder.
 * See http://liba52.sourceforge.net/ for updates.
 *
 * a52dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * a52dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define LIBAO_OSS 
#define PACKAGE "a52dec"
#define PACKAGE_NAME ""
#define PACKAGE_VERSION ""
#define STDC_HEADERS 1
#define VERSION "0.7.4"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <math.h>
#ifdef HAVE_IO_H
#include <fcntl.h>
#include <io.h>
#endif
#include <inttypes.h>

#ifndef LIBA52_DOUBLE
typedef float sample_t;
#else
typedef double sample_t;
#endif

typedef struct a52_state_s a52_state_t;

#define A52_STEREO 2
#define A52_DOLBY 10

#define A52_ADJUST_LEVEL 32

extern "C" {
a52_state_t * a52_init (uint32_t mm_accel);
sample_t * a52_samples (a52_state_t * state);
int a52_syncinfo (uint8_t * buf, int * flags,
          int * sample_rate, int * bit_rate);
int a52_frame (a52_state_t * state, uint8_t * buf, int * flags,
           sample_t * level, sample_t bias);
void a52_dynrng (a52_state_t * state,
         sample_t (* call) (sample_t, void *), void * data);
int a52_block (a52_state_t * state);
void a52_free (a52_state_t * state);
}



#ifdef WORDS_BIGENDIAN
#define s16_LE(s16,channels) s16_swap (s16, channels)
#define s16_BE(s16,channels) do {} while (0)
#else
#define s16_LE(s16,channels) do {} while (0)
#define s16_BE(s16,channels) s16_swap (s16, channels)
#endif


#define MM_ACCEL_DJBFFT     0x00000001

/* x86 accelerations */
#define MM_ACCEL_X86_MMX    0x80000000
#define MM_ACCEL_X86_3DNOW  0x40000000
#define MM_ACCEL_X86_MMXEXT 0x20000000

uint32_t mm_accel (void);

static inline int16_t convert (int32_t i)
{
    if (i > 0x43c07fff)
        return 32767;
    if (i < 0x43bf8000)
        return -32768;
    return i - 0x43c00000;
}

void float2s16_2 (float * _f, int16_t * s16)
{
    int32_t * f = (int32_t *) _f;

    for (int i = 0; i < 256; i++) {
        s16[2*i] = convert (f[i]);
        s16[2*i+1] = convert (f[i+256]);
    }
}

struct wav_instance_t {
    int sample_rate;
    int set_params;
    int flags;
    int size;
};

static uint8_t wav_header[] = {
    'R', 'I', 'F', 'F', 0xfc, 0xff, 0xff, 0xff, 'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ', 16, 0, 0, 0,
    1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 16, 0,
    'd', 'a', 't', 'a', 0xd8, 0xff, 0xff, 0xff
};

static int wav_setup (wav_instance_t * instance, int sample_rate, int * flags,
              sample_t * level, sample_t * bias)
{
    //wav_instance_t * instance = (wav_instance_t *) _instance;

    if ((instance->set_params == 0) && (instance->sample_rate != sample_rate))
        return 1;

    instance->sample_rate = sample_rate;
    *flags = instance->flags;
    *level = 1;
    *bias = 384;
    return 0;
}

static void store (uint8_t * buf, int value)
{
    buf[0] = value;
    buf[1] = value >> 8;
    buf[2] = value >> 16;
    buf[3] = value >> 24;
}

static int wav_play (wav_instance_t * instance, int flags, sample_t * _samples)
{
    int16_t int16_samples[256*2];
#ifdef LIBA52_DOUBLE
    float samples[256 * 2];

    for (int i = 0; i < 256 * 2; i++)
        samples[i] = _samples[i];
#else
    float * samples = _samples;
#endif

    if (instance->set_params) {
        instance->set_params = 0;
        store (wav_header + 24, instance->sample_rate);
        store (wav_header + 28, instance->sample_rate * 4);
        fwrite (wav_header, sizeof (wav_header), 1, stdout);
    }

    float2s16_2 (samples, int16_samples);
    s16_LE (int16_samples, 2);
    fwrite (int16_samples, 256 * sizeof (int16_t) * 2, 1, stdout);
    instance->size += 256 * sizeof (int16_t) * 2;
    return 0;
}


static void wav_close (wav_instance_t * _instance)
{
    wav_instance_t * instance = (wav_instance_t *) _instance;

    if (fseek (stdout, 0, SEEK_SET) < 0)
        return;

    store (wav_header + 4, instance->size + 36);
    store (wav_header + 40, instance->size);
    fwrite (wav_header, sizeof (wav_header), 1, stdout);
}

static wav_instance_t * wav_open (int flags)
{
    wav_instance_t * instance;
    instance = (wav_instance_t *)malloc (sizeof (wav_instance_t));
    if (instance == NULL)
        return NULL;
    instance->sample_rate = 0;
    instance->set_params = 1;
    instance->flags = flags;
    instance->size = 0;
    return instance;
}

#define BUFFER_SIZE 4096
static uint8_t buffer[BUFFER_SIZE];
static FILE * in_file;
static int disable_accel = 0;
static int disable_dynrng = 0;
static int disable_adjust = 0;
static sample_t gain = 1;
static wav_instance_t * output;
static a52_state_t * state;

static void handle_args (int argc, char ** argv)
{
    int c;

    //ao_driver_t *drivers = audio_out_drivers;
    while ((c = getopt (argc, argv, "s::t:crag:o:")) != -1)
    {
        switch (c)
        {
        case 'c':
            disable_accel = 1;
            break;
        case 'r':
            disable_dynrng = 1;
            break;
    	case 'a':
            disable_adjust = 1;
            break;
        }
    }

    if (optind < argc)
    {
	    in_file = fopen (argv[optind], "rb");
	    if (!in_file)
        {
            fprintf (stderr, "%s - could not open file %s\n", strerror (errno),
            argv[optind]);
            exit (1);
	    }
    }
    else
    {
        in_file = stdin;
    }
}

void a52_decode_data (uint8_t * start, uint8_t * end)
{
    static uint8_t buf[3840];
    static uint8_t * bufptr = buf;
    static uint8_t * bufpos = buf + 7;

    static int sample_rate;
    static int flags;
    int bit_rate;

    while (1)
    {
        int len = end - start;
        if (!len)
            break;

        if (len > bufpos - bufptr)
	        len = bufpos - bufptr;

        memcpy (bufptr, start, len);
        bufptr += len;
        start += len;

        if (bufptr != bufpos)
            continue;

        if (bufpos == buf + 7)
        {
            int length = a52_syncinfo (buf, &flags, &sample_rate, &bit_rate);

            if (!length)
            {
                fprintf (stderr, "skip\n");
                for (bufptr = buf; bufptr < buf + 6; bufptr++)
                    bufptr[0] = bufptr[1];
                continue;
            }
            bufpos = buf + length;
        }
        else
        {
            sample_t level, bias;

            if (wav_setup(output, sample_rate, &flags, &level, &bias))
                throw "error";
            if (!disable_adjust)
                flags |= A52_ADJUST_LEVEL;
            level *= gain;
            if (a52_frame (state, buf, &flags, &level, bias))
                throw "error";
            if (disable_dynrng)
                a52_dynrng (state, NULL, NULL);
            for (int i = 0; i < 6; i++)
            {
                if (a52_block (state))
                    throw "error";

                if (wav_play(output, flags, a52_samples(state)))
                    throw "error";
            }
            bufptr = buf;
            bufpos = buf + 7;
        }
    }
}

int main (int argc, char ** argv)
{
#ifdef HAVE_IO_H
    setmode (fileno (stdout), O_BINARY);
#endif
    fprintf (stderr, "a52dec"
	     " - by Michel Lespinasse <walken@zoy.org> and Aaron Holtzman\n");

    handle_args (argc, argv);
    uint32_t accel = disable_accel ? 0 : MM_ACCEL_DJBFFT;
    output = wav_open(A52_STEREO);
    if (output == NULL) {
        fprintf (stderr, "Can not open output\n");
        return 1;
    }

    state = a52_init (accel);
    if (state == NULL) {
        fprintf (stderr, "A52 init failed\n");
        return 1;
    }

    int size;
    do {
        size = fread(buffer, 1, BUFFER_SIZE, in_file);
        a52_decode_data(buffer, buffer + size);
    } while (size == BUFFER_SIZE);
    a52_free(state);
    wav_close(output);
    return 0;
}
