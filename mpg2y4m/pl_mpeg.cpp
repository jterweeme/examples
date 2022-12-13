//file: pl_mpeg.cpp

#include "pl_mpeg.h"
#include <string.h>
#include <stdlib.h>
#include <iostream>

#define PLM_UNUSED(expr) (void)(expr)

// Create a plmpeg instance with a filename. Returns NULL if the file could not
// be opened.
void PLM::plm_create_with_filename(const char *filename)
{
    _file_buffer.create_with_filename(filename);
    plm_create_with_buffer(&_file_buffer, TRUE);
}

// Create a plmpeg instance with a file handle. Pass TRUE to close_when_done to
// let plmpeg call fclose() on the handle when plm_destroy() is called.
void PLM::plm_create_with_file(FILE *fh, int close_when_done)
{
    _file_buffer.create_with_file(fh, close_when_done);
    plm_create_with_buffer(&_file_buffer, TRUE);
}

// Create a plmpeg instance with a plm_buffer as source. Pass TRUE to
// destroy_when_done to let plmpeg call plm_buffer_destroy() on the buffer when
// plm_destroy() is called.
void PLM::plm_create_with_buffer(Buffer *buffer, int destroy_when_done)
{
    _demux.create(buffer, destroy_when_done);
    _video_enabled = TRUE;
    audio_enabled = TRUE;
    plm_init_decoders();
}

int PLM::plm_init_decoders()
{
    if (_has_decoders)
        return TRUE;

    if (!_demux.has_headers())
        return FALSE;

    if (_demux.get_num_video_streams() > 0)
    {
        if (_video_enabled)
            _video_packet_type = Demux::PACKET_VIDEO_1;
        
        _video_buffer.create_with_capacity(PLM_BUFFER_DEFAULT_SIZE);
        _video_buffer.set_load_callback(plm_read_video_packet, this);
    }

    _video.create(&_video_buffer, TRUE);
    _has_decoders = TRUE;
    return TRUE;
}

// Destroy a plmpeg instance and free all data.
void PLM::plm_destroy()
{
    _video.destroy();
    _demux.destroy();
}

// Get or set whether audio decoding is enabled. Default TRUE.
int PLM::plm_get_audio_enabled() {
    return audio_enabled;
}

// Get whether we have headers on all available streams and we can accurately
// report the number of video/audio streams, video dimensions, framerate and
// audio samplerate.
// This returns FALSE if the file is not an MPEG-PS file or - when not using a
// file as source - when not enough data is available yet.
int PLM::plm_has_headers()
{
    if (!_demux.has_headers())
        return FALSE;
    
    if (!plm_init_decoders())
        return FALSE;

    if (!_video.has_header())
        return FALSE;

    return TRUE;
}

// Get or set whether video decoding is enabled. Default TRUE.
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

    _video_packet_type = plm_init_decoders() ? Demux::PACKET_VIDEO_1 : 0;
}

// Get the number of video streams (0--1) reported in the system header.
int PLM::plm_get_num_video_streams() {
    return _demux.get_num_video_streams();
}

// Get the display width/height of the video stream.
int PLM::plm_get_width() {
    return plm_init_decoders() ? _video.get_width() : 0;
}

int PLM::plm_get_height() {
    return plm_init_decoders() ? _video.get_height() : 0;
}

// Get the framerate of the video stream in frames per second.
double PLM::plm_get_framerate() {
    return plm_init_decoders() ? _video.get_framerate() : 0;
}

// Get the number of audio streams (0--4) reported in the system header.
int PLM::plm_get_num_audio_streams() {
    return _demux.get_num_audio_streams();
}

// Get the samplerate of the audio stream in samples per second.
int PLM::plm_get_samplerate() {
    return 0;
}

// Get or set the audio lead time in seconds - the time in which audio samples
// are decoded in advance (or behind) the video decode time. Typically this
// should be set to the duration of the buffer of the audio API that you use
// for output. E.g. for SDL2: (SDL_AudioSpec.samples / samplerate)
double PLM::plm_get_audio_lead_time() {
    return audio_lead_time;
}

