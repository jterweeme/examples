//file: pl_mpeg.cpp

#include "pl_mpeg.h"
#include <string.h>
#include <stdlib.h>

#define PLM_UNUSED(expr) (void)(expr)

void PLM::plm_create_with_filename(const char *filename)
{
    _file_buffer.create_with_filename(filename);
    plm_create_with_buffer(&_file_buffer, TRUE);
}

void PLM::plm_create_with_file(FILE *fh, int close_when_done)
{
    _file_buffer.create_with_file(fh, close_when_done);
    plm_create_with_buffer(&_file_buffer, TRUE);
}

void PLM::plm_create_with_buffer(Buffer *buffer, int destroy_when_done)
{
    _demux.plm_demux_create(buffer, destroy_when_done);
    _video_enabled = TRUE;
    audio_enabled = TRUE;
    plm_init_decoders();
}

int PLM::plm_init_decoders()
{
    if (_has_decoders)
        return TRUE;

    if (!_demux.plm_demux_has_headers())
        return FALSE;

    if (_demux.plm_demux_get_num_video_streams() > 0)
    {
        if (_video_enabled)
            _video_packet_type = Demux::PLM_DEMUX_PACKET_VIDEO_1;
        
        _video_buffer.create_with_capacity(PLM_BUFFER_DEFAULT_SIZE);
        _video_buffer.plm_buffer_set_load_callback(plm_read_video_packet, this);
    }

    if (_demux.plm_demux_get_num_audio_streams() > 0)
    {
        if (audio_enabled)
            audio_packet_type = Demux::PLM_DEMUX_PACKET_AUDIO_1 + audio_stream_index;
        
        _audio_buffer.create_with_capacity(PLM_BUFFER_DEFAULT_SIZE);
        _audio_buffer.plm_buffer_set_load_callback(plm_read_audio_packet, this);
    }

    _video.plm_video_create_with_buffer(&_video_buffer, TRUE);
    _audio.plm_audio_create_with_buffer(&_audio_buffer, TRUE);
    _has_decoders = TRUE;
    return TRUE;
}

void PLM::plm_destroy()
{
    _video.plm_video_destroy();
    _audio.plm_audio_destroy();
    _demux.plm_demux_destroy();
}

int PLM::plm_get_audio_enabled() {
    return audio_enabled;
}

int PLM::plm_has_headers()
{
    if (!_demux.plm_demux_has_headers())
        return FALSE;
    
    if (!plm_init_decoders())
        return FALSE;

    if (!_video.plm_video_has_header() || !_audio.plm_audio_has_header())
        return FALSE;

    return TRUE;
}

void PLM::plm_set_audio_enabled(int enabled)
{
    audio_enabled = enabled;

    if (!enabled)
    {
        audio_packet_type = 0;
        return;
    }

    audio_packet_type = plm_init_decoders() ?
            Demux::PLM_DEMUX_PACKET_AUDIO_1 + audio_stream_index : 0;
}

int PLM::plm_get_video_enabled() {
    return _video_enabled;
}

void PLM::plm_set_video_enabled(int enabled)
{
    _video_enabled = enabled;

    if (!enabled) {
        _video_packet_type = 0;
        return;
    }

    _video_packet_type = plm_init_decoders() ? Demux::PLM_DEMUX_PACKET_VIDEO_1 : 0;
}

// Get the number of video streams (0--1) reported in the system header.
int PLM::plm_get_num_video_streams() {
    return _demux.plm_demux_get_num_video_streams();
}

// Get the display width/height of the video stream.
int PLM::plm_get_width() {
    return plm_init_decoders() ? _video.plm_video_get_width() : 0;
}

int PLM::plm_get_height() {
    return plm_init_decoders() ? _video.plm_video_get_height() : 0;
}

// Get the framerate of the video stream in frames per second.
double PLM::plm_get_framerate() {
    return plm_init_decoders() ? _video.plm_video_get_framerate() : 0;
}

