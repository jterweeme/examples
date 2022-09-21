/*
PL_MPEG - MPEG1 Video decoder, MP2 Audio decoder, MPEG-PS demuxer

Dominic Szablewski - https://phoboslab.org


-- LICENSE: The MIT License(MIT)

Copyright(c) 2019 Dominic Szablewski

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files(the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions :
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.




-- Synopsis

// Define `PL_MPEG_IMPLEMENTATION` in *one* C/C++ file before including this
// library to create the implementation.

#define PL_MPEG_IMPLEMENTATION
#include "plmpeg.h"

// This function gets called for each decoded video frame
void my_video_callback(plm_t *plm, plm_frame_t *frame, void *user) {
    // Do something with frame->y.data, frame->cr.data, frame->cb.data
}

// This function gets called for each decoded audio frame
void my_audio_callback(plm_t *plm, plm_samples_t *frame, void *user) {
    // Do something with samples->interleaved
}

// Load a .mpg (MPEG Program Stream) file
plm_t *plm = plm_create_with_filename("some-file.mpg");

// Install the video & audio decode callbacks
plm_set_video_decode_callback(plm, my_video_callback, my_data);
plm_set_audio_decode_callback(plm, my_audio_callback, my_data);


// Decode
do {
    plm_decode(plm, time_since_last_call);
} while (!plm_has_ended(plm));

// All done
plm_destroy(plm);



-- Documentation

This library provides several interfaces to load, demux and decode MPEG video
and audio data. A high-level API combines the demuxer, video & audio decoders
in an easy to use wrapper.

Lower-level APIs for accessing the demuxer, video decoder and audio decoder, 
as well as providing different data sources are also available.

Interfaces are written in an object oriented style, meaning you create object 
instances via various different constructor functions (plm_*create()),
do some work on them and later dispose them via plm_*destroy().

plm_* ......... the high-level interface, combining demuxer and decoders
plm_buffer_* .. the data source used by all interfaces
plm_demux_* ... the MPEG-PS demuxer
plm_video_* ... the MPEG1 Video ("mpeg1") decoder
plm_audio_* ... the MPEG1 Audio Layer II ("mp2") decoder


With the high-level interface you have two options to decode video & audio:

 1. Use plm_decode() and just hand over the delta time since the last call.
    It will decode everything needed and call your callbacks (specified through
    plm_set_{video|audio}_decode_callback()) any number of times.

 2. Use plm_decode_video() and plm_decode_audio() to decode exactly one
    frame of video or audio data at a time. How you handle the synchronization 
    of both streams is up to you.

If you only want to decode video *or* audio through these functions, you should
disable the other stream (plm_set_{video|audio}_enabled(FALSE))

Video data is decoded into a struct with all 3 planes (Y, Cr, Cb) stored in
separate buffers. You can either convert this to RGB on the CPU (slow) via the
plm_frame_to_rgb() function or do it on the GPU with the following matrix:

mat4 bt601 = mat4(
    1.16438,  0.00000,  1.59603, -0.87079,
    1.16438, -0.39176, -0.81297,  0.52959,
    1.16438,  2.01723,  0.00000, -1.08139,
    0, 0, 0, 1
);
gl_FragColor = vec4(y, cb, cr, 1.0) * bt601;

Audio data is decoded into a struct with either one single float array with the
samples for the left and right channel interleaved, or if the 
PLM_AUDIO_SEPARATE_CHANNELS is defined *before* including this library, into
two separate float arrays - one for each channel.


Data can be supplied to the high level interface, the demuxer and the decoders
in three different ways:

 1. Using plm_create_from_filename() or with a file handle with 
    plm_create_from_file().

 2. Using plm_create_with_memory() and supplying a pointer to memory that
    contains the whole file.

 3. Using plm_create_with_buffer(), supplying your own plm_buffer_t instance and
    periodically writing to this buffer.

When using your own plm_buffer_t instance, you can fill this buffer using 
plm_buffer_write(). You can either monitor plm_buffer_get_remaining() and push 
data when appropriate, or install a callback on the buffer with 
plm_buffer_set_load_callback() that gets called whenever the buffer needs more 
data.

A buffer created with plm_buffer_create_with_capacity() is treated as a ring
buffer, meaning that data that has already been read, will be discarded. In
contrast, a buffer created with plm_buffer_create_for_appending() will keep all
data written to it in memory. This enables seeking in the already loaded data.


There should be no need to use the lower level plm_demux_*, plm_video_* and 
plm_audio_* functions, if all you want to do is read/decode an MPEG-PS file.
However, if you get raw mpeg1video data or raw mp2 audio data from a different
source, these functions can be used to decode the raw data directly. Similarly, 
if you only want to analyze an MPEG-PS file or extract raw video or audio
packets from it, you can use the plm_demux_* functions.


This library uses malloc(), realloc() and free() to manage memory. Typically 
all allocation happens up-front when creating the interface. However, the
default buffer size may be too small for certain inputs. In these cases plmpeg
will realloc() the buffer with a larger size whenever needed. You can configure
the default buffer size by defining PLM_BUFFER_DEFAULT_SIZE *before* 
including this library.


See below for detailed the API documentation.

*/