void PLM::plm_set_audio_lead_time(double lead_time) {
    audio_lead_time = lead_time;
}

// Get the current internal time in seconds.
double PLM::plm_get_time() {
    return _time;
}

// Get the video duration of the underlying source in seconds.
double PLM::plm_get_duration() {
    return _demux.get_duration(Demux::PACKET_VIDEO_1);
}

// Rewind all buffers back to the beginning.
void PLM::plm_rewind()
{
    _video.rewind();
    _demux.rewind();
    _time = 0;
}

// Get or set looping. Default FALSE.
int PLM::plm_get_loop() {
    return _loop;
}

void PLM::plm_set_loop(int loop) {
    _loop = loop;
}

// Get whether the file has ended. If looping is enabled, this will always
// return FALSE.
int PLM::plm_has_ended() {
    return _has_ended;
}

// Set the callback for decoded video frames used with plm_decode(). If no 
// callback is set, video data will be ignored and not be decoded. The *user
// Parameter will be passed to your callback.
void PLM::plm_set_video_decode_callback(plm_video_decode_callback fp, void *user)
{
    video_decode_callback = fp;
    video_decode_callback_user_data = user;
}

// Set the callback for decoded audio samples used with plm_decode(). If no 
// callback is set, audio data will be ignored and not be decoded. The *user
// Parameter will be passed to your callback.
void PLM::plm_set_audio_decode_callback(plm_audio_decode_callback fp, void *user)
{
    audio_decode_callback = fp;
    audio_decode_callback_user_data = user;
}

// Advance the internal timer by seconds and decode video/audio up to this time.
// This will call the video_decode_callback and audio_decode_callback any number
// of times. A frame-skip is not implemented, i.e. everything up to current time
// will be decoded.
void PLM::plm_decode(double tick)
{
    if (!plm_init_decoders())
        return;

    int decode_video = video_decode_callback && _video_packet_type;
    int decode_audio = audio_decode_callback && audio_packet_type;

    if (!decode_video && !decode_audio) {
        // Nothing to do here
        return;
    }

    int did_decode = FALSE;
    int decode_video_failed = FALSE;
    int decode_audio_failed = FALSE;

    double video_target_time = _time + tick;
    double audio_target_time = _time + tick + audio_lead_time;

    do {
        did_decode = FALSE;
        
        if (decode_video && _video.plm_video_get_time() < video_target_time)
        {
            plm_frame_t *frame = _video.decode();
            if (frame) {
                video_decode_callback(frame, video_decode_callback_user_data);
                did_decode = TRUE;
            }
            else {
                decode_video_failed = TRUE;
            }
        }

    } while (did_decode);
    
    // Did all sources we wanted to decode fail and the demuxer is at the end?
    if ((!decode_video || decode_video_failed) && 
        (!decode_audio || decode_audio_failed) &&
        _demux.has_ended())
    {
        plm_handle_end();
        return;
    }

    _time += tick;
}

// Decode and return one video frame. Returns NULL if no frame could be decoded
// (either because the source ended or data is corrupt). If you only want to 
// decode video, you should disable audio via plm_set_audio_enabled().
// The returned plm_frame_t is valid until the next call to plm_decode_video() 
// or until plm_destroy() is called.
plm_frame_t *PLM::plm_decode_video()
{
    if (!plm_init_decoders())
        return NULL;

    if (!_video_packet_type)
        return NULL;

    plm_frame_t *frame = _video.decode();

    if (frame)
        _time = frame->time;
    else if (_demux.has_ended())
        plm_handle_end();
    
    return frame;
}

// Decode and return one audio frame. Returns NULL if no frame could be decoded
// (either because the source ended or data is corrupt). If you only want to 
// decode audio, you should disable video via plm_set_video_enabled().
// The returned plm_samples_t is valid until the next call to plm_decode_audio()
// or until plm_destroy() is called.
plm_samples_t *PLM::plm_decode_audio()
{
    return NULL;
}