// Get the number of audio streams (0--4) reported in the system header.
int PLM::plm_get_num_audio_streams() {
    return _demux.plm_demux_get_num_audio_streams();
}

// Get the samplerate of the audio stream in samples per second.
int PLM::plm_get_samplerate() {
    return (plm_init_decoders()) ? _audio.plm_audio_get_samplerate() : 0;
}

double PLM::plm_get_time() {
    return _time;
}

// Get the video duration of the underlying source in seconds.
double PLM::plm_get_duration() {
    return _demux.plm_demux_get_duration(Demux::PLM_DEMUX_PACKET_VIDEO_1);
}

void PLM::plm_set_loop(int loop) {
    _loop = loop;
}

// Get whether the file has ended. If looping is enabled, this will always
// return FALSE.
int PLM::plm_has_ended() {
    return _has_ended;
}

plm_frame_t *PLM::plm_decode_video()
{
    if (!plm_init_decoders())
        return NULL;

    if (!_video_packet_type)
        return NULL;

    plm_frame_t *frame = _video.plm_video_decode();

    if (frame)
        _time = frame->time;
    else if (_demux.plm_demux_has_ended())
        plm_handle_end();
    
    return frame;
}

void PLM::plm_handle_end() {
    _has_ended = TRUE;
}

void PLM::plm_read_video_packet(Buffer *buffer, void *user)
{
    PLM_UNUSED(buffer);
    PLM *plm = (PLM *)(user);
    plm_read_packets(plm, plm->_video_packet_type);
}

void PLM::plm_read_audio_packet(Buffer *buffer, void *user)
{
    PLM_UNUSED(buffer);
    PLM *plm = (PLM *)(user);
    plm_read_packets(plm, plm->audio_packet_type);
}

void PLM::plm_read_packets(PLM *self, int requested_type)
{
    plm_packet_t *packet;
    while ((packet = self->_demux.plm_demux_decode()))
    {
        if (packet->type == self->_video_packet_type)
            self->_video_buffer.plm_buffer_write(packet->data, packet->length);
        else if (packet->type == self->audio_packet_type)
            self->_audio_buffer.plm_buffer_write(packet->data, packet->length);

        if (packet->type == requested_type)
            return;
    }

    if (self->_demux.plm_demux_has_ended())
    {
        self->_video_buffer.plm_buffer_signal_end();
        self->_audio_buffer.plm_buffer_signal_end();
    }
}

// Create a buffer instance with a filename. Returns NULL if the file could not
// be opened.
void Buffer::create_with_filename(const char *filename)
{
    FILE *fh = fopen(filename, "rb");

    if (!fh)
        throw "error";
    
    create_with_file(fh, TRUE);
}

size_t Buffer::bit_index() const
{
    return _bit_index;
}

void Buffer::create_with_file(FILE *fh, int close_when_done)
{
    create_with_capacity(PLM_BUFFER_DEFAULT_SIZE);
    _fh = fh;
    _close_when_done = close_when_done;
    _mode = PLM_BUFFER_MODE_FILE;
    _discard_read_bytes = TRUE;
    
    fseek(_fh, 0, SEEK_END);
    _total_size = ftell(_fh);
    fseek(_fh, 0, SEEK_SET);

    plm_buffer_set_load_callback(plm_buffer_load_file_callback, NULL);
}

void Buffer::create_with_capacity(size_t capacity)
{
    _capacity = capacity;
    _free_when_done = TRUE;
    _bytes = (uint8_t *)malloc(capacity);
    _mode = PLM_BUFFER_MODE_RING;
    _discard_read_bytes = TRUE;
}

void Buffer::destroy()
{
    if (_fh && _close_when_done)
        fclose(_fh);
    
    if (_free_when_done)
        free(_bytes);
}

// Get the total size. For files, this returns the file size. For all other 
// types it returns the number of bytes currently in the buffer.
size_t Buffer::get_size() {
    return _mode == PLM_BUFFER_MODE_FILE ? _total_size : _length;
}

