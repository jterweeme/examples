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

template <typename T> struct VLC
{
    int16_t idx;
    T val;
};

struct plm_vlc_t
{
    int16_t index = 0;
    int16_t value = 0;
};

struct plm_vlc_uint_t
{
    int16_t index = 0;
    uint16_t value = 0;
};

// Demuxed MPEG PS packet
// The type maps directly to the various MPEG-PES start codes. PTS is the
// presentation time stamp of the packet in seconds. Note that not all packets
// have a PTS value, indicated by PLM_PACKET_INVALID_TS.

#define PLM_PACKET_INVALID_TS -1

struct plm_packet_t {
    int type = 0;
    double pts = 0;
    size_t length = 0;
    uint8_t *data = nullptr;
};


// Decoded Video Plane 
// The byte length of the data is width * height. Note that different planes
// have different sizes: the Luma plane (Y) is double the size of each of 
// the two Chroma planes (Cr, Cb) - i.e. 4 times the byte length.
// Also note that the size of the plane does *not* denote the size of the 
// displayed frame. The sizes of planes are always rounded up to the nearest
// macroblock (16px).
struct plm_plane_t
{
    unsigned int width;
    unsigned int height;
    uint8_t *data;
};

// Decoded Video Frame
// width and height denote the desired display size of the frame. This may be
// different from the internal size of the 3 planes.
struct plm_frame_t 
{
    double time;
    unsigned int width;
    unsigned int height;
    plm_plane_t y;
    plm_plane_t cr;
    plm_plane_t cb;
};


// Decoded Audio Samples
// Samples are stored as normalized (-1, 1) float either interleaved, or if
// PLM_AUDIO_SEPARATE_CHANNELS is defined, in two separate arrays.
// The `count` is always PLM_AUDIO_SAMPLES_PER_FRAME and just there for
// convenience.

#define PLM_AUDIO_SAMPLES_PER_FRAME 1152

struct plm_samples_t {
    double time;
    unsigned int count;
#ifdef PLM_AUDIO_SEPARATE_CHANNELS
    float left[PLM_AUDIO_SAMPLES_PER_FRAME];
    float right[PLM_AUDIO_SAMPLES_PER_FRAME];
#else
    float interleaved[PLM_AUDIO_SAMPLES_PER_FRAME * 2];
#endif
};



class Buffer;

// Callback function for plm_buffer when it needs more data
typedef void(*plm_buffer_load_callback)(Buffer *self, void *user);


// Callback function type for decoded video frames used by the high-level
// plm_* interface
typedef void(*plm_video_decode_callback)(plm_frame_t *frame, void *user);

// Callback function type for decoded audio samples used by the high-level
// plm_* interface
typedef void(*plm_audio_decode_callback)(plm_samples_t *samples, void *user);

// The default size for buffers created from files or by the high-level API
#ifndef PLM_BUFFER_DEFAULT_SIZE
#define PLM_BUFFER_DEFAULT_SIZE (128 * 1024)
#endif

enum plm_buffer_mode {
    PLM_BUFFER_MODE_FILE,
    PLM_BUFFER_MODE_FIXED_MEM,
    PLM_BUFFER_MODE_RING,
    PLM_BUFFER_MODE_APPEND
};

class Buffer
{
private:
    plm_buffer_load_callback _load_callback;
    void *_load_callback_user_data;
    int _discard_read_bytes = 0;
    int _has_ended = 0;
    int _free_when_done = 0;
    int _close_when_done = 0;
    size_t _capacity = 0;
    size_t _total_size = 0;
    FILE *_fh = nullptr;
    enum plm_buffer_mode _mode;
public:
    uint8_t *_bytes;
    size_t _length = 0;
    size_t _bit_index = 0;
    void seek(size_t pos);
    size_t tell();
    static void load_file_callback(Buffer *self, void *user);
    int16_t read_vlc(const plm_vlc_t *table);
    uint16_t read_vlc_uint(const plm_vlc_uint_t *table);
    void create_with_filename(const char *filename);
    void create_with_file(FILE *fh, int close_when_done);
    size_t bit_index() const;
    void create_with_capacity(size_t capacity);
    void destroy();
    size_t write(uint8_t *bytes, size_t length);
    void signal_end();
    void set_load_callback(plm_buffer_load_callback fp, void *user);
    void rewind();
    size_t get_size();
    size_t get_remaining();
    int has_ended();
    int skip_bytes(uint8_t v);
    int read(int count);
    void skip(size_t count);
    int has(size_t count);
    void align();
    int find_start_code(int code);
    int has_start_code(int code);
    void discard_read_bytes();
    int next_start_code();
    int peek_non_zero(int bit_count);
};

