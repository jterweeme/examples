#include <stdio.h>
#include <string.h>
#include <stdint.h>

int main()
{
    FILE *os = stdout;
    fputc(0x1f, os);
    fputc(0x9d, os);
    fputc(0x90, os);
    unsigned cnt = 0, nbits = 9;
    const unsigned bitdepth = 16;
    char buf[20] = {0};
    
    for (uint16_t c; (fread(&c, 1, 2, stdin)) == 2;)
    {
        unsigned *window = (unsigned *)(buf + nbits * (cnt % 8) / 8);
        *window |= c << (cnt % 8) * (nbits - 8) % 8;

        if (++cnt % 8 == 0 || c == 256)
        {
            fwrite(buf, 1, nbits, os);
            memset(buf, 0, sizeof(buf));
        }

        if (c == 256)
            nbits = 9, cnt = 0;
        
        if (nbits != bitdepth && cnt == 1U << nbits - 1)
            ++nbits, cnt = 0;
    }

    unsigned num = (cnt % 8) * nbits;
    fwrite(buf, 1, num / 8 + (num % 8 ? 1 : 0), os);
    fflush(os);
    return 0;
}


