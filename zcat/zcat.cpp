#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <utime.h>

#define IBUFSIZ BUFSIZ  /* Default input buffer size                            */
#define OBUFSIZ BUFSIZ  /* Default output buffer size                           */

#define BIT_MASK    0x1f
#define BLOCK_MODE  0x80
#define FIRST   257                 /* first free entry                             */
#define CLEAR   256                 /* table clear output code                      */
#define INIT_BITS 9         /* initial number of bits/code */

#ifndef USERMEM
    /*
     * Set USERMEM to the maximum amount of physical user memory available
     * in bytes.  USERMEM is used to determine the maximum BITS that can be used
     * for compression.
     */
#   define USERMEM  450000  /* default user memory */
#endif

#ifndef BITS        /* General processor calculate BITS */
#   if USERMEM >= (800000)
#       define FAST
#   else
#   if USERMEM >= (433484)
#       define BITS 16
#   else
#   if USERMEM >= (229600)
#       define BITS 15
#   else
#   if USERMEM >= (127536)
#       define BITS 14
#   else
#   if USERMEM >= (73464)
#       define BITS 13
#   else
#       define BITS 12
#   endif
#   endif
#   endif
#   endif
#   endif
#endif /* BITS */

#ifdef FAST
#   define  HBITS       17          /* 50% occupancy */
#   define  HSIZE      (1<<HBITS)
#   define  HMASK      (HSIZE-1)
#   define  HPRIME       9941
#   define  BITS           16
#else
#   if BITS == 16
#       define HSIZE    69001       /* 95% occupancy */
#   endif
#   if BITS == 15
#       define HSIZE    35023       /* 94% occupancy */
#   endif
#   if BITS == 14
#       define HSIZE    18013       /* 91% occupancy */
#   endif
#   if BITS == 13
#       define HSIZE    9001        /* 91% occupancy */
#   endif
#   if BITS <= 12
#       define HSIZE    5003        /* 80% occupancy */
#   endif
#endif

typedef long int            code_int;

#ifdef SIGNED_COMPARE_SLOW
    typedef unsigned long int   count_int;
    typedef unsigned short int  count_short;
    typedef unsigned long int   cmp_code_int;   /* Cast to make compare faster  */
#else
    typedef long int            count_int;
    typedef long int            cmp_code_int;
#endif

typedef uint8_t char_type;

#define MAXCODE(n)  (1L << (n))

#define input(b,o,c,n,m){   char_type       *p = &(b)[(o)>>3];              \
                            (c) = ((((long)(p[0]))|((long)(p[1])<<8)|       \
                                     ((long)(p[2])<<16))>>((o)&0x7))&(m);   \
                            (o) += (n);                                     \
                        }

#define reset_n_bits_for_decompressor(n_bits, bitmask, maxbits, maxcode, maxmaxcode) {  \
    n_bits = INIT_BITS;                             \
    bitmask = (1<<n_bits)-1;                            \
    if (n_bits == maxbits)                              \
        maxcode = maxmaxcode;                           \
    else                                        \
        maxcode = MAXCODE(n_bits)-1;                        \
}

int maxbits = BITS;     /* user settable max # bits/code                */

#define tab_prefixof(i)         codetab[i]
#define tab_suffixof(i)         ((char_type *)(htab))[i]
#define de_stack                ((char_type *)&(htab[HSIZE-1]))
#define clear_htab()            memset(htab, -1, sizeof(htab))
#define clear_tab_prefixof()    memset(codetab, 0, 256);