class Demux
{
private:
    static constexpr int PLM_START_PACK = 0xBA;
    static constexpr int PLM_START_END = 0xB9;
    static constexpr int PLM_START_SYSTEM = 0xBB;
    int _start_code = 0;
    int _has_pack_header = 0;
    int _has_system_header = 0;
    int _has_headers = 0;
    int _num_audio_streams = 0;
    int _num_video_streams = 0;
    plm_packet_t _current_packet;
    plm_packet_t _next_packet;
    int _destroy_buffer_when_done;
    double _system_clock_ref;
    double _duration = 0;
    size_t _last_file_size = 0;
    double _last_decoded_pts = 0;
    double _start_time = 0;
    Buffer *_buffer;
public:
    static constexpr int PACKET_PRIVATE = 0xBD;
    static constexpr int PACKET_AUDIO_1 = 0xC0;
    static constexpr int PACKET_AUDIO_2 = 0xC1;
    static constexpr int PACKET_AUDIO_3 = 0xC2;
    static constexpr int PACKET_AUDIO_4 = 0xC2;
    static constexpr int PACKET_VIDEO_1 = 0xE0;
    void create(Buffer *buffer, int destroy_when_done);
    void buffer_seek(size_t pos);
    double decode_time();
    plm_packet_t *decode_packet(int type);
    plm_packet_t *get_packet();
    void destroy();
    int has_headers();
    int get_num_video_streams();
    int get_num_audio_streams();
    void rewind();
    int has_ended();
    plm_packet_t *seek(double time, int type, int force_intra);
    double get_start_time(int type);
    double get_duration(int type);
    plm_packet_t *decode();
};

struct plm_quantizer_spec_t
{
    uint16_t levels;
    uint8_t group;
    uint8_t bits;
};

class Audio
{
private:
    Buffer *_buffer;
    int _destroy_buffer_when_done = 0;
    plm_samples_t _samples;
    float _D[1024];
    float _V[2][1024];
    float _U[32];
    const plm_quantizer_spec_t *_allocation[2][32];
    uint8_t _scale_factor_info[2][32];
    int _scale_factor[2][32][3];
    int _sample[2][32][3];
    double _time = 0;
    int _samples_decoded = 0;
    int _samplerate_index = 0;
    int _bitrate_index = 0;
    int _version = 0;
    int _layer = 0;
    int _mode = 0;
    int _bound = 0;
    int _v_pos = 0;
    int _next_frame_data_size = 0;
    int _has_header = 0;
    int _find_frame_sync();
    void _idct36(int s[32][3], int ss, float *d, int dp);
    const plm_quantizer_spec_t *_read_allocation(int sb, int tab3);
    void _decode_frame();
    void _read_samples(int ch, int sb, int part);
public:
    int has_header();
    double get_time();
    void destroy();
    void set_time(double time);
    int has_ended();
    plm_samples_t *decode();
    int get_samplerate();
    void rewind();
    int decode_header();
    void create(Buffer *buffer, int destroy_when_done);
};

struct plm_video_motion_t
{
    int full_px;
    int is_set;
    int r_size;
    int h;
    int v;
};

class Video
{
private:
    double _framerate = 0;
    double _time = 0;
    int _frames_decoded = 0;
    int _width = 0;
    int _height = 0;
    int _mb_width = 0;
    int _mb_height = 0;
    int _mb_size = 0;
    int _luma_width = 0;
    int _luma_height = 0;

    int _chroma_width = 0;
    int _chroma_height = 0;

    int _start_code = 0;
    int _picture_type = 0;
    int _has_sequence_header = 0;

    int _quantizer_scale = 0;
    int _slice_begin = 0;
    int _macroblock_address = 0;

