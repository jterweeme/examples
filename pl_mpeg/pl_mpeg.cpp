//file: pl_mpeg.cpp

#include "pl_mpeg.h"
#include <string.h>
#include <stdlib.h>

#define PLM_UNUSED(expr) (void)(expr)

// Create a plmpeg instance with a filename. Returns NULL if the file could not
// be opened.
void PLM::plm_create_with_filename(const char *filename)
{
    plm_buffer_t *buffer = Buffer::plm_buffer_create_with_filename(filename);
    if (!buffer)
        throw "error";
    
    plm_create_with_buffer(buffer, TRUE);
}

// Create a plmpeg instance with a file handle. Pass TRUE to close_when_done to
// let plmpeg call fclose() on the handle when plm_destroy() is called.
void PLM::plm_create_with_file(FILE *fh, int close_when_done)
{
    plm_buffer_t *buffer = Buffer::plm_buffer_create_with_file(fh, close_when_done);
    plm_create_with_buffer(buffer, TRUE);
}

// Create a plmpeg instance with a pointer to memory as source. This assumes the
// whole file is in memory. The memory is not copied. Pass TRUE to 
// free_when_done to let plmpeg call free() on the pointer when plm_destroy() 
// is called.
void PLM::plm_create_with_memory(uint8_t *bytes, size_t length, int free_when_done)
{
    plm_buffer_t *buffer = Buffer::plm_buffer_create_with_memory(bytes, length, free_when_done);
    plm_create_with_buffer(buffer, TRUE);
}

// Create a plmpeg instance with a plm_buffer as source. Pass TRUE to
// destroy_when_done to let plmpeg call plm_buffer_destroy() on the buffer when
// plm_destroy() is called.
void PLM::plm_create_with_buffer(plm_buffer_t *buffer, int destroy_when_done)
{
    demux = Demux::plm_demux_create(buffer, destroy_when_done);
    _video_enabled = TRUE;
    audio_enabled = TRUE;
    plm_init_decoders();
}

int PLM::plm_init_decoders()
{
    if (_has_decoders)
        return TRUE;

    if (!Demux::plm_demux_has_headers(demux))
        return FALSE;

    if (Demux::plm_demux_get_num_video_streams(demux) > 0)
    {
        if (_video_enabled)
            _video_packet_type = Demux::PLM_DEMUX_PACKET_VIDEO_1;
        
        video_buffer = Buffer::plm_buffer_create_with_capacity(PLM_BUFFER_DEFAULT_SIZE);

        Buffer::plm_buffer_set_load_callback(
            video_buffer, PLM::plm_read_video_packet, this);
    }

    if (Demux::plm_demux_get_num_audio_streams(demux) > 0)
    {
        if (audio_enabled)
            audio_packet_type = Demux::PLM_DEMUX_PACKET_AUDIO_1 + audio_stream_index;
        
        audio_buffer = Buffer::plm_buffer_create_with_capacity(PLM_BUFFER_DEFAULT_SIZE);

        Buffer::plm_buffer_set_load_callback(
            audio_buffer, PLM::plm_read_audio_packet, this);
    }

    if (video_buffer)
        video_decoder = Video::plm_video_create_with_buffer(video_buffer, TRUE);

    if (audio_buffer)
        audio_decoder = Audio::plm_audio_create_with_buffer(audio_buffer, TRUE);

    _has_decoders = TRUE;
    return TRUE;
}

// Destroy a plmpeg instance and free all data.
void PLM::plm_destroy()
{
    if (video_decoder)
        Video::plm_video_destroy(video_decoder);
    
    if (audio_decoder)
        Audio::plm_audio_destroy(audio_decoder);

    Demux::plm_demux_destroy(demux);
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
    if (!Demux::plm_demux_has_headers(demux))
        return FALSE;
    
    if (!plm_init_decoders())
        return FALSE;

    if (
        (video_decoder && !Video::plm_video_has_header(video_decoder)) ||
        (audio_decoder && !Audio::plm_audio_has_header(audio_decoder))
    ) {
        return FALSE;
    }

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

    audio_packet_type = plm_init_decoders() && audio_decoder
        ? Demux::PLM_DEMUX_PACKET_AUDIO_1 + audio_stream_index
        : 0;
}

// Set the desired audio stream (0--3). Default 0.
void PLM::plm_set_audio_stream(int stream_index)
{
    if (stream_index < 0 || stream_index > 3)
        return;
    
    audio_stream_index = stream_index;

    // Set the correct audio_packet_type
    plm_set_audio_enabled(audio_enabled);
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

    _video_packet_type = (plm_init_decoders() && video_decoder)
        ? Demux::PLM_DEMUX_PACKET_VIDEO_1 : 0;
}

