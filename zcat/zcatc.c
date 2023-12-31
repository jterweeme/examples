//Usage: ./zcatc archive.Z > archive

#include <stdio.h>
#include <assert.h>

int main(int argc, char **argv) {
    FILE *fp = argc > 1 ? fopen(argv[1], "r") : stdin;
    assert(fgetc(fp) == 0x1f && fgetc(fp) == 0x9d);
    int c = fgetc(fp);
    assert(c != -1 && c & 0x80);
    unsigned maxbits = c & 0x7f, oldcode = 0, pos = 256, ncodes, ncodes2 = 0, nbits = 9;
    assert(maxbits >= 9 && maxbits <= 16);
    char buf[20], finchar = 0, htab[1 << 16], stack[1000];
    unsigned short codes[1 << 16];

    while ((ncodes = fread(buf, 1, nbits, fp) * 8 / nbits) > 0) {
        for (unsigned i = 0, bits = 0; ncodes; ++i, ++ncodes2, bits += nbits, --ncodes) {
            unsigned *window = (unsigned *)(buf + bits / 8), newcode, c, pstack = sizeof(stack);
            newcode = c = *window >> i * (nbits - 8) % 8 & (1 << nbits) - 1;
            assert(c <= pos);

            if (c == 256) {
                nbits = 9, ncodes2 = 0, pos = 256;
                break;
            }
            
            if (c == pos)
                stack[--pstack] = finchar, c = oldcode;

            for (;c >= 256; c = codes[c])
                stack[--pstack] = htab[c];
            
            finchar = c;

            if (pos < sizeof(htab))
                codes[pos] = oldcode, htab[pos] = finchar, ++pos;

            putchar(finchar);
            fwrite(stack + pstack, 1, sizeof(stack) - pstack, stdout);
            oldcode = newcode;
        }

        if (ncodes2 == 1U << nbits - 1U && nbits != maxbits)
            ++nbits, ncodes2 = 0;
    }
}

