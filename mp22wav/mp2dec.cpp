// kjmp2 example application: decodes .mp2 into .wav
// this file is public domain -- do with it whatever you want!

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include "kjmp2.h"


class Toolbox
{
public:
    static void writeWLE(char *buf, uint16_t w);
    static void writeDwLE(char *buf, uint32_t dw);
};

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

class CMain
{
private:
    static constexpr uint32_t KJMP2_SAMPLES_PER_FRAME = 1152;
    static constexpr uint32_t KJMP2_MAX_FRAME_SIZE = 1440;
    static constexpr uint32_t MAX_BUFSIZE = 1000 * KJMP2_MAX_FRAME_SIZE;
public:
    int run(const char *infn, const char *outfn);
};

int main(int argc, char **argv)
{
    CMain inst;

    if (argc < 2)
    {
        printf("Usage: %s <input.mp2> [<output.wav>]\n", argv[0]);
        return 2;
    }

    char *outname;

    if (argc > 2)
    {
        outname = argv[2];
    }
    else
    {
        char *dot = strrchr(argv[1], '.');
        int size = dot ? (dot - argv[1]) : (int) strlen(argv[1]);
        outname = (char *)(malloc(size + 5));
        if (!outname) { return -1; }
        memcpy((void*) outname, (const void*)argv[1], size);
        strcpy(&outname[size], ".wav");
    }

    return inst.run(argv[1], outname);
}

int CMain::run(const char *infn, const char *outname)
{
    uint8_t buffer[MAX_BUFSIZE];
    signed short samples[KJMP2_SAMPLES_PER_FRAME * 2];
    kjmp2_context_t mp2;
    FILE *fin = fopen(infn, "rb");

    if (!fin)
    {
        printf("Could not open input file %s!\n", infn);
        return 1;
    }

    int bufsize = (int) fread((void*) buffer, 1, MAX_BUFSIZE, fin);
    int bufpos = 0;
    int in_offset = 0;

    int rate = (bufsize > 4) ? kjmp2_get_sample_rate(buffer) : 0;

    if (!rate)
    {
        printf("Input is not a valid MP2 audio file, exiting.\n");
        fclose(fin);
        return 1;
    }

    FILE *fout = fopen(outname, "wb");

    if (!fout)
    {
        printf("Could not open output file %s!\n", outname);
        return 1;
    }

    Toolbox t;


    uint8_t header[44];
    strncpy((char *)header + 0, "RIFF", 4);
    t.writeDwLE((char *)(header + 4), 0);
    strncpy((char *)header + 8, "WAVE", 4);
    strncpy((char *)header + 12, "fmt ", 4);
    t.writeDwLE((char *)(header + 16), 16);
    t.writeWLE((char *)(header + 20), 1);
    t.writeWLE((char *)(header + 22), 2);
    t.writeDwLE((char *)(header + 24), rate);
    rate <<= 2;
    t.writeDwLE((char *)(header + 28), rate);
    t.writeWLE((char *)(header + 32), 4);
    t.writeWLE((char *)(header + 34), 16);
    strncpy((char *)header + 36, "data", 4);
    t.writeDwLE((char *)(header + 40), 0);

    //write wav header to file
    fwrite((const void*) header, 44, 1, fout);

    printf("Decoding %s into %s ...\n", infn, outname);
    int out_bytes = 0;
    int desync = 0;
    int eof = 0;
    kjmp2_init(&mp2);

    while (!eof || (bufsize > 4))
    {
        int bytes;

        if (!eof && (bufsize < int(KJMP2_MAX_FRAME_SIZE)))
        {
            memcpy((void*) buffer, &buffer[bufpos], bufsize);
            bufpos = 0;
            in_offset += bufsize;
            bytes = (int) fread((void*) &buffer[bufsize], 1, MAX_BUFSIZE - bufsize, fin);
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
                    printf("Stream error detected at file offset %d.\n", in_offset + bufpos);
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

    t.writeDwLE((char *)(header + 40), out_bytes);
    out_bytes += 36;
    t.writeDwLE((char *)(header + 4), out_bytes);

    //rewrite WAV header
    fseek(fout, 0, SEEK_SET);
    fwrite((const void*) header, 44, 1, fout);

    fclose(fout);
    fclose(fin);
    printf("Done.\n");
    return 0;
}