#ifndef PL_MPEG_H
#define PL_MPEG_H

#include <stdint.h>
#include <stdio.h>

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif




// Demuxed MPEG PS packet
// The type maps directly to the various MPEG-PES start codes. PTS is the
// presentation time stamp of the packet in seconds. Note that not all packets
// have a PTS value, indicated by PLM_PACKET_INVALID_TS.

#define PLM_PACKET_INVALID_TS -1

typedef struct {
    int type;
    double pts;
    size_t length;
    uint8_t *data;
} plm_packet_t;


// Decoded Video Plane 
// The byte length of the data is width * height. Note that different planes
// have different sizes: the Luma plane (Y) is double the size of each of 
// the two Chroma planes (Cr, Cb) - i.e. 4 times the byte length.
// Also note that the size of the plane does *not* denote the size of the 
// displayed frame. The sizes of planes are always rounded up to the nearest
// macroblock (16px).

typedef struct {
    unsigned int width;
    unsigned int height;
    uint8_t *data;
} plm_plane_t;


// Decoded Video Frame
// width and height denote the desired display size of the frame. This may be
// different from the internal size of the 3 planes.

typedef struct {
    double time;
    unsigned int width;
    unsigned int height;
    plm_plane_t y;
    plm_plane_t cr;
    plm_plane_t cb;
} plm_frame_t;

typedef struct plm_t plm_t;


// Callback function type for decoded video frames used by the high-level
// plm_* interface
typedef void(*plm_video_decode_callback)
    (plm_t *self, plm_frame_t *frame, void *user);


// Decoded Audio Samples
// Samples are stored as normalized (-1, 1) float either interleaved, or if
// PLM_AUDIO_SEPARATE_CHANNELS is defined, in two separate arrays.
// The `count` is always PLM_AUDIO_SAMPLES_PER_FRAME and just there for
// convenience.

#define PLM_AUDIO_SAMPLES_PER_FRAME 1152

typedef struct {
    double time;
    unsigned int count;
    #ifdef PLM_AUDIO_SEPARATE_CHANNELS
        float left[PLM_AUDIO_SAMPLES_PER_FRAME];
        float right[PLM_AUDIO_SAMPLES_PER_FRAME];
    #else
        float interleaved[PLM_AUDIO_SAMPLES_PER_FRAME * 2];
    #endif
} plm_samples_t;


// Callback function type for decoded audio samples used by the high-level
// plm_* interface

typedef void(*plm_audio_decode_callback)
    (plm_t *self, plm_samples_t *samples, void *user);

typedef struct plm_buffer_t plm_buffer_t;
typedef struct plm_demux_t plm_demux_t;
typedef struct plm_video_t plm_video_t;
typedef struct plm_audio_t plm_audio_t;

// Callback function for plm_buffer when it needs more data
typedef void(*plm_buffer_load_callback)(plm_buffer_t *self, void *user);