// Get the number of remaining (yet unread) bytes in the buffer. This can be
// useful to throttle writing.
size_t Buffer::plm_buffer_get_remaining() {
    return _length - (_bit_index >> 3);
}

// Copy data into the buffer. If the data to be written is larger than the 
// available space, the buffer will realloc() with a larger capacity. 
// Returns the number of bytes written. This will always be the same as the
// passed in length, except when the buffer was created _with_memory() for
// which _write() is forbidden.
size_t Buffer::plm_buffer_write(uint8_t *bytes, size_t length)
{
    if (_mode == PLM_BUFFER_MODE_FIXED_MEM)
        return 0;

    if (_discard_read_bytes) {
        // This should be a ring buffer, but instead it just shifts all unread 
        // data to the beginning of the buffer and appends new data at the end. 
        // Seems to be good enough.

        plm_buffer_discard_read_bytes();
        if (_mode == PLM_BUFFER_MODE_RING)
            _total_size = 0;
    }

    // Do we have to resize to fit the new data?
    size_t bytes_available = _capacity - _length;
    if (bytes_available < length) {
        size_t new_size = _capacity;
        do {
            new_size *= 2;
        } while (new_size - _length < length);
        _bytes = (uint8_t *)realloc(_bytes, new_size);
        _capacity = new_size;
    }

    memcpy(_bytes + _length, bytes, length);
    _length += length;
    _has_ended = FALSE;
    return length;
}

// Mark the current byte length as the end of this buffer and signal that no 
// more data is expected to be written to it. This function should be called
// just after the last plm_buffer_write().
// For _with_capacity buffers, this is cleared on a plm_buffer_rewind().
void Buffer::plm_buffer_signal_end() {
    _total_size = _length;
}

// Set a callback that is called whenever the buffer needs more data
void Buffer::plm_buffer_set_load_callback(plm_buffer_load_callback fp, void *user)
{
    _load_callback = fp;
    _load_callback_user_data = user;
}

// Rewind the buffer back to the beginning. When loading from a file handle,
// this also seeks to the beginning of the file.
void Buffer::rewind() {
    plm_buffer_seek(0);
}

void Buffer::plm_buffer_seek(size_t pos)
{
    _has_ended = FALSE;

    if (_mode == PLM_BUFFER_MODE_FILE)
    {
        fseek(_fh, pos, SEEK_SET);
        _bit_index = 0;
        _length = 0;
    }
    else if (_mode == PLM_BUFFER_MODE_RING)
    {
        if (pos != 0) {
            // Seeking to non-0 is forbidden for dynamic-mem buffers
            return; 
        }
        _bit_index = 0;
        _length = 0;
        _total_size = 0;
    }
    else if (pos < _length) {
        _bit_index = pos << 3;
    }
}

size_t Buffer::plm_buffer_tell() {
    return _mode == PLM_BUFFER_MODE_FILE ? ftell(_fh) + (_bit_index >> 3) - _length
        : _bit_index >> 3;
}

void Buffer::plm_buffer_discard_read_bytes() {
    size_t byte_pos = _bit_index >> 3;
    if (byte_pos == _length) {
        _bit_index = 0;
        _length = 0;
    }
    else if (byte_pos > 0) {
        memmove(_bytes, _bytes + byte_pos, _length - byte_pos);
        _bit_index -= byte_pos << 3;
        _length -= byte_pos;
    }
}

void Buffer::plm_buffer_load_file_callback(Buffer *self, void *user)
{
    PLM_UNUSED(user);
    
    if (self->_discard_read_bytes)
        self->plm_buffer_discard_read_bytes();

    size_t bytes_available = self->_capacity - self->_length;
    size_t bytes_read = fread(self->_bytes + self->_length, 1, bytes_available, self->_fh);
    self->_length += bytes_read;

    if (bytes_read == 0)
        self->_has_ended = TRUE;
}

// Get whether the read position of the buffer is at the end and no more data 
// is expected.
int Buffer::plm_buffer_has_ended() {
    return _has_ended;
}