void PLM::plm_handle_end() {
    if (_loop) {
        plm_rewind();
    } else {
        _has_ended = TRUE;
    }
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
    while ((packet = self->_demux.decode()))
    {
        if (packet->type == self->_video_packet_type)
            self->_video_buffer.write(packet->data, packet->length);

        if (packet->type == requested_type)
            return;
    }

    if (self->_demux.has_ended())
    {
        self->_video_buffer.signal_end();
        //self->_audio_buffer.signal_end();
    }
}

// Similar to plm_seek(), but will not call the video_decode_callback,
// audio_decode_callback or make any attempts to sync audio.
// Returns the found frame or NULL if no frame could be found.
plm_frame_t *PLM::plm_seek_frame(double time, int seek_exact)
{
    if (!plm_init_decoders())
        return NULL;

    if (!_video_packet_type)
        return NULL;

    int type = _video_packet_type;

    double start_time = _demux.get_start_time(type);
    double duration = _demux.get_duration(type);

    if (time < 0) {
        time = 0;
    }
    else if (time > duration) {
        time = duration;
    }
    
    plm_packet_t *packet = _demux.seek(time, type, TRUE);
    if (!packet)
        return NULL;

    // Disable writing to the audio buffer while decoding video
    int previous_audio_packet_type = audio_packet_type;
    audio_packet_type = 0;

    // Clear video buffer and decode the found packet
    _video.rewind();
    _video.set_time(packet->pts - start_time);
    _video_buffer.write(packet->data, packet->length);
    plm_frame_t *frame = _video.decode(); 

    // If we want to seek to an exact frame, we have to decode all frames
    // on top of the intra frame we just jumped to.
    if (seek_exact)
        while (frame && frame->time < time)
            frame = _video.decode();

    // Enable writing to the audio buffer again?
    audio_packet_type = previous_audio_packet_type;

    if (frame)
        _time = frame->time;

    _has_ended = FALSE;
    return frame;
}

// Seek to the specified time, clamped between 0 -- duration. This can only be 
// used when the underlying plm_buffer is seekable, i.e. for files, fixed 
// memory buffers or _for_appending buffers. 
// If seek_exact is TRUE this will seek to the exact time, otherwise it will 
// seek to the last intra frame just before the desired time. Exact seeking can 
// be slow, because all frames up to the seeked one have to be decoded on top of
// the previous intra frame.
// If seeking succeeds, this function will call the video_decode_callback 
// exactly once with the target frame. If audio is enabled, it will also call
// the audio_decode_callback any number of times, until the audio_lead_time is
// satisfied.
// Returns TRUE if seeking succeeded or FALSE if no frame could be found.
int PLM::plm_seek(double time, int seek_exact)
{
    plm_frame_t *frame = plm_seek_frame(time, seek_exact);
    
    if (!frame)
        return FALSE;

    if (video_decode_callback)
        video_decode_callback(frame, video_decode_callback_user_data);    

    // If audio is not enabled we are done here.
    if (!audio_packet_type)
        return TRUE;

    // Sync up Audio. This demuxes more packets until the first audio packet
    // with a PTS greater than the current time is found. plm_decode() is then
    // called to decode enough audio data to satisfy the audio_lead_time.
    double start_time = _demux.get_start_time(_video_packet_type);
    //_audio.rewind();

    plm_packet_t *packet = NULL;
    while ((packet = _demux.decode()))
    {
        if (packet->type == _video_packet_type) {
            _video_buffer.write(packet->data, packet->length);
        }
        else if (packet->type == audio_packet_type && packet->pts - start_time > _time)
        {
            //_audio.set_time(packet->pts - start_time);
            _audio_buffer.write(packet->data, packet->length);
            plm_decode(0);
            break;
        }
    }   
    
    return TRUE;
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

// Create a buffer instance with a file handle. Pass TRUE to close_when_done
// to let plmpeg call fclose() on the handle when plm_destroy() is called.
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

    set_load_callback(load_file_callback, NULL);
}

// Create an empty buffer with an initial capacity. The buffer will grow
// as needed. Data that has already been read, will be discarded.
void Buffer::create_with_capacity(size_t capacity)
{
    _capacity = capacity;
    _free_when_done = TRUE;
    _bytes = (uint8_t *)malloc(capacity);
    _mode = PLM_BUFFER_MODE_RING;
    _discard_read_bytes = TRUE;
}