    int _mb_row = 0;
    int _mb_col = 0;

    int _macroblock_type = 0;
    int _macroblock_intra = 0;
    plm_video_motion_t _motion_forward;
    plm_video_motion_t _motion_backward;
    int _dc_predictor[3];

    Buffer *_buffer;
    int _destroy_buffer_when_done;

    plm_frame_t _frame_current;
    plm_frame_t _frame_forward;
    plm_frame_t _frame_backward;
    uint8_t *_frames_data;

    int _block_data[64];
    uint8_t _intra_quant_matrix[64];
    uint8_t _non_intra_quant_matrix[64];

    int _has_reference_frame = 0;
    int _assume_no_b_frames = 0;
    void _copy_macroblock(plm_frame_t *s, int motion_h, int motion_v);
    void _process_macroblock(uint8_t *s, uint8_t *d, int mh, int mb, int bs, int interp);
    void _interpolate_macroblock(plm_frame_t *s, int motion_h, int motion_v);
    void _decode_block(int block);
    void _predict_macroblock();
    static void _idct(int *block);
    void _decode_picture();
    void _decode_macroblock();
    void _decode_slice(int slice);
    void _decode_motion_vectors();
    void _init_frame(plm_frame_t *frame, uint8_t *base);
    int _decode_sequence_header();
    int _decode_motion_vector(int r_size, int motion);
public:
    void destroy();
    int has_header();
    double get_framerate();
    int get_width();
    int get_height();
    void set_no_delay(int no_delay);
    double plm_video_get_time();
    void set_time(double time);
    void rewind();
    int has_ended();
    plm_frame_t *decode();
    void create(Buffer *buffer, int destroy_when_done);
};

class PLM
{
private:
    int _loop = 0;
    double _time = 0;
    int _has_ended = 0;
    int _has_decoders = 0;
    int _video_enabled = 0;
    int _video_packet_type = 0;
    Demux _demux;
    Buffer _file_buffer;
    Buffer _video_buffer;
    Buffer _audio_buffer;
    Video _video;
    Audio _audio;
    plm_video_decode_callback video_decode_callback = nullptr;
    void *video_decode_callback_user_data = nullptr;
    plm_audio_decode_callback audio_decode_callback = nullptr;
    void *audio_decode_callback_user_data = nullptr;
    void plm_create_with_file(FILE *fh, int close_when_done);
    void plm_create_with_memory(uint8_t *bytes, size_t length, int free_when_done);
    void plm_create_with_buffer(Buffer *buffer, int destroy_when_done);
    static void plm_read_packets(PLM *self, int requested_type);
    static void plm_read_audio_packet(Buffer *buffer, void *user);
    static void plm_read_video_packet(Buffer *buffer, void *user);
    double audio_lead_time;
    int audio_enabled = 0;
    int audio_stream_index = 0;
    int audio_packet_type = 0;
public:
    void plm_create_with_filename(const char *filename);
    int plm_init_decoders();
    void plm_handle_end();
    void plm_destroy();
    int plm_has_headers();
    int plm_get_video_enabled();
    void plm_set_video_enabled(int enabled);
    int plm_get_num_video_streams();
    int plm_get_width();
    int plm_get_height();
    double plm_get_framerate();
    int plm_get_audio_enabled();
    void plm_set_audio_enabled(int enabled);
    int plm_get_num_audio_streams();
    void plm_set_audio_stream(int stream_index);
    int plm_get_samplerate();
    double plm_get_audio_lead_time();
    void plm_set_audio_lead_time(double lead_time);
    double plm_get_time();
    double plm_get_duration();
    void plm_rewind();
    int plm_get_loop();
    void plm_set_loop(int loop);
    int plm_has_ended();
    void plm_set_video_decode_callback(plm_video_decode_callback fp, void *user);
    void plm_set_audio_decode_callback(plm_audio_decode_callback fp, void *user);
    void plm_decode(double seconds);
    plm_frame_t *plm_decode_video();
    plm_samples_t *plm_decode_audio();
    int plm_seek(double time, int seek_exact);
    plm_frame_t *plm_seek_frame(double time, int seek_exact);
};
#endif