int Buffer::plm_buffer_has(size_t count)
{
    if (((_length << 3) - _bit_index) >= count)
        return TRUE;

    if (_load_callback)
    {
        _load_callback(this, _load_callback_user_data);
        
        if (((_length << 3) - _bit_index) >= count)
            return TRUE;
    }   
    
    if (_total_size != 0 && _length == _total_size)
        _has_ended = TRUE;
    
    return FALSE;
}

int Buffer::read(int count)
{
    if (!plm_buffer_has(count))
        return 0;

    int value = 0;
    while (count) {
        int current_byte = _bytes[_bit_index >> 3];

        int remaining = 8 - (_bit_index & 7); // Remaining bits in byte
        int read = remaining < count ? remaining : count; // Bits in self run
        int shift = remaining - read;
        int mask = (0xff >> (8 - read));

        value = (value << read) | ((current_byte & (mask << shift)) >> shift);

        _bit_index += read;
        count -= read;
    }

    return value;
}

void Buffer::plm_buffer_align() {
    _bit_index = ((_bit_index + 7) >> 3) << 3; // Align to next byte
}

void Buffer::skip(size_t count)
{
    if (plm_buffer_has(count))
        _bit_index += count;
}

int Buffer::skip_bytes(uint8_t v)
{
    plm_buffer_align();
    int skipped = 0;
    while (plm_buffer_has(8) && _bytes[_bit_index >> 3] == v) {
        _bit_index += 8;
        skipped++;
    }
    return skipped;
}

int Buffer::plm_buffer_next_start_code()
{
    plm_buffer_align();

    while (plm_buffer_has(5 << 3)) {
        size_t byte_index = (_bit_index) >> 3;
        if (
            _bytes[byte_index + 0] == 0x00 &&
            _bytes[byte_index + 1] == 0x00 &&
            _bytes[byte_index + 2] == 0x01)
        {
            _bit_index = (byte_index + 4) << 3;
            return _bytes[byte_index + 3];
        }
        _bit_index += 8;
    }
    return -1;
}

int Buffer::plm_buffer_find_start_code(int code) {
    int current = 0;
    while (TRUE) {
        current = plm_buffer_next_start_code();
        if (current == code || current == -1) {
            return current;
        }
    }
    return -1;
}

int Buffer::plm_buffer_has_start_code(int code) {
    size_t previous_bit_index = _bit_index;
    int previous_discard_read_bytes = _discard_read_bytes;
    
    _discard_read_bytes = FALSE;
    int current = plm_buffer_find_start_code(code);

    _bit_index = previous_bit_index;
    _discard_read_bytes = previous_discard_read_bytes;
    return current;
}

int Buffer::plm_buffer_peek_non_zero(int bit_count)
{
    if (!plm_buffer_has(bit_count))
        return FALSE;

    int val = read(bit_count);
    _bit_index -= bit_count;
    return val != 0;
}

int16_t Buffer::read_vlc(const plm_vlc_t *table)
{
    plm_vlc_t state = {0, 0};
    do {
        state = table[state.index + read(1)];
    } while (state.index > 0);
    return state.value;
}

uint16_t Buffer::read_vlc_uint(const plm_vlc_uint_t *table) {
    return (uint16_t)read_vlc((const plm_vlc_t *)(table));
}

// Create a demuxer with a plm_buffer as source. This will also attempt to read
// the pack and system headers from the buffer.
void Demux::plm_demux_create(Buffer *buffer, int destroy_when_done)
{
    _buffer = buffer;
    _destroy_buffer_when_done = destroy_when_done;

    _start_time = PLM_PACKET_INVALID_TS;
    _duration = PLM_PACKET_INVALID_TS;
    _start_code = -1;

    plm_demux_has_headers();
}

// Destroy a demuxer and free all data.
void Demux::plm_demux_destroy()
{
    if (_destroy_buffer_when_done)
        _buffer->destroy();
}

