#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

static int code(FILE *fp)
{
    int n = 0;
    bool flag = false;

    for (int c; (c = fgetc(fp)) != -1;)
    {
        if (isdigit(c))
        {
            flag = true;
            n = n * 10 + c - '0';
        }
        else
        {
            if (flag)
                return n;

            flag = false;
            n = 0;
        }
    }

    return -1;
}

int main()
{
    FILE *os = stdout;
    fputc(0x1f, os);
    fputc(0x9d, os);
    fputc(0x90, os);
    unsigned cnt = 0, nbits = 9;
    const unsigned bitdepth = 16;
    char buf[20] = {0};
    
    for (int c; (c = code(stdin)) != -1;)
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


