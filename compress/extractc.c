//Usage: ./extractc archive.Z | ./lzw > archive

#include <stdio.h>
#include <assert.h>

int main(int argc, char **argv)
{
    FILE *fp = fopen(argv[1], "r");
    assert(fgetc(fp) == 0x1f && fgetc(fp) == 0x9d);
    int c = fgetc(fp);
    assert(c != -1 && c & 0x80);
    unsigned bitdepth = c & 0x7f;
    assert(bitdepth >= 9 && bitdepth <= 16);
    char buf[20];
start_block:
    for (unsigned nbits = 9;; ++nbits)
    {
        for (unsigned i = 0, ncodes; i < 1U << nbits - 1 || nbits == bitdepth;)
        {
            if ((ncodes = fread(buf, 1, nbits, fp) * 8 / nbits) == 0)
                return 0;

            for (unsigned j = 0; j < ncodes; ++j, ++i)
            {
                unsigned *window = (unsigned *)(buf + (nbits * j) / 8);
                unsigned code = *window >> j * (nbits - 8) % 8 & (1 << nbits) - 1;
                fwrite(&code, 2, 1, stdout);
                //printf("%u\r\n", code);

                if (code == 256)
                    goto start_block;
            }
        }
    }
}