// Destroy a buffer instance and free all data
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
size_t Buffer::get_remaining() {
    return _length - (_bit_index >> 3);
}

// Copy data into the buffer. If the data to be written is larger than the 
// available space, the buffer will realloc() with a larger capacity. 
// Returns the number of bytes written. This will always be the same as the
// passed in length, except when the buffer was created _with_memory() for
// which _write() is forbidden.
size_t Buffer::write(uint8_t *bytes, size_t length)
{
    if (_mode == PLM_BUFFER_MODE_FIXED_MEM)
        return 0;

    if (_discard_read_bytes) {
        // This should be a ring buffer, but instead it just shifts all unread 
        // data to the beginning of the buffer and appends new data at the end. 
        // Seems to be good enough.

        discard_read_bytes();
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
void Buffer::signal_end() {
    _total_size = _length;
}

// Set a callback that is called whenever the buffer needs more data
void Buffer::set_load_callback(plm_buffer_load_callback fp, void *user)
{
    _load_callback = fp;
    _load_callback_user_data = user;
}

// Rewind the buffer back to the beginning. When loading from a file handle,
// this also seeks to the beginning of the file.
void Buffer::rewind() {
    seek(0);
}

void Buffer::seek(size_t pos)
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
        // Seeking to non-0 is forbidden for dynamic-mem buffers
        if (pos != 0)
            return; 
        
        _bit_index = 0;
        _length = 0;
        _total_size = 0;
    }
    else if (pos < _length) {
        _bit_index = pos << 3;
    }
}

size_t Buffer::tell() {
    return _mode == PLM_BUFFER_MODE_FILE ? ftell(_fh) + (_bit_index >> 3) - _length
        : _bit_index >> 3;
}

