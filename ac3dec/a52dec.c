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
#include <signal.h>
#ifdef HAVE_IO_H
#include <fcntl.h>
#include <io.h>
#endif
#include <inttypes.h>

#include <a52dec/a52.h>
#include "audio_out.h"
#include "mm_accel.h"
#include "gettimeofday.h"

#define BUFFER_SIZE 4096
static uint8_t buffer[BUFFER_SIZE];
static FILE * in_file;
static int demux_track = 0;
static int demux_pid = 0;
static int disable_accel = 0;
static int disable_dynrng = 0;
static int disable_adjust = 0;
static sample_t gain = 1;
static ao_open_t * output_open = NULL;
static ao_instance_t * output;
static a52_state_t * state;

#ifdef HAVE_GETTIMEOFDAY

static void print_fps (int final);

static RETSIGTYPE signal_handler (int sig)
{
    print_fps (1);
    signal (sig, SIG_DFL);
    raise (sig);
}

static void print_fps (int final)
{
    static uint32_t frame_counter = 0;
    static struct timeval tv_beg, tv_start;
    static int total_elapsed;
    static int last_count = 0;
    struct timeval tv_end;
    float fps, tfps;
    int frames, elapsed;

    gettimeofday (&tv_end, NULL);

    if (!frame_counter) {
	tv_start = tv_beg = tv_end;
	signal (SIGINT, signal_handler);
    }

    elapsed = (tv_end.tv_sec - tv_beg.tv_sec) * 100 +
	(tv_end.tv_usec - tv_beg.tv_usec) / 10000;
    total_elapsed = (tv_end.tv_sec - tv_start.tv_sec) * 100 +
	(tv_end.tv_usec - tv_start.tv_usec) / 10000;

    if (final) {
	if (total_elapsed)
	    tfps = frame_counter * 100.0 / total_elapsed;
	else
	    tfps = 0;

	fprintf (stderr,"\n%d frames decoded in %.2f seconds (%.2f fps)\n",
		 frame_counter, total_elapsed / 100.0, tfps);

	return;
    }

    frame_counter++;

    /* only display every 0.50 seconds */
    if (elapsed < 50)	
        return;

    tv_beg = tv_end;
    frames = frame_counter - last_count;

    fps = frames * 100.0 / elapsed;
    tfps = frame_counter * 100.0 / total_elapsed;

    fprintf (stderr, "%d frames in %.2f sec (%.2f fps), "
	     "%d last %.2f sec (%.2f fps)\033[K\r", frame_counter,
	     total_elapsed / 100.0, tfps, frames, elapsed / 100.0, fps);

    last_count = frame_counter;
}

#else /* !HAVE_GETTIMEOFDAY */

static void print_fps (int final)
{
}

#endif

static void print_usage (char ** argv)
{
    int i;
    ao_driver_t * drivers;

    fprintf (stderr, "usage: "
	     "%s [-o <mode>] [-s [<track>]] [-t <pid>] [-c] [-r] [-a] \\\n"
	     "\t\t[-g <gain>] <file>\n"
	     "\t-s\tuse program stream demultiplexer, track 0-7 or 0x80-0x87\n"
	     "\t-t\tuse transport stream demultiplexer, pid 0x10-0x1ffe\n"
	     "\t-c\tuse c implementation, disables all accelerations\n"
	     "\t-r\tdisable dynamic range compression\n"
	     "\t-a\tdisable level adjustment based on output mode\n"
	     "\t-g\tadd specified gain in decibels, -96.0 to +96.0\n"
	     "\t-o\taudio output mode\n", argv[0]);

    drivers = ao_drivers ();
    for (i = 0; drivers[i].name; i++)
	fprintf (stderr, "\t\t\t%s\n", drivers[i].name);

    exit (1);
}

static void handle_args (int argc, char ** argv)
{
    int c;
    ao_driver_t * drivers;
    int i;
    char * s;

    drivers = ao_drivers ();
    while ((c = getopt (argc, argv, "s::t:crag:o:")) != -1)
	switch (c) {
	case 'o':
	    for (i = 0; drivers[i].name != NULL; i++)
		if (strcmp (drivers[i].name, optarg) == 0)
		    output_open = drivers[i].open;
	    if (output_open == NULL) {
		fprintf (stderr, "Invalid video driver: %s\n", optarg);
		print_usage (argv);
	    }
	    break;

	case 's':
	    demux_track = 0x80;
	    if (optarg != NULL) {
		demux_track = strtol (optarg, &s, 16);
		if (demux_track < 0x80)
		    demux_track += 0x80;
		if ((demux_track < 0x80) || (demux_track > 0x87) || (*s)) {
		    fprintf (stderr, "Invalid track number: %s\n", optarg);
		    print_usage (argv);
		}
	    }
	    break;

	case 't':
	    demux_pid = strtol (optarg, &s, 16);
	    if ((demux_pid < 0x10) || (demux_pid > 0x1ffe) || (*s)) {
		fprintf (stderr, "Invalid pid: %s\n", optarg);
		print_usage (argv);
	    }
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

	case 'g':
	    gain = strtod (optarg, &s);
	    if ((gain < -96) || (gain > 96) || (*s)) {
		fprintf (stderr, "Invalid gain: %s\n", optarg);
		print_usage (argv);
	    }
	    gain = pow (2, gain / 6);
	    break;

	default:
	    print_usage (argv);
	}

    /* -o not specified, use a default driver */
    if (output_open == NULL)
	output_open = drivers[0].open;

    if (optind < argc) {
	in_file = fopen (argv[optind], "rb");
	if (!in_file) {
	    fprintf (stderr, "%s - could not open file %s\n", strerror (errno),
		     argv[optind]);
	    exit (1);
	}
    } else
	in_file = stdin;
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
    int len;

    while (1) {
	len = end - start;
	if (!len)
	    break;
	if (len > bufpos - bufptr)
	    len = bufpos - bufptr;
	memcpy (bufptr, start, len);
	bufptr += len;
	start += len;
	if (bufptr == bufpos) {
	    if (bufpos == buf + 7) {
		int length;

		length = a52_syncinfo (buf, &flags, &sample_rate, &bit_rate);
		if (!length) {
		    fprintf (stderr, "skip\n");
		    for (bufptr = buf; bufptr < buf + 6; bufptr++)
			bufptr[0] = bufptr[1];
		    continue;
		}
		bufpos = buf + length;
	    } else {
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
		for (i = 0; i < 6; i++) {
		    if (a52_block (state))
			goto error;
		    if (ao_play (output, flags, a52_samples (state)))
			goto error;
		}
		bufptr = buf;
		bufpos = buf + 7;
		print_fps (0);
		continue;
	    error:
		fprintf (stderr, "error\n");
		bufptr = buf;
		bufpos = buf + 7;
	    }
	}
    }
}

static void es_loop (void)
{
    int size;
		
    do {
	size = fread (buffer, 1, BUFFER_SIZE, in_file);
	a52_decode_data (buffer, buffer + size);
    } while (size == BUFFER_SIZE);
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

    es_loop();
    a52_free(state);
    print_fps(1);
    ao_close(output);
    return 0;
}
