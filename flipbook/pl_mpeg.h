#ifndef PL_MPEG_H
#define PL_MPEG_H

#include <stdint.h>
#include <stdio.h>

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#if 0
template <struct T> struct VLC
{
    int16_t idx = 0;
    T val = 0;
}
#endif

struct plm_vlc_t
{
    int16_t index;
    int16_t value;
};

struct plm_vlc_uint_t
{
    int16_t index;
    uint16_t value;
};

// Demuxed MPEG PS packet
// The type maps directly to the various MPEG-PES start codes. PTS is the
// presentation time stamp of the packet in seconds. Note that not all packets
// have a PTS value, indicated by PLM_PACKET_INVALID_TS.

#define PLM_PACKET_INVALID_TS -1

struct plm_packet_t {
    int type = 0;
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
    unsigned width;
    unsigned height;
    uint8_t *data;
};

// Decoded Video Frame
// width and height denote the desired display size of the frame. This may be
// different from the internal size of the 3 planes.
struct plm_frame_t 
{
    unsigned width, height;
    plm_plane_t y, cr, cb;
};

struct plm_video_motion_t
{
    int full_px;
    int is_set;
    int r_size;
    int h;
    int v;
};

class Buffer;

// Callback function for plm_buffer when it needs more data
typedef void(*plm_buffer_load_callback)(Buffer *self, void *user);


// Callback function type for decoded video frames used by the high-level
// plm_* interface
typedef void(*plm_video_decode_callback)(plm_frame_t *frame, void *user);

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
    void plm_buffer_seek(size_t pos);
    size_t plm_buffer_tell();
    static void plm_buffer_load_file_callback(Buffer *self, void *user);
    int16_t read_vlc(const plm_vlc_t *table);
    uint16_t read_vlc_uint(const plm_vlc_uint_t *table);
    void create_with_file(FILE *fh);
    size_t bit_index() const;
    void create_with_capacity(size_t capacity);
    void destroy();
    size_t plm_buffer_write(uint8_t *bytes, size_t length);
    void plm_buffer_signal_end();
    void plm_buffer_set_load_callback(plm_buffer_load_callback fp, void *user);
    void rewind();
    size_t get_size();
    size_t plm_buffer_get_remaining();
    int plm_buffer_has_ended();
    int skip_bytes(uint8_t v);
    int read(int count);
    void skip(size_t count);
    int plm_buffer_has(size_t count);
    void plm_buffer_align();
    int find_start_code(int code);
    int plm_buffer_has_start_code(int code);
    void plm_buffer_discard_read_bytes();
    int plm_buffer_next_start_code();
    int plm_buffer_peek_non_zero(int bit_count);
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
    int _num_video_streams = 0;
    plm_packet_t _current_packet;
    plm_packet_t _next_packet;
    int _destroy_buffer_when_done;
    size_t _last_file_size = 0;
    Buffer *_buffer;
    plm_packet_t *plm_demux_get_packet();
public:
    static constexpr int PLM_DEMUX_PACKET_PRIVATE = 0xBD;
    static constexpr int PLM_DEMUX_PACKET_AUDIO_1 = 0xC0;
    static constexpr int PLM_DEMUX_PACKET_AUDIO_2 = 0xC1;
    static constexpr int PLM_DEMUX_PACKET_AUDIO_3 = 0xC2;
    static constexpr int PLM_DEMUX_PACKET_AUDIO_4 = 0xC2;
    static constexpr int PLM_DEMUX_PACKET_VIDEO_1 = 0xE0;
    void plm_demux_create(Buffer *buffer, int destroy_when_done);
    void plm_demux_buffer_seek(size_t pos);
    plm_packet_t *plm_demux_decode_packet(int type);
    void plm_demux_destroy();
    int plm_demux_has_headers();
    int plm_demux_get_num_video_streams();
    int plm_demux_get_num_audio_streams();
    void plm_demux_rewind();
    int plm_demux_has_ended();
    plm_packet_t *plm_demux_decode();
};

class Video
{
private:
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
    void _idct(int *block);
    void _decode_picture();
    void _decode_macroblock();
    void _decode_slice(int slice);
    void _decode_motion_vectors();
    void plm_video_init_frame(plm_frame_t *frame, uint8_t *base);
    int plm_video_decode_sequence_header();
    int plm_video_decode_motion_vector(int r_size, int motion);
public:
    void plm_video_destroy();
    int plm_video_has_header();
    int plm_video_get_width();
    int plm_video_get_height();
    void plm_video_set_no_delay(int no_delay);
    int plm_video_has_ended();
    plm_frame_t *plm_video_decode();
    void create(Buffer *buffer, int destroy_when_done = 0);
};

class PLM
{
private:
    int _loop = 0;
    int _has_ended = 0;
    int _has_decoders = 0;
    int _video_enabled = 0;
    int _video_packet_type = 0;
    Demux _demux;
    Buffer _file_buffer;
    Buffer _video_buffer;
    Video _video;
    void plm_create_with_memory(uint8_t *bytes, size_t length, int free_when_done);
    void plm_create_with_buffer(Buffer *buffer, int destroy_when_done);
    static void _read_packets(PLM *self, int requested_type);
    static void _read_video_packet(Buffer *buffer, void *user);
public:
    void plm_create_with_file(FILE *fh);
    int plm_init_decoders();
    void plm_handle_end();
    void plm_destroy();
    int plm_has_headers();
    void plm_set_video_enabled(int enabled);
    int plm_get_num_video_streams();
    int plm_get_width();
    int plm_get_height();
    void plm_set_loop(int loop);
    int plm_has_ended();
    plm_frame_t *plm_decode_video();
};
#endif