void decompress(int fdin, int fdout)
{
    char_type inbuf[IBUFSIZ+64];  //Input buffer
    char_type outbuf[OBUFSIZ+2048];  //Output buffer
    long bytes_in = 0;  //Total number of byte from input
    count_int htab[HSIZE];
    unsigned short  codetab[HSIZE];
    char_type *stackp;
    code_int code;
    int finchar;
    code_int oldcode;
    code_int incode;
    int inbits;
    int posbits;
    int outpos;
    int insize = 0;
    int bitmask;
    code_int free_ent;
    code_int maxcode;
    int n_bits;
    int rsize;

    while (insize < 3 && (rsize = read(fdin, inbuf+insize, IBUFSIZ)) > 0)
        insize += rsize;

    assert(insize >= 3);
    assert(inbuf[0] == 0x1f);
    assert(inbuf[1] == 0x9d);
    maxbits = inbuf[2] & BIT_MASK;
    int block_mode = inbuf[2] & BLOCK_MODE;
    assert(maxbits <= BITS);
    code_int maxmaxcode = MAXCODE(maxbits);
    bytes_in = insize;
    reset_n_bits_for_decompressor(n_bits, bitmask, maxbits, maxcode, maxmaxcode);
    oldcode = -1;
    finchar = 0;
    outpos = 0;
    posbits = 3<<3;
    free_ent = block_mode ? FIRST : 256;
    clear_tab_prefixof();   //As above, initialize the first 256 entries in the table.

    for (code = 255 ; code >= 0 ; --code)
        tab_suffixof(code) = (char_type)code;

resetbuf:
    do
    {
        int o = posbits >> 3;
        int e = o <= insize ? insize - o : 0;

        for (int i = 0 ; i < e ; ++i)
            inbuf[i] = inbuf[i+o];

        insize = e;
        posbits = 0;

        if (insize < sizeof(inbuf)-IBUFSIZ)
        {
            rsize = read(fdin, inbuf + insize, IBUFSIZ);
            assert(rsize >= 0);
            insize += rsize;
        }

        inbits = rsize > 0 ? (insize - insize%n_bits)<<3 : (insize<<3)-(n_bits-1);

        while (inbits > posbits)
        {
            if (free_ent > maxcode)
            {
                posbits = ((posbits-1) + ((n_bits<<3) -(posbits-1+(n_bits<<3))%(n_bits<<3)));
                ++n_bits;
                if (n_bits == maxbits)
                    maxcode = maxmaxcode;
                else
                    maxcode = MAXCODE(n_bits)-1;

                bitmask = (1<<n_bits)-1;
                goto resetbuf;
            }

            input(inbuf,posbits,code,n_bits,bitmask);

            if (oldcode == -1)
            {
                    if (code >= 256) {
                        fprintf(stderr, "oldcode:-1 code:%i\n", (int)(code));
                        fprintf(stderr, "uncompress: corrupt input\n");
                        exit(1);
                    }
                    outbuf[outpos++] = (char_type)(finchar = (int)(oldcode = code));
                    continue;
            }

            if (code == CLEAR && block_mode)
            {
                    clear_tab_prefixof();
                    free_ent = FIRST - 1;
                    posbits = ((posbits-1) + ((n_bits<<3) -
                                (posbits-1+(n_bits<<3))%(n_bits<<3)));
                    reset_n_bits_for_decompressor(n_bits, bitmask, maxbits, maxcode, maxmaxcode);
                    goto resetbuf;
            }

            incode = code;
            stackp = de_stack;

            //Special case for KwKwK string.
            if (code >= free_ent)   
            {
                if (code > free_ent)
                {
                    char_type *p;
                    posbits -= n_bits;
                    p = &inbuf[posbits>>3];

                    if (p == inbuf)
                        ++p;

                    fprintf(stderr,
                        "insize:%d posbits:%d inbuf:%02X %02X %02X %02X %02X (%d)\n",
                        insize, posbits, p[-1],p[0],p[1],p[2],p[3], (posbits&07));

                    fprintf(stderr, "uncompress: corrupt input\n");
                    exit(1);
                }

                *--stackp = (char_type)finchar;
                code = oldcode;
            }

            while ((cmp_code_int)code >= (cmp_code_int)256)
            {
                /* Generate output characters in reverse order */
                *--stackp = tab_suffixof(code);
                code = tab_prefixof(code);
            }

            *--stackp = (char_type)(finchar = tab_suffixof(code));

            /* And put them out in forward order */
            {
                int i;

                if (outpos+(i = (de_stack-stackp)) >= OBUFSIZ)
                {
                    do
                    {
                        if (i > OBUFSIZ-outpos)
                            i = OBUFSIZ-outpos;

                        if (i > 0)
                        {
                            memcpy(outbuf+outpos, stackp, i);
                            outpos += i;
                        }

                        if (outpos >= OBUFSIZ)
                        {
                            assert(write(fdout, outbuf, outpos) == outpos);
                            outpos = 0;
                        }
                        stackp+= i;
                    }
                    while ((i = (de_stack-stackp)) > 0);
                }
                else
                {
                    memcpy(outbuf+outpos, stackp, i);
                    outpos += i;
                }
            }

            if ((code = free_ent) < maxmaxcode) /* Generate the new entry. */
            {
                tab_prefixof(code) = (unsigned short)oldcode;
                tab_suffixof(code) = (char_type)finchar;
                free_ent = code+1;
            }

            oldcode = incode;   /* Remember previous code. */
        }

        bytes_in += rsize;
    }
    while (rsize > 0);

    if (outpos > 0 && write(fdout, outbuf, outpos) != outpos)
        assert(false);
}

int main(int argc, char **argv)
{
    if (maxbits < INIT_BITS)    maxbits = INIT_BITS;
    if (maxbits > BITS)         maxbits = BITS;

    decompress(0, 1);
    return 0;
}


