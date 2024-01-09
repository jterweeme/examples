//Usage: ./zcatc archive.Z > archive

#include <stdio.h>
#include <assert.h>

static int decompress_block(FILE *in, FILE *out, unsigned bitdepth)
{
    char buf[20], htab[1 << bitdepth], stack[1000];
    unsigned short codes[1 << bitdepth];
    unsigned pos = 256, oldcode = 0, finchar = 0;

    for (unsigned nbits = 9; nbits <= bitdepth; ++nbits)
    {
        for (unsigned i = 0, ncodes; i < 1U << nbits - 1 || nbits == bitdepth;)
        {
            if ((ncodes = fread(buf, 1, nbits, in) * 8 / nbits) == 0)
                return 0;

            for (unsigned j = 0, bits = 0; ncodes--; ++j, ++i, bits += nbits)
            {
                unsigned *window = (unsigned *)(buf + bits / 8);
                unsigned newcode, c, pstack = sizeof(stack);
                newcode = c = *window >> j * (nbits - 8) % 8 & (1 << nbits) - 1;
                assert(c <= pos);

                if (c == 256)
                    return 1;

                if (c == pos)
                    stack[--pstack] = finchar, c = oldcode;

                for (; c >= 256; c = codes[c])
                    stack[--pstack] = htab[c];

                putchar(finchar = c);
                fwrite(stack + pstack, 1, sizeof(stack) - pstack, out);

                if (pos < sizeof(htab))
                    codes[pos] = oldcode, htab[pos] = finchar, ++pos;

                oldcode = newcode;
            }
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    FILE *fp = argc > 1 ? fopen(argv[1], "r") : stdin;
    assert(fgetc(fp) == 0x1f && fgetc(fp) == 0x9d);
    int c = fgetc(fp);
    assert(c != -1 && c & 0x80);
    unsigned bitdepth = c & 0x7f;
    assert(bitdepth >= 9 && bitdepth <= 16);

    while (decompress_block(fp, stdout, bitdepth))
        continue;

    return 0;
}


