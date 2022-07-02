// kjmp2 example application: decodes .mp2 into .wav
// this file is public domain -- do with it whatever you want!

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

#include "kjmp2.h"

#define set_le32(p, x) \
        do { \
            (p)[0] =  (x)        & 0xFF; \
            (p)[1] = ((x) >>  8) & 0xFF; \
            (p)[2] = ((x) >> 16) & 0xFF; \
            (p)[3] = ((x) >> 24) & 0xFF; \
        } while (0)

class CMain
{
private:
    static constexpr uint32_t KJMP2_SAMPLES_PER_FRAME = 1152;
    static constexpr uint32_t KJMP2_MAX_FRAME_SIZE = 1440;
    static constexpr uint32_t MAX_BUFSIZE = 1000 * KJMP2_MAX_FRAME_SIZE;
public:
    int run(int argc, char **argv);
};

int main(int argc, char **argv)
{
    CMain inst;

    if (argc < 2)
    {
        printf("Usage: %s <input.mp2> [<output.wav>]\n", argv[0]);
        return 2;
    }

    return inst.run(argc, argv);
}

int CMain::run(int argc, char **argv)
{
    char *outname;
    uint8_t buffer[MAX_BUFSIZE];
    signed short samples[KJMP2_SAMPLES_PER_FRAME * 2];
    kjmp2_context_t mp2;
    FILE *fin = fopen(argv[1], "rb");

    if (!fin)
    {
        printf("Could not open input file %s!\n", argv[1]);
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

    if (argc > 2) {
        outname = argv[2];
    } else {
        char *dot = strrchr(argv[1], '.');
        int size = dot ? (dot - argv[1]) : (int) strlen(argv[1]);
        outname = (char *)(malloc(size + 5));
        if (!outname) { return -1; }
        memcpy((void*) outname, (const void*) argv[1], size);
        strcpy(&outname[size], ".wav");
    }

    FILE *fout = fopen(outname, "wb");

    if (!fout)
    {
        printf("Could not open output file %s!\n", argv[1]);
        return 1;
    }

    {
        uint8_t header[44] = {
            /*  0 */  'R', 'I', 'F', 'F',
            /*  4 */  0,0,0,0,  /* cksize */
            /*  8 */  'W', 'A', 'V', 'E',
            /* 12 */  'f', 'm', 't', ' ',
            /* 16 */  16,0,0,0,  /* cksize */
            /* 20 */  1,0,       /* wFormatTag = 1 (PCM) */
            /* 22 */  2,0,       /* nChannels = 2 */
            /* 24 */  0,0,0,0,   /* nSamplesPerSec */
            /* 28 */  0,0,0,0,   /* nAvgBytesPerSec */
            /* 32 */  4,0,       /* nBlockAlign = 4 */
            /* 34 */  16,0,      /* wBitsPerSample */
            /* 36 */  'd', 'a', 't', 'a',
            /* 40 */  0,0,0,0,   /* cksize */
        };


        set_le32(&header[24], rate);
        rate <<= 2;
        set_le32(&header[28], rate);
        fwrite((const void*) header, 44, 1, fout);

        printf("Decoding %s into %s ...\n", argv[1], outname);
        int out_bytes = 0;
        int desync = 0;
        int eof = 0;
        kjmp2_init(&mp2);

        while (!eof || (bufsize > 4))
        {
            int bytes;

            if (!eof && (bufsize < KJMP2_MAX_FRAME_SIZE))
            {
                memcpy((void*) buffer, (const void*) &buffer[bufpos], bufsize);
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

                if ((bytes < 4) || (bytes > KJMP2_MAX_FRAME_SIZE) || (bytes > bufsize))
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

        fseek(fout, 0, SEEK_SET);
        set_le32(&header[40], out_bytes);
        out_bytes += 36;
        set_le32(&header[4], out_bytes);
        (void) fwrite((const void*) header, 44, 1, fout);
    }
    fclose(fout);
    fclose(fin);
    printf("Done.\n");
    return 0;
}
