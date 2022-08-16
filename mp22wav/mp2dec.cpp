// kjmp2 example application: decodes .mp2 into .wav
// this file is public domain -- do with it whatever you want!

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <fcntl.h>
#include <string>
#include "kjmp2.h"

class Toolbox
{
public:
    static void writeWLE(char *buf, uint16_t w);
    static void writeDwLE(char *buf, uint32_t dw);
};

class COptions
{
private:
    bool _stdinput = false;
    std::string _ifn;
public:
    void parse(int argc, char **argv);
    bool stdinput() const;
    std::string ifn() const;
};

class CWavHeader
{
private:
    int _rate;
public:
    void rate(int val);
    void write(FILE *fp) const;
};

class CMain
{
private:
    static constexpr uint32_t KJMP2_SAMPLES_PER_FRAME = 1152;
    static constexpr uint32_t KJMP2_MAX_FRAME_SIZE = 1440;
    static constexpr uint32_t MAX_BUFSIZE = 1000 * KJMP2_MAX_FRAME_SIZE;
public:
    int run(FILE *fin, FILE *outfile);
};

void CWavHeader::rate(int val)
{
    _rate = val;
}

void CWavHeader::write(FILE *fp) const
{
    Toolbox t;


    uint8_t header[44];
    strncpy((char *)header + 0, "RIFF", 4);
    t.writeDwLE((char *)(header + 4), 0);
    strncpy((char *)header + 8, "WAVE", 4);
    strncpy((char *)header + 12, "fmt ", 4);
    t.writeDwLE((char *)(header + 16), 16);
    t.writeWLE((char *)(header + 20), 1);
    t.writeWLE((char *)(header + 22), 2);
    t.writeDwLE((char *)(header + 24), _rate);
    t.writeDwLE((char *)(header + 28), _rate << 2);
    t.writeWLE((char *)(header + 32), 4);
    t.writeWLE((char *)(header + 34), 16);
    strncpy((char *)header + 36, "data", 4);
    t.writeDwLE((char *)(header + 40), 0);

    //write wav header to file
    fwrite((const void*) header, 44, 1, fp);
}

void Toolbox::writeWLE(char *buf, uint16_t w)
{
    buf[0] = char(w >> 0 & 0xff);
    buf[1] = char(w >> 8 & 0xff);
}

void Toolbox::writeDwLE(char *buf, uint32_t dw)
{
    buf[0] = char(dw >>  0 & 0xff);
    buf[1] = char(dw >>  8 & 0xff);
    buf[2] = char(dw >> 16 & 0xff);
    buf[3] = char(dw >> 24 & 0xff);
}

void COptions::parse(int argc, char **argv)
{
    if (argc < 2)
    {
        _stdinput = true;
    }
    else
    {
        _ifn = argv[1];
    }
}

bool COptions::stdinput() const
{
    return _stdinput;
}

std::string COptions::ifn() const
{
    return _ifn;
}

int main(int argc, char **argv)
{
    COptions opts;
    opts.parse(argc, argv);
    CMain inst;
    FILE *fin;

    if (opts.stdinput())
    {
        fin = stdin;
#ifdef _WIN32
        setmode(fileno(stdin), O_BINARY);
#endif
    }
    else
    {
        fin = fopen(opts.ifn().c_str(), "rb");
    }
    int ret = inst.run(fin, stdout);
    return ret;
}

int CMain::run(FILE *fin, FILE *fout)
{
    uint8_t buffer[MAX_BUFSIZE];
    signed short samples[KJMP2_SAMPLES_PER_FRAME * 2];
    kjmp2_context_t mp2;
    int bufsize = (int) fread((void*) buffer, 1, MAX_BUFSIZE, fin);
    int bufpos = 0;
    int in_offset = 0;

    int rate = (bufsize > 4) ? kjmp2_get_sample_rate(buffer) : 0;

    if (!rate)
    {
        fprintf(stderr, "Input is not a valid MP2 audio file, exiting.\n");
        fclose(fin);
        return 1;
    }
#ifdef _WIN32
    setmode(fileno(stdout), O_BINARY);
#endif
    if (!fout)
    {
        fprintf(stderr, "Could not open output file %s!\n", "out.wav");
        return 1;
    }

    CWavHeader h;
    h.rate(rate);
    h.write(fout);

    //printf("Decoding %s into %s ...\n", infn, outname);
    int out_bytes = 0;
    int desync = 0;
    int eof = 0;
    kjmp2_init(&mp2);

    while (!eof || (bufsize > 4))
    {
        int bytes;

        if (!eof && (bufsize < int(KJMP2_MAX_FRAME_SIZE)))
        {
            memcpy(buffer, buffer + bufpos, bufsize);
            bufpos = 0;
            in_offset += bufsize;
            bytes = (int) fread(buffer + bufsize, 1, MAX_BUFSIZE - bufsize, fin);

            if (bytes > 0) {
                bufsize += bytes;
            } else {
                eof = 1;
            }
        }
        else
        {
            bytes = (int) kjmp2_decode_frame(&mp2, &buffer[bufpos], samples);

            if ((bytes < 4) || (bytes > int(KJMP2_MAX_FRAME_SIZE)) || (bytes > bufsize))
            {
                if (!desync)
                {
                    fprintf(stderr, "Stream error detected at file offset %d.\n", in_offset + bufpos);
                }

                desync = bytes = 1;
            }
            else
            {
                out_bytes += (int) fwrite((const void*) samples, 1, KJMP2_SAMPLES_PER_FRAME * 4, fout);
                desync = 0;
            }

            bufsize -= bytes;
            bufpos += bytes;
        }
    }
#if 0
    if (false)
    {
        t.writeDwLE((char *)(header + 40), out_bytes);
        out_bytes += 36;
        t.writeDwLE((char *)(header + 4), out_bytes);

        //rewrite WAV header
        fseek(fout, 0, SEEK_SET);
        fwrite((const void*) header, 44, 1, fout);
    }
#endif
    fflush(fout);
    fclose(fout);
    fclose(fin);
    fprintf(stderr, "Done.\n");
    return 0;
}
