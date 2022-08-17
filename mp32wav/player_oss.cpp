// minimp3 example player application for Linux/OSS
// this file is public domain -- do with it whatever you want!
#include "minimp3.h"
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>

struct SWavHeader
{
    char chunkID[4];
    uint32_t chunkSize;
    char format[4];
    char subchunk1ID[4];
    uint32_t subchunk1Size;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char subchunk2ID[4];
    uint32_t subchunk2Size;
};

class WavHeader
{
private:
    SWavHeader _header;
public:
    WavHeader();
    void write(std::ostream &os);
    void rate(uint32_t val);
};

class Toolbox
{
public:
    static void writeWLE(std::ostream &os, uint16_t w);
    static void writeDwLE(std::ostream &os, uint32_t dw);
};

WavHeader::WavHeader()
{
    strncpy(_header.chunkID, "RIFF", 4);
    _header.chunkSize = 0;
    strncpy(_header.format, "WAVE", 4);
    strncpy(_header.subchunk1ID, "fmt ", 4);
    _header.subchunk1Size = 16;
    _header.audioFormat = 1;
    _header.numChannels = 2;
    _header.sampleRate = 0;
    _header.byteRate = 0;
    _header.blockAlign = 4;
    _header.bitsPerSample = 16;
    strncpy(_header.subchunk2ID, "data", 4);
    _header.subchunk2Size = 0;
}

void WavHeader::rate(uint32_t val)
{
    _header.sampleRate = val;
    _header.byteRate = val << 2;
}

void WavHeader::write(std::ostream &os)
{
    os << "RIFF";
    Toolbox::writeDwLE(os, _header.chunkSize);
    os << "WAVE";
    os << "fmt ";
    Toolbox::writeDwLE(os, _header.subchunk1Size);
    Toolbox::writeWLE(os, _header.audioFormat);
    Toolbox::writeWLE(os, _header.numChannels);
    Toolbox::writeDwLE(os, _header.sampleRate);
    Toolbox::writeDwLE(os, _header.byteRate);
    Toolbox::writeWLE(os, _header.blockAlign);
    Toolbox::writeWLE(os, _header.bitsPerSample);
    os << "data";
    Toolbox::writeDwLE(os, _header.subchunk2Size);
}

void Toolbox::writeWLE(std::ostream &os, uint16_t w)
{
    os.put(w >> 0 & 0xff);
    os.put(w >> 8 & 0xff);
}

void Toolbox::writeDwLE(std::ostream &os, uint32_t dw)
{
    os.put(dw >> 0 & 0xff);
    os.put(dw >> 8 & 0xff);
    os.put(dw >> 16 & 0xff);
    os.put(dw >> 24 & 0xff);
}

#define out(text) write(1, (const void *) text, strlen(text))

int main(int argc, char *argv[])
{
    mp3_decoder_t mp3;
    mp3_info_t info;
    int fd, pcm;
    void *file_data;
    unsigned char *stream_pos;
    signed short sample_buf[MP3_MAX_SAMPLES_PER_FRAME];
    int bytes_left;
    int frame_size;
    int value;

    //out("minimp3 -- a small MPEG-1 Audio Layer III player based on ffmpeg\n\n");
    if (argc < 2) {
        out("Error: no input file specified!\n");
        return 1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        out("Error: cannot open `");
        out(argv[1]);
        out("'!\n");
        return 1;
    }
    
    bytes_left = lseek(fd, 0, SEEK_END);    
    file_data = mmap(0, bytes_left, PROT_READ, MAP_PRIVATE, fd, 0);
    stream_pos = (unsigned char *) file_data;
    bytes_left -= 100;
    //out("Now Playing: ");
    //out(argv[1]);

    mp3 = mp3_create();
    frame_size = mp3_decode((void**)mp3, stream_pos, bytes_left, sample_buf, &info);
    if (!frame_size) {
        //out("\nError: not a valid MP3 audio file!\n");
        return 1;
    }
    
    WavHeader header;
    header.rate(info.sample_rate);
    header.write(std::cout);

    while ((bytes_left >= 0) && (frame_size > 0))
    {
        stream_pos += frame_size;
        bytes_left -= frame_size;

        for (size_t i = 0; i < info.audio_bytes / 2; ++i)
            Toolbox::writeWLE(std::cout, sample_buf[i]);

        frame_size = mp3_decode((void**)mp3, stream_pos, bytes_left, sample_buf, NULL);
    }

    return 0;
}