// Get the number of video streams (0--1) reported in the system header.
int PLM::plm_get_num_video_streams() {
    return Demux::plm_demux_get_num_video_streams(demux);
}

// Get the display width/height of the video stream.
int PLM::plm_get_width() {
    return (plm_init_decoders() && video_decoder)
        ? Video::plm_video_get_width(video_decoder) : 0;
}

int PLM::plm_get_height()
{
    return (plm_init_decoders() && video_decoder)
        ? Video::plm_video_get_height(video_decoder) : 0;
}

// Get the framerate of the video stream in frames per second.
double PLM::plm_get_framerate() {
    return (plm_init_decoders() && video_decoder)
        ? Video::plm_video_get_framerate(video_decoder) : 0;
}

// Get the number of audio streams (0--4) reported in the system header.
int PLM::plm_get_num_audio_streams() {
    return Demux::plm_demux_get_num_audio_streams(demux);
}

// Get the samplerate of the audio stream in samples per second.
int PLM::plm_get_samplerate() {
    return (plm_init_decoders() && audio_decoder)
        ? Audio::plm_audio_get_samplerate(audio_decoder) : 0;
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
    return Demux::plm_demux_get_duration(demux, Demux::PLM_DEMUX_PACKET_VIDEO_1);
}

// Rewind all buffers back to the beginning.
void PLM::plm_rewind()
{
    if (video_decoder)
        Video::plm_video_rewind(video_decoder);

    if (audio_decoder)
        Audio::plm_audio_rewind(audio_decoder);

    Demux::plm_demux_rewind(demux);
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

    int decode_video = (video_decode_callback && _video_packet_type);
    int decode_audio = (audio_decode_callback && audio_packet_type);

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
        
        if (decode_video && Video::plm_video_get_time(video_decoder) < video_target_time)
        {
            plm_frame_t *frame = Video::plm_video_decode(video_decoder);
            if (frame) {
                video_decode_callback(nullptr, frame, video_decode_callback_user_data);
                did_decode = TRUE;
            }
            else {
                decode_video_failed = TRUE;
            }
        }

        if (decode_audio && Audio::plm_audio_get_time(audio_decoder) < audio_target_time)
        {
            plm_samples_t *samples = Audio::plm_audio_decode(audio_decoder);
            if (samples)
            {
                audio_decode_callback(nullptr, samples, audio_decode_callback_user_data);
                did_decode = TRUE;
            }
            else {
                decode_audio_failed = TRUE;
            }
        }
    } while (did_decode);
    
    // Did all sources we wanted to decode fail and the demuxer is at the end?
    if (
        (!decode_video || decode_video_failed) && 
        (!decode_audio || decode_audio_failed) &&
        Demux::plm_demux_has_ended(demux)
    ) {
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

    plm_frame_t *frame = Video::plm_video_decode(video_decoder);

    if (frame)
        _time = frame->time;
    else if (Demux::plm_demux_has_ended(demux))
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
    if (!plm_init_decoders())
        return NULL;

    if (!audio_packet_type)
        return NULL;

    plm_samples_t *samples = Audio::plm_audio_decode(audio_decoder);
    if (samples) {
        _time = samples->time;
    }
    else if (Demux::plm_demux_has_ended(demux)) {
        plm_handle_end();
    }
    return samples;
}

void PLM::plm_handle_end() {
    if (_loop) {
        plm_rewind();
    }
    else {
        _has_ended = TRUE;
    }
}

void PLM::plm_read_video_packet(plm_buffer_t *buffer, void *user)
{
    PLM_UNUSED(buffer);
    PLM *plm = (PLM *)(user);
    plm_read_packets(plm, plm->_video_packet_type);
}

void PLM::plm_read_audio_packet(plm_buffer_t *buffer, void *user)
{
    PLM_UNUSED(buffer);
    PLM *plm = (PLM *)(user);
    plm_read_packets(plm, plm->audio_packet_type);
}

