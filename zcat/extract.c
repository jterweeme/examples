#include <stdio.h>
#include <assert.h>
#include <stdint.h>

int main(int argc, char **argv)
{
    FILE *fp = fopen(argv[1], "r");
    assert(fgetc(fp) == 0x1f);
    assert(fgetc(fp) == 0x9d);
    int c = fgetc(fp);
    assert(c != -1 && c & 0x80);
    unsigned maxbits = c & 0x7f;
    assert(maxbits >= 9 && maxbits <= 16);
    uint8_t buf[512];

    for (unsigned ncodes, ncodes2=0,nbits=9; (ncodes = fread(buf, 1, nbits, fp) * 8 / nbits) > 0;)
    {
        for (unsigned i = 0, bits = 0; ncodes--; ++i)
        {
            unsigned shift = (i * (nbits - 8)) % 8;
            unsigned *window = (unsigned *)(buf + bits / 8);
            unsigned code = *window >> shift & (1 << nbits) - 1;
            bits += nbits;
            ++ncodes2;
            printf("%u\r\n", code);

            if (code == 256)
            {
                nbits = 9, ncodes2 = 0;
                break;
            }
        }

        if (ncodes2 == 1U << nbits - 1U && nbits != maxbits)
            ++nbits, ncodes2 = 0;
    }
}