plm_t *plm_create_with_filename(const char *filename);
plm_t *plm_create_with_file(FILE *fh, int close_when_done);
plm_t *plm_create_with_memory(uint8_t *bytes, size_t length, int free_when_done);
plm_t *plm_create_with_buffer(plm_buffer_t *buffer, int destroy_when_done);
void plm_destroy(plm_t *self);
int plm_has_headers(plm_t *self);
int plm_get_video_enabled(plm_t *self);
void plm_set_video_enabled(plm_t *self, int enabled);
int plm_get_num_video_streams(plm_t *self);
int plm_get_width(plm_t *self);
int plm_get_height(plm_t *self);
double plm_get_framerate(plm_t *self);
int plm_get_audio_enabled(plm_t *self);
void plm_set_audio_enabled(plm_t *self, int enabled);
int plm_get_num_audio_streams(plm_t *self);
void plm_set_audio_stream(plm_t *self, int stream_index);
int plm_get_samplerate(plm_t *self);
double plm_get_audio_lead_time(plm_t *self);
void plm_set_audio_lead_time(plm_t *self, double lead_time);
double plm_get_time(plm_t *self);
double plm_get_duration(plm_t *self);
void plm_rewind(plm_t *self);
int plm_get_loop(plm_t *self);
void plm_set_loop(plm_t *self, int loop);
int plm_has_ended(plm_t *self);
void plm_set_video_decode_callback(plm_t *self, plm_video_decode_callback fp, void *user);
void plm_set_audio_decode_callback(plm_t *self, plm_audio_decode_callback fp, void *user);
void plm_decode(plm_t *self, double seconds);
plm_frame_t *plm_decode_video(plm_t *self);
plm_samples_t *plm_decode_audio(plm_t *self);
int plm_seek(plm_t *self, double time, int seek_exact);
plm_frame_t *plm_seek_frame(plm_t *self, double time, int seek_exact);


// The default size for buffers created from files or by the high-level API
#ifndef PLM_BUFFER_DEFAULT_SIZE
#define PLM_BUFFER_DEFAULT_SIZE (128 * 1024)
#endif

plm_buffer_t *plm_buffer_create_with_filename(const char *filename);
plm_buffer_t *plm_buffer_create_with_file(FILE *fh, int close_when_done);
plm_buffer_t *plm_buffer_create_with_memory(uint8_t *bytes, size_t length, int free_when_done);
plm_buffer_t *plm_buffer_create_with_capacity(size_t capacity);
plm_buffer_t *plm_buffer_create_for_appending(size_t initial_capacity);
void plm_buffer_destroy(plm_buffer_t *self);
size_t plm_buffer_write(plm_buffer_t *self, uint8_t *bytes, size_t length);
void plm_buffer_signal_end(plm_buffer_t *self);
void plm_buffer_set_load_callback(plm_buffer_t *self, plm_buffer_load_callback fp, void *user);
void plm_buffer_rewind(plm_buffer_t *self);
size_t plm_buffer_get_size(plm_buffer_t *self);
size_t plm_buffer_get_remaining(plm_buffer_t *self);
int plm_buffer_has_ended(plm_buffer_t *self);
static const int PLM_DEMUX_PACKET_PRIVATE = 0xBD;
static const int PLM_DEMUX_PACKET_AUDIO_1 = 0xC0;
static const int PLM_DEMUX_PACKET_AUDIO_2 = 0xC1;
static const int PLM_DEMUX_PACKET_AUDIO_3 = 0xC2;
static const int PLM_DEMUX_PACKET_AUDIO_4 = 0xC2;
static const int PLM_DEMUX_PACKET_VIDEO_1 = 0xE0;
plm_demux_t *plm_demux_create(plm_buffer_t *buffer, int destroy_when_done);
void plm_demux_destroy(plm_demux_t *self);
int plm_demux_has_headers(plm_demux_t *self);
int plm_demux_get_num_video_streams(plm_demux_t *self);
int plm_demux_get_num_audio_streams(plm_demux_t *self);
void plm_demux_rewind(plm_demux_t *self);
int plm_demux_has_ended(plm_demux_t *self);
plm_packet_t *plm_demux_seek(plm_demux_t *self, double time, int type, int force_intra);
double plm_demux_get_start_time(plm_demux_t *self, int type);
double plm_demux_get_duration(plm_demux_t *self, int type);
plm_packet_t *plm_demux_decode(plm_demux_t *self);
plm_video_t *plm_video_create_with_buffer(plm_buffer_t *buffer, int destroy_when_done);
void plm_video_destroy(plm_video_t *self);
int plm_video_has_header(plm_video_t *self);
double plm_video_get_framerate(plm_video_t *self);
int plm_video_get_width(plm_video_t *self);
int plm_video_get_height(plm_video_t *self);
void plm_video_set_no_delay(plm_video_t *self, int no_delay);
double plm_video_get_time(plm_video_t *self);
void plm_video_set_time(plm_video_t *self, double time);
void plm_video_rewind(plm_video_t *self);
int plm_video_has_ended(plm_video_t *self);
plm_frame_t *plm_video_decode(plm_video_t *self);
int plm_buffer_skip_bytes(plm_buffer_t *self, uint8_t v);
int plm_buffer_read(plm_buffer_t *self, int count);
void plm_buffer_skip(plm_buffer_t *self, size_t count);
plm_audio_t *plm_audio_create_with_buffer(plm_buffer_t *buffer, int destroy_when_done);
void plm_audio_destroy(plm_audio_t *self);
int plm_audio_has_header(plm_audio_t *self);
int plm_audio_get_samplerate(plm_audio_t *self);
double plm_audio_get_time(plm_audio_t *self);
void plm_audio_set_time(plm_audio_t *self, double time);
void plm_audio_rewind(plm_audio_t *self);
int plm_audio_has_ended(plm_audio_t *self);
plm_samples_t *plm_audio_decode(plm_audio_t *self);