// Returns TRUE/FALSE whether pack and system headers have been found. This will
// attempt to read the headers if non are present yet.
int Demux::plm_demux_has_headers() 
{
    if (_has_headers)
        return TRUE;

    // Decode pack header
    if (!_has_pack_header)
    {
        if (_start_code != PLM_START_PACK &&
            _buffer->plm_buffer_find_start_code(PLM_START_PACK) == -1
        ) {
            return FALSE;
        }

        _start_code = PLM_START_PACK;

        if (!_buffer->plm_buffer_has(64))
            return FALSE;
        
        _start_code = -1;

        if (_buffer->read(4) != 0x02)
            return FALSE;

        _system_clock_ref = plm_demux_decode_time();
        _buffer->skip(1);
        _buffer->skip(22); // mux_rate * 50
        _buffer->skip(1);

        _has_pack_header = TRUE;
    }

    // Decode system header
    if (!_has_system_header)
    {
        if (_start_code != PLM_START_SYSTEM &&
            _buffer->plm_buffer_find_start_code(PLM_START_SYSTEM) == -1)
        {
            return FALSE;
        }

        _start_code = PLM_START_SYSTEM;
        if (!_buffer->plm_buffer_has(56)) {
            return FALSE;
        }
        _start_code = -1;

        _buffer->skip(16); // header_length
        _buffer->skip(24); // rate bound
        _num_audio_streams = _buffer->read(6);
        _buffer->skip(5); // misc flags
        _num_video_streams = _buffer->read(5);

        _has_system_header = TRUE;
    }

    _has_headers = TRUE;
    return TRUE;
}

// Returns the number of video streams found in the system header. This will
// attempt to read the system header if non is present yet.
int Demux::plm_demux_get_num_video_streams() {
    return plm_demux_has_headers() ? _num_video_streams : 0;
}

// Returns the number of audio streams found in the system header. This will
// attempt to read the system header if non is present yet.
int Demux::plm_demux_get_num_audio_streams() {
    return plm_demux_has_headers() ? _num_audio_streams : 0;
}

// Rewind the internal buffer. See plm_buffer_rewind().
void Demux::plm_demux_rewind()
{
    _buffer->rewind();
    _current_packet.length = 0;
    _next_packet.length = 0;
    _start_code = -1;
}

// Get whether the file has ended. This will be cleared on seeking or rewind.
int Demux::plm_demux_has_ended() {
    return _buffer->plm_buffer_has_ended();
}

void Demux::plm_demux_buffer_seek(size_t pos)
{
    _buffer->plm_buffer_seek(pos);
    _current_packet.length = 0;
    _next_packet.length = 0;
    _start_code = -1;
}

// Get the PTS of the first packet of this type. Returns PLM_PACKET_INVALID_TS
// if not packet of this packet type can be found.
double Demux::plm_demux_get_start_time(int type)
{
    if (_start_time != PLM_PACKET_INVALID_TS)
        return _start_time;

    int previous_pos = _buffer->plm_buffer_tell();
    int previous_start_code = _start_code;
    
    // Find first video PTS
    plm_demux_rewind();
    do {
        plm_packet_t *packet = plm_demux_decode();
        if (!packet)
            break;
        
        if (packet->type == type)
            _start_time = packet->pts;
    } while (_start_time == PLM_PACKET_INVALID_TS);

    plm_demux_buffer_seek(previous_pos);
    _start_code = previous_start_code;
    return _start_time;
}