void Buffer::discard_read_bytes() {
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

void Buffer::load_file_callback(Buffer *self, void *user)
{
    PLM_UNUSED(user);
    
    if (self->_discard_read_bytes)
        self->discard_read_bytes();

    size_t bytes_available = self->_capacity - self->_length;
    size_t bytes_read = fread(self->_bytes + self->_length, 1, bytes_available, self->_fh);
    self->_length += bytes_read;

    if (bytes_read == 0)
        self->_has_ended = TRUE;
}

// Get whether the read position of the buffer is at the end and no more data 
// is expected.
int Buffer::has_ended() {
    return _has_ended;
}

int Buffer::has(size_t count)
{
    if ((_length << 3) - _bit_index >= count)
        return TRUE;

    if (_load_callback)
    {
        _load_callback(this, _load_callback_user_data);
        
        if ((_length << 3) - _bit_index >= count)
            return TRUE;
    }   
    
    if (_total_size != 0 && _length == _total_size)
        _has_ended = TRUE;
    
    return FALSE;
}

int Buffer::read(int count)
{
    if (!has(count))
        return 0;

    int value = 0;
    while (count) {
        int current_byte = _bytes[_bit_index >> 3];

        int remaining = 8 - (_bit_index & 7); // Remaining bits in byte
        int read = remaining < count ? remaining : count; // Bits in self run
        int shift = remaining - read;
        int mask = 0xff >> 8 - read;

        value = value << read | (current_byte & mask << shift) >> shift;

        _bit_index += read;
        count -= read;
    }

    return value;
}

void Buffer::align() {
    _bit_index = ((_bit_index + 7) >> 3) << 3; // Align to next byte
}

void Buffer::skip(size_t count)
{
    if (has(count))
        _bit_index += count;
}

int Buffer::skip_bytes(uint8_t v)
{
    align();
    int skipped = 0;
    while (has(8) && _bytes[_bit_index >> 3] == v) {
        _bit_index += 8;
        skipped++;
    }
    return skipped;
}

int Buffer::next_start_code()
{
    align();

    while (has(5 << 3))
    {
        size_t i = (_bit_index) >> 3;

        if (_bytes[i + 0] == 0x00 && _bytes[i + 1] == 0x00 && _bytes[i + 2] == 0x01)
        {
            _bit_index = (i + 4) << 3;
            return _bytes[i + 3];
        }
        _bit_index += 8;
    }
    return -1;
}

int Buffer::find_start_code(int code) {
    int current = 0;
    while (TRUE) {
        current = next_start_code();
        if (current == code || current == -1)
            return current;
    }
    return -1;
}

int Buffer::has_start_code(int code) {
    size_t previous_bit_index = _bit_index;
    int previous_discard_read_bytes = _discard_read_bytes;
    
    _discard_read_bytes = FALSE;
    int current = find_start_code(code);

    _bit_index = previous_bit_index;
    _discard_read_bytes = previous_discard_read_bytes;
    return current;
}

int Buffer::peek_non_zero(int bit_count)
{
    if (!has(bit_count))
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
void Demux::create(Buffer *buffer, int destroy_when_done)
{
    _buffer = buffer;
    _destroy_buffer_when_done = destroy_when_done;

    _start_time = PLM_PACKET_INVALID_TS;
    _duration = PLM_PACKET_INVALID_TS;
    _start_code = -1;

    has_headers();
}

// Destroy a demuxer and free all data.
void Demux::destroy()
{
    if (_destroy_buffer_when_done)
        _buffer->destroy();
}

// Returns TRUE/FALSE whether pack and system headers have been found. This will
// attempt to read the headers if non are present yet.
int Demux::has_headers() 
{
    if (_has_headers)
        return TRUE;

    // Decode pack header
    if (!_has_pack_header)
    {
        if (_start_code != PLM_START_PACK && _buffer->find_start_code(PLM_START_PACK) == -1)
            return FALSE;

        _start_code = PLM_START_PACK;

        if (!_buffer->has(64))
            return FALSE;
        
        _start_code = -1;

        if (_buffer->read(4) != 0x02)
            return FALSE;

        _system_clock_ref = decode_time();
        _buffer->skip(1);
        _buffer->skip(22); // mux_rate * 50
        _buffer->skip(1);

        _has_pack_header = TRUE;
    }

    // Decode system header
    if (!_has_system_header)
    {
        if (_start_code != PLM_START_SYSTEM && _buffer->find_start_code(PLM_START_SYSTEM) == -1)
            return FALSE;

        _start_code = PLM_START_SYSTEM;
        if (!_buffer->has(56))
            return FALSE;
        
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
int Demux::get_num_video_streams() {
    return has_headers() ? _num_video_streams : 0;
}

// Returns the number of audio streams found in the system header. This will
// attempt to read the system header if non is present yet.
int Demux::get_num_audio_streams() {
    return has_headers() ? _num_audio_streams : 0;
}

// Rewind the internal buffer. See plm_buffer_rewind().
void Demux::rewind()
{
    _buffer->rewind();
    _current_packet.length = 0;
    _next_packet.length = 0;
    _start_code = -1;
}

// Get whether the file has ended. This will be cleared on seeking or rewind.
int Demux::has_ended() {
    return _buffer->has_ended();
}

void Demux::buffer_seek(size_t pos)
{
    _buffer->seek(pos);
    _current_packet.length = 0;
    _next_packet.length = 0;
    _start_code = -1;
}

// Get the PTS of the first packet of this type. Returns PLM_PACKET_INVALID_TS
// if not packet of this packet type can be found.
double Demux::get_start_time(int type)
{
    if (_start_time != PLM_PACKET_INVALID_TS)
        return _start_time;

    int previous_pos = _buffer->tell();
    int previous_start_code = _start_code;
    
    // Find first video PTS
    rewind();
    do {
        plm_packet_t *packet = decode();
        if (!packet)
            break;
        
        if (packet->type == type)
            _start_time = packet->pts;
    } while (_start_time == PLM_PACKET_INVALID_TS);

    buffer_seek(previous_pos);
    _start_code = previous_start_code;
    return _start_time;
}

// Get the duration for the specified packet type - i.e. the span between the
// the first PTS and the last PTS in the data source. This only makes sense when
// the underlying data source is a file or fixed memory.
double Demux::get_duration(int type)
{
    size_t file_size = _buffer->get_size();

    if (_duration != PLM_PACKET_INVALID_TS && _last_file_size == file_size)
        return _duration;

    size_t previous_pos = _buffer->tell();
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
        buffer_seek(seek_pos);
        _current_packet.length = 0;

        double last_pts = PLM_PACKET_INVALID_TS;
        plm_packet_t *packet = NULL;

        while ((packet = decode())) {
            if (packet->pts != PLM_PACKET_INVALID_TS && packet->type == type)
                last_pts = packet->pts;
        }
        if (last_pts != PLM_PACKET_INVALID_TS) {
            _duration = last_pts - get_start_time(type);
            break;
        }
    }

    buffer_seek(previous_pos);
    _start_code = previous_start_code;
    _last_file_size = file_size;
    return _duration;
}

// Seek to a packet of the specified type with a PTS just before specified time.
// If force_intra is TRUE, only packets containing an intra frame will be 
// considered - this only makes sense when the type is PLM_DEMUX_PACKET_VIDEO_1.
// Note that the specified time is considered 0-based, regardless of the first 
// PTS in the data source.
plm_packet_t *Demux::seek(double seek_time, int type, int force_intra)
{
    if (!has_headers())
        return NULL;

    // Using the current time, current byte position and the average bytes per
    // second for this file, try to jump to a byte position that hopefully has
    // packets containing timestamps within one second before to the desired 
    // seek_time.

    // If we hit close to the seek_time scan through all packets to find the
    // last one (just before the seek_time) containing an intra frame.
    // Otherwise we should at least be closer than before. Calculate the bytes
    // per second for the jumped range and jump again.

    // The number of retries here is hard-limited to a generous amount. Usually
    // the correct range is found after 1--5 jumps, even for files with very 
    // variable bitrates. If significantly more jumps are needed, there's
    // probably something wrong with the file and we just avoid getting into an
    // infinite loop. 32 retries should be enough for anybody.

    double duration = get_duration(type);
    long file_size = _buffer->get_size();
    long byterate = file_size / duration;

    double cur_time = _last_decoded_pts;
    double scan_span = 1;

    if (seek_time > duration)
        seek_time = duration;
    else if (seek_time < 0)
        seek_time = 0;
    
    seek_time += _start_time;

    for (int retry = 0; retry < 32; retry++)
    {
        int found_packet_with_pts = FALSE;
        int found_packet_in_range = FALSE;
        long last_valid_packet_start = -1;
        double first_packet_time = PLM_PACKET_INVALID_TS;

        long cur_pos = _buffer->tell();

        // Estimate byte offset and jump to it.
        long offset = (seek_time - cur_time - scan_span) * byterate;
        long seek_pos = cur_pos + offset;
        if (seek_pos < 0) {
            seek_pos = 0;
        }
        else if (seek_pos > file_size - 256) {
            seek_pos = file_size - 256;
        }

        buffer_seek(seek_pos);

        // Scan through all packets up to the seek_time to find the last packet
        // containing an intra frame.
        while (_buffer->find_start_code(type) != -1)
        {
            long packet_start = _buffer->tell();
            plm_packet_t *packet = decode_packet(type);

            // Skip packet if it has no PTS
            if (!packet || packet->pts == PLM_PACKET_INVALID_TS)
                continue;

            // Bail scanning through packets if we hit one that is outside
            // seek_time - scan_span.
            // We also adjust the cur_time and byterate values here so the next 
            // iteration can be a bit more precise.
            if (packet->pts > seek_time || packet->pts < seek_time - scan_span) {
                found_packet_with_pts = TRUE;
                byterate = (seek_pos - cur_pos) / (packet->pts - cur_time);
                cur_time = packet->pts;
                break;
            }

            // If we are still here, it means this packet is in close range to
            // the seek_time. If this is the first packet for this jump position
            // record the PTS. If we later have to back off, when there was no
            // intra frame in this range, we can lower the seek_time to not scan
            // this range again.
            if (!found_packet_in_range) {
                found_packet_in_range = TRUE;
                first_packet_time = packet->pts;
            }

            // Check if this is an intra frame packet. If so, record the buffer
            // position of the start of this packet. We want to jump back to it 
            // later, when we know it's the last intra frame before desired
            // seek time.
            if (force_intra) {
                for (size_t i = 0; i < packet->length - 6; i++) {
                    // Find the START_PICTURE code
                    if (
                        packet->data[i + 0] == 0x00 &&
                        packet->data[i + 1] == 0x00 &&
                        packet->data[i + 2] == 0x01 &&
                        packet->data[i + 3] == 0x00
                    ) {
                        // Bits 11--13 in the picture header contain the frame 
                        // type, where 1=Intra
                        if ((packet->data[i + 5] & 0x38) == 8)
                            last_valid_packet_start = packet_start;
                        
                        break;
                    }
                }
            }

            // If we don't want intra frames, just use the last PTS found.
            else {
                last_valid_packet_start = packet_start;
            }
        }

        // If there was at least one intra frame in the range scanned above,
        // our search is over. Jump back to the packet and decode it again.
        if (last_valid_packet_start != -1) {
            buffer_seek(last_valid_packet_start);
            return decode_packet(type);
        }

        // If we hit the right range, but still found no intra frame, we have
        // to increases the scan_span. This is done exponentially to also handle
        // video files with very few intra frames.
        else if (found_packet_in_range) {
            scan_span *= 2;
            seek_time = first_packet_time;
        }

        // If we didn't find any packet with a PTS, it probably means we reached
        // the end of the file. Estimate byterate and cur_time accordingly.
        else if (!found_packet_with_pts) {
            byterate = (seek_pos - cur_pos) / (duration - cur_time);
            cur_time = duration;
        }
    }

    return NULL;
}

// Decode and return the next packet. The returned packet_t is valid until
// the next call to plm_demux_decode() or until the demuxer is destroyed.
plm_packet_t *Demux::decode()
{
    if (!has_headers())
        return NULL;

    if (_current_packet.length)
    {
        size_t bits_till_next_packet = _current_packet.length << 3;

        if (!_buffer->has(bits_till_next_packet))
            return NULL;
        
        _buffer->skip(bits_till_next_packet);
        _current_packet.length = 0;
    }

    // Pending packet waiting for data?
    if (_next_packet.length)
        return get_packet();

    // Pending packet waiting for header?
    if (_start_code != -1)
        return decode_packet(_start_code);

    do {
        _start_code = _buffer->next_start_code();
        if (_start_code == PACKET_VIDEO_1 || _start_code == PACKET_PRIVATE ||
            (_start_code >= PACKET_AUDIO_1 && _start_code <= PACKET_AUDIO_4))
        {
            return decode_packet(_start_code);
        }
    } while (_start_code != -1);

    return NULL;
}

double Demux::decode_time()
{
    int64_t clock = _buffer->read(3) << 30;
    _buffer->skip(1);
    clock |= _buffer->read(15) << 15;
    _buffer->skip(1);
    clock |= _buffer->read(15);
    _buffer->skip(1);
    return (double)clock / 90000.0;
}

plm_packet_t *Demux::decode_packet(int type) 
{
    if (!_buffer->has(16 << 3))
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
        _next_packet.pts = decode_time();
        _last_decoded_pts = _next_packet.pts;
        _buffer->skip(40); // skip dts
        _next_packet.length -= 10;
    }
    else if (pts_dts_marker == 0x02) {
        _next_packet.pts = decode_time();
        _last_decoded_pts = _next_packet.pts;
        _next_packet.length -= 5;
    }
    else if (pts_dts_marker == 0x00) {
        _next_packet.pts = PLM_PACKET_INVALID_TS;
        _buffer->skip(4);
        _next_packet.length -= 1;
    }
    else {
        std::cerr << "invalid\r\n";
        std::cerr.flush();
        return NULL; // invalid
    }
    
    return get_packet();
}

plm_packet_t *Demux::get_packet()
{
    if (!_buffer->has(_next_packet.length << 3))
        return NULL;

    _current_packet.data = _buffer->_bytes + (_buffer->bit_index() >> 3);
    _current_packet.length = _next_packet.length;
    _current_packet.type = _next_packet.type;
    _current_packet.pts = _next_packet.pts;

    _next_packet.length = 0;
    return &_current_packet;
}