void PLM::plm_read_packets(PLM *self, int requested_type)
{
    plm_packet_t *packet;
    while ((packet = Demux::plm_demux_decode(self->demux)))
    {
        if (packet->type == self->_video_packet_type) {
            Buffer::plm_buffer_write(self->video_buffer, packet->data, packet->length);
        }
        else if (packet->type == self->audio_packet_type) {
            Buffer::plm_buffer_write(self->audio_buffer, packet->data, packet->length);
        }

        if (packet->type == requested_type) {
            return;
        }
    }

    if (Demux::plm_demux_has_ended(self->demux))
    {
        if (self->video_buffer)
            Buffer::plm_buffer_signal_end(self->video_buffer);
        
        if (self->audio_buffer)
            Buffer::plm_buffer_signal_end(self->audio_buffer);
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

    double start_time = Demux::plm_demux_get_start_time(demux, type);
    double duration = Demux::plm_demux_get_duration(demux, type);

    if (time < 0) {
        time = 0;
    }
    else if (time > duration) {
        time = duration;
    }
    
    plm_packet_t *packet = Demux::plm_demux_seek(demux, time, type, TRUE);
    if (!packet) {
        return NULL;
    }

    // Disable writing to the audio buffer while decoding video
    int previous_audio_packet_type = audio_packet_type;
    audio_packet_type = 0;

    // Clear video buffer and decode the found packet
    Video::plm_video_rewind(video_decoder);
    Video::plm_video_set_time(video_decoder, packet->pts - start_time);
    Buffer::plm_buffer_write(video_buffer, packet->data, packet->length);
    plm_frame_t *frame = Video::plm_video_decode(video_decoder); 

    // If we want to seek to an exact frame, we have to decode all frames
    // on top of the intra frame we just jumped to.
    if (seek_exact) {
        while (frame && frame->time < time) {
            frame = Video::plm_video_decode(video_decoder);
        }
    }

    // Enable writing to the audio buffer again?
    audio_packet_type = previous_audio_packet_type;

    if (frame) {
        _time = frame->time;
    }

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
        video_decode_callback(nullptr, frame, video_decode_callback_user_data);    

    // If audio is not enabled we are done here.
    if (!audio_packet_type)
        return TRUE;

    // Sync up Audio. This demuxes more packets until the first audio packet
    // with a PTS greater than the current time is found. plm_decode() is then
    // called to decode enough audio data to satisfy the audio_lead_time.

    double start_time = Demux::plm_demux_get_start_time(demux, _video_packet_type);
    Audio::plm_audio_rewind(audio_decoder);

    plm_packet_t *packet = NULL;
    while ((packet = Demux::plm_demux_decode(demux)))
    {
        if (packet->type == _video_packet_type) {
            Buffer::plm_buffer_write(video_buffer, packet->data, packet->length);
        }
        else if (packet->type == audio_packet_type && packet->pts - start_time > _time)
        {
            Audio::plm_audio_set_time(audio_decoder, packet->pts - start_time);
            Buffer::plm_buffer_write(audio_buffer, packet->data, packet->length);
            plm_decode(0);
            break;
        }
    }   
    
    return TRUE;
}

// Create a buffer instance with a filename. Returns NULL if the file could not
// be opened.
plm_buffer_t *Buffer::plm_buffer_create_with_filename(const char *filename)
{
    FILE *fh = fopen(filename, "rb");

    if (!fh)
        return NULL;
    
    return plm_buffer_create_with_file(fh, TRUE);
}

// Create a buffer instance with a file handle. Pass TRUE to close_when_done
// to let plmpeg call fclose() on the handle when plm_destroy() is called.
plm_buffer_t *Buffer::plm_buffer_create_with_file(FILE *fh, int close_when_done)
{
    plm_buffer_t *self = plm_buffer_create_with_capacity(PLM_BUFFER_DEFAULT_SIZE);
    self->fh = fh;
    self->close_when_done = close_when_done;
    self->mode = PLM_BUFFER_MODE_FILE;
    self->discard_read_bytes = TRUE;
    
    fseek(self->fh, 0, SEEK_END);
    self->total_size = ftell(self->fh);
    fseek(self->fh, 0, SEEK_SET);

    plm_buffer_set_load_callback(self, plm_buffer_load_file_callback, NULL);
    return self;
}

// Create a buffer instance with a pointer to memory as source. This assumes
// the whole file is in memory. The bytes are not copied. Pass 1 to 
// free_when_done to let plmpeg call free() on the pointer when plm_destroy() 
// is called.
plm_buffer_t *Buffer::plm_buffer_create_with_memory(
    uint8_t *bytes, size_t length, int free_when_done)
{
    plm_buffer_t *self = (plm_buffer_t *)malloc(sizeof(plm_buffer_t));
    memset(self, 0, sizeof(plm_buffer_t));
    self->capacity = length;
    self->length = length;
    self->total_size = length;
    self->free_when_done = free_when_done;
    self->bytes = bytes;
    self->mode = PLM_BUFFER_MODE_FIXED_MEM;
    self->discard_read_bytes = FALSE;
    return self;
}

// Create an empty buffer with an initial capacity. The buffer will grow
// as needed. Data that has already been read, will be discarded.
plm_buffer_t *Buffer::plm_buffer_create_with_capacity(size_t capacity)
{
    plm_buffer_t *self = (plm_buffer_t *)malloc(sizeof(plm_buffer_t));
    memset(self, 0, sizeof(plm_buffer_t));
    self->capacity = capacity;
    self->free_when_done = TRUE;
    self->bytes = (uint8_t *)malloc(capacity);
    self->mode = PLM_BUFFER_MODE_RING;
    self->discard_read_bytes = TRUE;
    return self;
}

// Create an empty buffer with an initial capacity. The buffer will grow
// as needed. Decoded data will *not* be discarded. This can be used when
// loading a file over the network, without needing to throttle the download. 
// It also allows for seeking in the already loaded data.
plm_buffer_t *Buffer::plm_buffer_create_for_appending(size_t initial_capacity)
{
    plm_buffer_t *self = plm_buffer_create_with_capacity(initial_capacity);
    self->mode = PLM_BUFFER_MODE_APPEND;
    self->discard_read_bytes = FALSE;
    return self;
}

// Destroy a buffer instance and free all data
void Buffer::plm_buffer_destroy(plm_buffer_t *self)
{
    if (self->fh && self->close_when_done)
        fclose(self->fh);
    
    if (self->free_when_done)
        free(self->bytes);
    
    free(self);
}

// Get the total size. For files, this returns the file size. For all other 
// types it returns the number of bytes currently in the buffer.
size_t Buffer::plm_buffer_get_size(plm_buffer_t *self) {
    return (self->mode == PLM_BUFFER_MODE_FILE)
        ? self->total_size
        : self->length;
}

// Get the number of remaining (yet unread) bytes in the buffer. This can be
// useful to throttle writing.
size_t Buffer::plm_buffer_get_remaining(plm_buffer_t *self) {
    return self->length - (self->bit_index >> 3);
}

// Copy data into the buffer. If the data to be written is larger than the 
// available space, the buffer will realloc() with a larger capacity. 
// Returns the number of bytes written. This will always be the same as the
// passed in length, except when the buffer was created _with_memory() for
// which _write() is forbidden.
size_t Buffer::plm_buffer_write(plm_buffer_t *self, uint8_t *bytes, size_t length) {
    if (self->mode == PLM_BUFFER_MODE_FIXED_MEM) {
        return 0;
    }

    if (self->discard_read_bytes) {
        // This should be a ring buffer, but instead it just shifts all unread 
        // data to the beginning of the buffer and appends new data at the end. 
        // Seems to be good enough.

        plm_buffer_discard_read_bytes(self);
        if (self->mode == PLM_BUFFER_MODE_RING) {
            self->total_size = 0;
        }
    }

    // Do we have to resize to fit the new data?
    size_t bytes_available = self->capacity - self->length;
    if (bytes_available < length) {
        size_t new_size = self->capacity;
        do {
            new_size *= 2;
        } while (new_size - self->length < length);
        self->bytes = (uint8_t *)realloc(self->bytes, new_size);
        self->capacity = new_size;
    }

    memcpy(self->bytes + self->length, bytes, length);
    self->length += length;
    self->has_ended = FALSE;
    return length;
}

// Mark the current byte length as the end of this buffer and signal that no 
// more data is expected to be written to it. This function should be called
// just after the last plm_buffer_write().
// For _with_capacity buffers, this is cleared on a plm_buffer_rewind().
void Buffer::plm_buffer_signal_end(plm_buffer_t *self) {
    self->total_size = self->length;
}

// Set a callback that is called whenever the buffer needs more data
void Buffer::plm_buffer_set_load_callback(
    plm_buffer_t *self, plm_buffer_load_callback fp, void *user)
{
    self->load_callback = fp;
    self->load_callback_user_data = user;
}

// Rewind the buffer back to the beginning. When loading from a file handle,
// this also seeks to the beginning of the file.
void Buffer::plm_buffer_rewind(plm_buffer_t *self) {
    Buffer::plm_buffer_seek(self, 0);
}

void Buffer::plm_buffer_seek(plm_buffer_t *self, size_t pos) {
    self->has_ended = FALSE;

    if (self->mode == PLM_BUFFER_MODE_FILE)
    {
        fseek(self->fh, pos, SEEK_SET);
        self->bit_index = 0;
        self->length = 0;
    }
    else if (self->mode == PLM_BUFFER_MODE_RING)
    {
        if (pos != 0) {
            // Seeking to non-0 is forbidden for dynamic-mem buffers
            return; 
        }
        self->bit_index = 0;
        self->length = 0;
        self->total_size = 0;
    }
    else if (pos < self->length) {
        self->bit_index = pos << 3;
    }
}

size_t Buffer::plm_buffer_tell(plm_buffer_t *self) {
    return self->mode == PLM_BUFFER_MODE_FILE
        ? ftell(self->fh) + (self->bit_index >> 3) - self->length
        : self->bit_index >> 3;
}

void Buffer::plm_buffer_discard_read_bytes(plm_buffer_t *self) {
    size_t byte_pos = self->bit_index >> 3;
    if (byte_pos == self->length) {
        self->bit_index = 0;
        self->length = 0;
    }
    else if (byte_pos > 0) {
        memmove(self->bytes, self->bytes + byte_pos, self->length - byte_pos);
        self->bit_index -= byte_pos << 3;
        self->length -= byte_pos;
    }
}

void Buffer::plm_buffer_load_file_callback(plm_buffer_t *self, void *user)
{
    PLM_UNUSED(user);
    
    if (self->discard_read_bytes)
        plm_buffer_discard_read_bytes(self);

    size_t bytes_available = self->capacity - self->length;
    size_t bytes_read = fread(self->bytes + self->length, 1, bytes_available, self->fh);
    self->length += bytes_read;

    if (bytes_read == 0)
        self->has_ended = TRUE;
}

// Get whether the read position of the buffer is at the end and no more data 
// is expected.
int Buffer::plm_buffer_has_ended(plm_buffer_t *self) {
    return self->has_ended;
}

int Buffer::plm_buffer_has(plm_buffer_t *self, size_t count)
{
    if (((self->length << 3) - self->bit_index) >= count)
        return TRUE;

    if (self->load_callback)
    {
        self->load_callback(self, self->load_callback_user_data);
        
        if (((self->length << 3) - self->bit_index) >= count)
            return TRUE;
    }   
    
    if (self->total_size != 0 && self->length == self->total_size)
        self->has_ended = TRUE;
    
    return FALSE;
}

int Buffer::plm_buffer_read(plm_buffer_t *self, int count) {
    if (!plm_buffer_has(self, count)) {
        return 0;
    }

    int value = 0;
    while (count) {
        int current_byte = self->bytes[self->bit_index >> 3];

        int remaining = 8 - (self->bit_index & 7); // Remaining bits in byte
        int read = remaining < count ? remaining : count; // Bits in self run
        int shift = remaining - read;
        int mask = (0xff >> (8 - read));

        value = (value << read) | ((current_byte & (mask << shift)) >> shift);

        self->bit_index += read;
        count -= read;
    }

    return value;
}

void Buffer::plm_buffer_align(plm_buffer_t *self) {
    self->bit_index = ((self->bit_index + 7) >> 3) << 3; // Align to next byte
}

void Buffer::plm_buffer_skip(plm_buffer_t *self, size_t count)
{
    if (plm_buffer_has(self, count)) {
        self->bit_index += count;
    }
}

int Buffer::plm_buffer_skip_bytes(plm_buffer_t *self, uint8_t v)
{
    plm_buffer_align(self);
    int skipped = 0;
    while (plm_buffer_has(self, 8) && self->bytes[self->bit_index >> 3] == v) {
        self->bit_index += 8;
        skipped++;
    }
    return skipped;
}

int Buffer::plm_buffer_next_start_code(plm_buffer_t *self)
{
    plm_buffer_align(self);

    while (plm_buffer_has(self, (5 << 3))) {
        size_t byte_index = (self->bit_index) >> 3;
        if (
            self->bytes[byte_index] == 0x00 &&
            self->bytes[byte_index + 1] == 0x00 &&
            self->bytes[byte_index + 2] == 0x01
        ) {
            self->bit_index = (byte_index + 4) << 3;
            return self->bytes[byte_index + 3];
        }
        self->bit_index += 8;
    }
    return -1;
}

int Buffer::plm_buffer_find_start_code(plm_buffer_t *self, int code) {
    int current = 0;
    while (TRUE) {
        current = plm_buffer_next_start_code(self);
        if (current == code || current == -1) {
            return current;
        }
    }
    return -1;
}

int Buffer::plm_buffer_has_start_code(plm_buffer_t *self, int code) {
    size_t previous_bit_index = self->bit_index;
    int previous_discard_read_bytes = self->discard_read_bytes;
    
    self->discard_read_bytes = FALSE;
    int current = plm_buffer_find_start_code(self, code);

    self->bit_index = previous_bit_index;
    self->discard_read_bytes = previous_discard_read_bytes;
    return current;
}

int Buffer::plm_buffer_peek_non_zero(plm_buffer_t *self, int bit_count)
{
    if (!plm_buffer_has(self, bit_count))
        return FALSE;

    int val = plm_buffer_read(self, bit_count);
    self->bit_index -= bit_count;
    return val != 0;
}

int16_t Buffer::plm_buffer_read_vlc(plm_buffer_t *self, const plm_vlc_t *table)
{
    plm_vlc_t state = {0, 0};
    do {
        state = table[state.index + Buffer::plm_buffer_read(self, 1)];
    } while (state.index > 0);
    return state.value;
}

uint16_t Buffer::plm_buffer_read_vlc_uint(plm_buffer_t *self, const plm_vlc_uint_t *table) {
    return (uint16_t)plm_buffer_read_vlc(self, (const plm_vlc_t *)table);
}



// ----------------------------------------------------------------------------
// plm_demux implementation

static constexpr int PLM_START_PACK = 0xBA;
static constexpr int PLM_START_END = 0xB9;
static constexpr int PLM_START_SYSTEM = 0xBB;

typedef struct plm_demux_t {
    plm_buffer_t *buffer;
    int destroy_buffer_when_done;
    double system_clock_ref;

    size_t last_file_size;
    double last_decoded_pts;
    double start_time;
    double duration;

    int start_code;
    int has_pack_header;
    int has_system_header;
    int has_headers;

    int num_audio_streams;
    int num_video_streams;
    plm_packet_t current_packet;
    plm_packet_t next_packet;
} plm_demux_t;

// Create a demuxer with a plm_buffer as source. This will also attempt to read
// the pack and system headers from the buffer.
plm_demux_t *Demux::plm_demux_create(plm_buffer_t *buffer, int destroy_when_done)
{
    plm_demux_t *self = (plm_demux_t *)malloc(sizeof(plm_demux_t));
    memset(self, 0, sizeof(plm_demux_t));

    self->buffer = buffer;
    self->destroy_buffer_when_done = destroy_when_done;

    self->start_time = PLM_PACKET_INVALID_TS;
    self->duration = PLM_PACKET_INVALID_TS;
    self->start_code = -1;

    plm_demux_has_headers(self);
    return self;
}

// Destroy a demuxer and free all data.
void Demux::plm_demux_destroy(plm_demux_t *self)
{
    if (self->destroy_buffer_when_done)
        Buffer::plm_buffer_destroy(self->buffer);
    
    free(self);
}

// Returns TRUE/FALSE whether pack and system headers have been found. This will
// attempt to read the headers if non are present yet.
int Demux::plm_demux_has_headers(plm_demux_t *self) 
{
    if (self->has_headers) {
        return TRUE;
    }

    // Decode pack header
    if (!self->has_pack_header)
    {
        if (
            self->start_code != PLM_START_PACK &&
            Buffer::plm_buffer_find_start_code(self->buffer, PLM_START_PACK) == -1
        ) {
            return FALSE;
        }

        self->start_code = PLM_START_PACK;

        if (!Buffer::plm_buffer_has(self->buffer, 64))
            return FALSE;
        
        self->start_code = -1;

        if (Buffer::plm_buffer_read(self->buffer, 4) != 0x02)
            return FALSE;

        self->system_clock_ref = plm_demux_decode_time(self);
        Buffer::plm_buffer_skip(self->buffer, 1);
        Buffer::plm_buffer_skip(self->buffer, 22); // mux_rate * 50
        Buffer::plm_buffer_skip(self->buffer, 1);

        self->has_pack_header = TRUE;
    }

    // Decode system header
    if (!self->has_system_header)
    {
        if (self->start_code != PLM_START_SYSTEM &&
            Buffer::plm_buffer_find_start_code(self->buffer, PLM_START_SYSTEM) == -1
        ) {
            return FALSE;
        }

        self->start_code = PLM_START_SYSTEM;
        if (!Buffer::plm_buffer_has(self->buffer, 56)) {
            return FALSE;
        }
        self->start_code = -1;

        Buffer::plm_buffer_skip(self->buffer, 16); // header_length
        Buffer::plm_buffer_skip(self->buffer, 24); // rate bound
        self->num_audio_streams = Buffer::plm_buffer_read(self->buffer, 6);
        Buffer::plm_buffer_skip(self->buffer, 5); // misc flags
        self->num_video_streams = Buffer::plm_buffer_read(self->buffer, 5);

        self->has_system_header = TRUE;
    }

    self->has_headers = TRUE;
    return TRUE;
}

// Returns the number of video streams found in the system header. This will
// attempt to read the system header if non is present yet.
int Demux::plm_demux_get_num_video_streams(plm_demux_t *self) {
    return plm_demux_has_headers(self) ? self->num_video_streams : 0;
}

// Returns the number of audio streams found in the system header. This will
// attempt to read the system header if non is present yet.
int Demux::plm_demux_get_num_audio_streams(plm_demux_t *self) {
    return plm_demux_has_headers(self) ? self->num_audio_streams : 0;
}

// Rewind the internal buffer. See plm_buffer_rewind().
void Demux::plm_demux_rewind(plm_demux_t *self)
{
    Buffer::plm_buffer_rewind(self->buffer);
    self->current_packet.length = 0;
    self->next_packet.length = 0;
    self->start_code = -1;
}

// Get whether the file has ended. This will be cleared on seeking or rewind.
int Demux::plm_demux_has_ended(plm_demux_t *self) {
    return Buffer::plm_buffer_has_ended(self->buffer);
}

void Demux::plm_demux_buffer_seek(plm_demux_t *self, size_t pos)
{
    Buffer::plm_buffer_seek(self->buffer, pos);
    self->current_packet.length = 0;
    self->next_packet.length = 0;
    self->start_code = -1;
}

// Get the PTS of the first packet of this type. Returns PLM_PACKET_INVALID_TS
// if not packet of this packet type can be found.
double Demux::plm_demux_get_start_time(plm_demux_t *self, int type)
{
    if (self->start_time != PLM_PACKET_INVALID_TS)
        return self->start_time;

    int previous_pos = Buffer::plm_buffer_tell(self->buffer);
    int previous_start_code = self->start_code;
    
    // Find first video PTS
    plm_demux_rewind(self);
    do {
        plm_packet_t *packet = plm_demux_decode(self);
        if (!packet) {
            break;
        }
        if (packet->type == type) {
            self->start_time = packet->pts;
        }
    } while (self->start_time == PLM_PACKET_INVALID_TS);

    plm_demux_buffer_seek(self, previous_pos);
    self->start_code = previous_start_code;
    return self->start_time;
}

// Get the duration for the specified packet type - i.e. the span between the
// the first PTS and the last PTS in the data source. This only makes sense when
// the underlying data source is a file or fixed memory.
double Demux::plm_demux_get_duration(plm_demux_t *self, int type)
{
    size_t file_size = Buffer::plm_buffer_get_size(self->buffer);

    if (
        self->duration != PLM_PACKET_INVALID_TS &&
        self->last_file_size == file_size
    ) {
        return self->duration;
    }

    size_t previous_pos = Buffer::plm_buffer_tell(self->buffer);
    int previous_start_code = self->start_code;
    
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
        plm_demux_buffer_seek(self, seek_pos);
        self->current_packet.length = 0;

        double last_pts = PLM_PACKET_INVALID_TS;
        plm_packet_t *packet = NULL;
        while ((packet = plm_demux_decode(self))) {
            if (packet->pts != PLM_PACKET_INVALID_TS && packet->type == type) {
                last_pts = packet->pts;
            }
        }
        if (last_pts != PLM_PACKET_INVALID_TS) {
            self->duration = last_pts - plm_demux_get_start_time(self, type);
            break;
        }
    }

    plm_demux_buffer_seek(self, previous_pos);
    self->start_code = previous_start_code;
    self->last_file_size = file_size;
    return self->duration;
}