enum plm_buffer_mode {
    PLM_BUFFER_MODE_FILE,
    PLM_BUFFER_MODE_FIXED_MEM,
    PLM_BUFFER_MODE_RING,
    PLM_BUFFER_MODE_APPEND
};

typedef struct plm_buffer_t {
    size_t bit_index;
    size_t capacity;
    size_t length;
    size_t total_size;
    int discard_read_bytes;
    int has_ended;
    int free_when_done;
    int close_when_done;
    FILE *fh;
    plm_buffer_load_callback load_callback;
    void *load_callback_user_data;
    uint8_t *bytes;
    enum plm_buffer_mode mode;
} plm_buffer_t;

int plm_buffer_has(plm_buffer_t *self, size_t count);
void plm_buffer_align(plm_buffer_t *self);

typedef struct plm_t {
    plm_demux_t *demux;
    double time;
    int has_ended;
    int loop;
    int has_decoders;

    int video_enabled;
    int video_packet_type;
    plm_buffer_t *video_buffer;
    plm_video_t *video_decoder;

    int audio_enabled;
    int audio_stream_index;
    int audio_packet_type;
    double audio_lead_time;
    plm_buffer_t *audio_buffer;
    plm_audio_t *audio_decoder;

    plm_video_decode_callback video_decode_callback;
    void *video_decode_callback_user_data;

    plm_audio_decode_callback audio_decode_callback;
    void *audio_decode_callback_user_data;
} plm_t;

typedef struct {
    int16_t index;
    int16_t value;
} plm_vlc_t;

typedef struct {
    int16_t index;
    uint16_t value;
} plm_vlc_uint_t;

int plm_buffer_find_start_code(plm_buffer_t *self, int code);
int plm_buffer_has_start_code(plm_buffer_t *self, int code);
void plm_buffer_discard_read_bytes(plm_buffer_t *self);
int plm_buffer_next_start_code(plm_buffer_t *self);
int plm_buffer_peek_non_zero(plm_buffer_t *self, int bit_count);
int16_t plm_buffer_read_vlc(plm_buffer_t *self, const plm_vlc_t *table);
uint16_t plm_buffer_read_vlc_uint(plm_buffer_t *self, const plm_vlc_uint_t *table);

#endif // PL_MPEG_H