// Get the duration for the specified packet type - i.e. the span between the
// the first PTS and the last PTS in the data source. This only makes sense when
// the underlying data source is a file or fixed memory.
double Demux::plm_demux_get_duration(int type)
{
    size_t file_size = _buffer->get_size();

    if (_duration != PLM_PACKET_INVALID_TS && _last_file_size == file_size)
        return _duration;

    size_t previous_pos = _buffer->plm_buffer_tell();
    int previous_start_code = _start_code;
    
    // Find last video PTS. Start searching 64kb from the end and go further 
    // back if needed.
    long start_range = 64 * 1024;
    long max_range = 4096 * 1024;
    for (long range = start_range; range <= max_range; range *= 2) {
        long seek_pos = file_size - range;
        if (seek_pos < 0) {
            seek_pos = 0;
            range = max_range; // Make sure to bail after this round
        }
        plm_demux_buffer_seek(seek_pos);
        _current_packet.length = 0;

        double last_pts = PLM_PACKET_INVALID_TS;
        plm_packet_t *packet = NULL;

        while ((packet = plm_demux_decode())) {
            if (packet->pts != PLM_PACKET_INVALID_TS && packet->type == type)
                last_pts = packet->pts;
        }
        if (last_pts != PLM_PACKET_INVALID_TS) {
            _duration = last_pts - plm_demux_get_start_time(type);
            break;
        }
    }

    plm_demux_buffer_seek(previous_pos);
    _start_code = previous_start_code;
    _last_file_size = file_size;
    return _duration;
}

plm_packet_t *Demux::plm_demux_decode()
{
    if (!plm_demux_has_headers())
        return NULL;

    if (_current_packet.length)
    {
        size_t bits_till_next_packet = _current_packet.length << 3;

        if (!_buffer->plm_buffer_has(bits_till_next_packet))
            return NULL;
        
        _buffer->skip(bits_till_next_packet);
        _current_packet.length = 0;
    }

    // Pending packet waiting for data?
    if (_next_packet.length)
        return plm_demux_get_packet();

    // Pending packet waiting for header?
    if (_start_code != -1)
        return plm_demux_decode_packet(_start_code);

    do {
        _start_code = _buffer->plm_buffer_next_start_code();
        if (
            _start_code == PLM_DEMUX_PACKET_VIDEO_1 || 
            _start_code == PLM_DEMUX_PACKET_PRIVATE || (
                _start_code >= PLM_DEMUX_PACKET_AUDIO_1 && 
                _start_code <= PLM_DEMUX_PACKET_AUDIO_4
            )
        ) {
            return plm_demux_decode_packet(_start_code);
        }
    } while (_start_code != -1);

    return NULL;
}

double Demux::plm_demux_decode_time()
{
    int64_t clock = _buffer->read(3) << 30;
    _buffer->skip(1);
    clock |= _buffer->read(15) << 15;
    _buffer->skip(1);
    clock |= _buffer->read(15);
    _buffer->skip(1);
    return (double)clock / 90000.0;
}

plm_packet_t *Demux::plm_demux_decode_packet(int type) 
{
    if (!_buffer->plm_buffer_has(16 << 3))
        return NULL;

    _start_code = -1;

    _next_packet.type = type;
    _next_packet.length = _buffer->read(16);
    _next_packet.length -= _buffer->skip_bytes(0xff); // stuffing

    // skip P-STD
    if (_buffer->read(2) == 0x01)
    {
        _buffer->skip(16);
        _next_packet.length -= 2;
    }

    int pts_dts_marker = _buffer->read(2);

    if (pts_dts_marker == 0x03)
    {
        _next_packet.pts = plm_demux_decode_time();
        _last_decoded_pts = _next_packet.pts;
        _buffer->skip(40); // skip dts
        _next_packet.length -= 10;
    }
    else if (pts_dts_marker == 0x02) {
        _next_packet.pts = plm_demux_decode_time();
        _last_decoded_pts = _next_packet.pts;
        _next_packet.length -= 5;
    }
    else if (pts_dts_marker == 0x00) {
        _next_packet.pts = PLM_PACKET_INVALID_TS;
        _buffer->skip(4);
        _next_packet.length -= 1;
    }
    else {
        return NULL; // invalid
    }
    
    return plm_demux_get_packet();
}

plm_packet_t *Demux::plm_demux_get_packet()
{
    if (!_buffer->plm_buffer_has(_next_packet.length << 3))
        return NULL;

    _current_packet.data = _buffer->_bytes + (_buffer->bit_index() >> 3);
    _current_packet.length = _next_packet.length;
    _current_packet.type = _next_packet.type;
    _current_packet.pts = _next_packet.pts;

    _next_packet.length = 0;
    return &_current_packet;
}