// Seek to a packet of the specified type with a PTS just before specified time.
// If force_intra is TRUE, only packets containing an intra frame will be 
// considered - this only makes sense when the type is PLM_DEMUX_PACKET_VIDEO_1.
// Note that the specified time is considered 0-based, regardless of the first 
// PTS in the data source.
plm_packet_t *Demux::plm_demux_seek(
    plm_demux_t *self, double seek_time, int type, int force_intra)
{
    if (!plm_demux_has_headers(self)) {
        return NULL;
    }

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

    double duration = plm_demux_get_duration(self, type);
    long file_size = Buffer::plm_buffer_get_size(self->buffer);
    long byterate = file_size / duration;

    double cur_time = self->last_decoded_pts;
    double scan_span = 1;

    if (seek_time > duration) {
        seek_time = duration;
    }
    else if (seek_time < 0) {
        seek_time = 0;
    }
    seek_time += self->start_time;

    for (int retry = 0; retry < 32; retry++) {
        int found_packet_with_pts = FALSE;
        int found_packet_in_range = FALSE;
        long last_valid_packet_start = -1;
        double first_packet_time = PLM_PACKET_INVALID_TS;

        long cur_pos = Buffer::plm_buffer_tell(self->buffer);

        // Estimate byte offset and jump to it.
        long offset = (seek_time - cur_time - scan_span) * byterate;
        long seek_pos = cur_pos + offset;
        if (seek_pos < 0) {
            seek_pos = 0;
        }
        else if (seek_pos > file_size - 256) {
            seek_pos = file_size - 256;
        }

        plm_demux_buffer_seek(self, seek_pos);

        // Scan through all packets up to the seek_time to find the last packet
        // containing an intra frame.
        while (Buffer::plm_buffer_find_start_code(self->buffer, type) != -1)
        {
            long packet_start = Buffer::plm_buffer_tell(self->buffer);
            plm_packet_t *packet = plm_demux_decode_packet(self, type);

            // Skip packet if it has no PTS
            if (!packet || packet->pts == PLM_PACKET_INVALID_TS) {
                continue;
            }

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
                        packet->data[i] == 0x00 &&
                        packet->data[i + 1] == 0x00 &&
                        packet->data[i + 2] == 0x01 &&
                        packet->data[i + 3] == 0x00
                    ) {
                        // Bits 11--13 in the picture header contain the frame 
                        // type, where 1=Intra
                        if ((packet->data[i + 5] & 0x38) == 8) {
                            last_valid_packet_start = packet_start;
                        }
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
            plm_demux_buffer_seek(self, last_valid_packet_start);
            return plm_demux_decode_packet(self, type);
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
plm_packet_t *Demux::plm_demux_decode(plm_demux_t *self) {
    if (!plm_demux_has_headers(self)) {
        return NULL;
    }

    if (self->current_packet.length)
    {
        size_t bits_till_next_packet = self->current_packet.length << 3;
        if (!Buffer::plm_buffer_has(self->buffer, bits_till_next_packet)) {
            return NULL;
        }
        Buffer::plm_buffer_skip(self->buffer, bits_till_next_packet);
        self->current_packet.length = 0;
    }

    // Pending packet waiting for data?
    if (self->next_packet.length) {
        return plm_demux_get_packet(self);
    }

    // Pending packet waiting for header?
    if (self->start_code != -1) {
        return plm_demux_decode_packet(self, self->start_code);
    }

    do {
        self->start_code = Buffer::plm_buffer_next_start_code(self->buffer);
        if (
            self->start_code == PLM_DEMUX_PACKET_VIDEO_1 || 
            self->start_code == PLM_DEMUX_PACKET_PRIVATE || (
                self->start_code >= PLM_DEMUX_PACKET_AUDIO_1 && 
                self->start_code <= PLM_DEMUX_PACKET_AUDIO_4
            )
        ) {
            return plm_demux_decode_packet(self, self->start_code);
        }
    } while (self->start_code != -1);

    return NULL;
}

double Demux::plm_demux_decode_time(plm_demux_t *self)
{
    int64_t clock = Buffer::plm_buffer_read(self->buffer, 3) << 30;
    Buffer::plm_buffer_skip(self->buffer, 1);
    clock |= Buffer::plm_buffer_read(self->buffer, 15) << 15;
    Buffer::plm_buffer_skip(self->buffer, 1);
    clock |= Buffer::plm_buffer_read(self->buffer, 15);
    Buffer::plm_buffer_skip(self->buffer, 1);
    return (double)clock / 90000.0;
}

plm_packet_t *Demux::plm_demux_decode_packet(plm_demux_t *self, int type) {
    if (!Buffer::plm_buffer_has(self->buffer, 16 << 3)) {
        return NULL;
    }

    self->start_code = -1;

    self->next_packet.type = type;
    self->next_packet.length = Buffer::plm_buffer_read(self->buffer, 16);
    self->next_packet.length -= Buffer::plm_buffer_skip_bytes(self->buffer, 0xff); // stuffing

    // skip P-STD
    if (Buffer::plm_buffer_read(self->buffer, 2) == 0x01) {
        Buffer::plm_buffer_skip(self->buffer, 16);
        self->next_packet.length -= 2;
    }

    int pts_dts_marker = Buffer::plm_buffer_read(self->buffer, 2);

    if (pts_dts_marker == 0x03)
    {
        self->next_packet.pts = plm_demux_decode_time(self);
        self->last_decoded_pts = self->next_packet.pts;
        Buffer::plm_buffer_skip(self->buffer, 40); // skip dts
        self->next_packet.length -= 10;
    }
    else if (pts_dts_marker == 0x02) {
        self->next_packet.pts = plm_demux_decode_time(self);
        self->last_decoded_pts = self->next_packet.pts;
        self->next_packet.length -= 5;
    }
    else if (pts_dts_marker == 0x00) {
        self->next_packet.pts = PLM_PACKET_INVALID_TS;
        Buffer::plm_buffer_skip(self->buffer, 4);
        self->next_packet.length -= 1;
    }
    else {
        return NULL; // invalid
    }
    
    return plm_demux_get_packet(self);
}

plm_packet_t *Demux::plm_demux_get_packet(plm_demux_t *self)
{
    if (!Buffer::plm_buffer_has(self->buffer, self->next_packet.length << 3))
        return NULL;

    self->current_packet.data = self->buffer->bytes + (self->buffer->bit_index >> 3);
    self->current_packet.length = self->next_packet.length;
    self->current_packet.type = self->next_packet.type;
    self->current_packet.pts = self->next_packet.pts;

    self->next_packet.length = 0;
    return &self->current_packet;
}

