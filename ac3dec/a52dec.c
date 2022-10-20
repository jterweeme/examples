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

#include "config.h"

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

#include "audio_out.h"
#include "mm_accel.h"
#include <sys/time.h>

#define BUFFER_SIZE 4096
static uint8_t buffer[BUFFER_SIZE];
static FILE * in_file;
static int disable_accel = 0;
static int disable_dynrng = 0;
static int disable_adjust = 0;
static sample_t gain = 1;
static ao_open_t * output_open = NULL;
static ao_instance_t * output;
static a52_state_t * state;

static void handle_args (int argc, char ** argv)
{
    int c;
    int i;
    char * s;

    ao_driver_t *drivers = ao_drivers ();
    while ((c = getopt (argc, argv, "s::t:crag:o:")) != -1)
        switch (c)
        {
        case 'o':
	        for (i = 0; drivers[i].name != NULL; i++)
                if (strcmp (drivers[i].name, optarg) == 0)
		            output_open = drivers[i].open;

            if (output_open == NULL)
                fprintf (stderr, "Invalid video driver: %s\n", optarg);
	        break;
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

    /* -o not specified, use a default driver */
    if (output_open == NULL)
        output_open = drivers[0].open;

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

    /*
     * sample_rate and flags are static because this routine could
     * exit between the a52_syncinfo() and the ao_setup(), and we want
     * to have the same values when we get back !
     */

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

        if (bufptr == bufpos)
        {
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
                int i;

                if (ao_setup (output, sample_rate, &flags, &level, &bias))
                    goto error;
                if (!disable_adjust)
                    flags |= A52_ADJUST_LEVEL;
                level *= gain;
                if (a52_frame (state, buf, &flags, &level, bias))
                    goto error;
                if (disable_dynrng)
                    a52_dynrng (state, NULL, NULL);
                for (i = 0; i < 6; i++)
                {
                    if (a52_block (state))
                        goto error;
                    if (ao_play (output, flags, a52_samples (state)))
                        goto error;
                }
                bufptr = buf;
                bufpos = buf + 7;
                continue;
error:
                fprintf (stderr, "error\n");
                bufptr = buf;
                bufpos = buf + 7;
            }
        }
    }
}

int main (int argc, char ** argv)
{
#ifdef HAVE_IO_H
    setmode (fileno (stdout), O_BINARY);
#endif
    fprintf (stderr, PACKAGE"-"VERSION
	     " - by Michel Lespinasse <walken@zoy.org> and Aaron Holtzman\n");

    handle_args (argc, argv);
    uint32_t accel = disable_accel ? 0 : MM_ACCEL_DJBFFT;
    output = ao_open (output_open);
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
    ao_close(output);
    return 0;
}
