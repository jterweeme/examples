#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <utime.h>

#define SIG_TYPE __sighandler_t

#if defined(AMIGA) || defined(DOS) || defined(MINGW) || defined(WINDOWS)
#   define  chmod(pathname, mode) 0
#   define  chown(pathname, owner, group) 0
#   define  utime(pathname, times) 0
#endif

#if defined(MINGW) || defined(WINDOWS)
#   define isatty(fd) 0
#   define open _open
#   define close _close
#   define read _read
#   define strdup _strdup
#   define unlink _unlink
#   define write _write
#else
/* NB: macOS has a setmode() that is different from Windows. */
#   define setmode(fd, mode)
#endif

#ifndef LSTAT
#   define  lstat   stat
#endif

#if defined(DOS) || defined(WINDOWS)
#   define  F_OK    0
static inline int access(const char *pathname, int mode)
{
    struct stat st;
    return lstat(pathname, &st);
}
#endif

#ifndef IBUFSIZ
#   define  IBUFSIZ BUFSIZ  /* Default input buffer size                            */
#endif
#ifndef OBUFSIZ
#   define  OBUFSIZ BUFSIZ  /* Default output buffer size                           */
#endif

                            /* Defines for third byte of header                     */
#define MAGIC_1     (char_type)'\037'/* First byte of compressed file               */
#define MAGIC_2     (char_type)'\235'/* Second byte of compressed file              */
#define BIT_MASK    0x1f            /* Mask for 'number of compression bits'        */
                                    /* Masks 0x20 and 0x40 are free.                */
                                    /* I think 0x20 should mean that there is       */
                                    /* a fourth header byte (for expansion).        */
#define BLOCK_MODE  0x80            /* Block compression if table is full and       */
                                    /* compression rate is dropping flush tables    */

            /* the next two codes should not be changed lightly, as they must not   */
            /* lie within the contiguous general code space.                        */
#define FIRST   257                 /* first free entry                             */
#define CLEAR   256                 /* table clear output code                      */

#define INIT_BITS 9         /* initial number of bits/code */

#ifndef SACREDMEM
    /*
     * SACREDMEM is the amount of physical memory saved for others; compress
     * will hog the rest.
     */
#   define SACREDMEM    0
#endif

#ifndef USERMEM
    /*
     * Set USERMEM to the maximum amount of physical user memory available
     * in bytes.  USERMEM is used to determine the maximum BITS that can be used
     * for compression.
     */
#   define USERMEM  450000  /* default user memory */
#endif

/*
 * machine variants which require cc -Dmachine:  pdp11, z8000, DOS
 */

#ifdef  DOS         /* PC/XT/AT (8088) processor                                    */
#   define  BITS   16   /* 16-bits processor max 12 bits                            */
#endif /* DOS */

#ifndef O_BINARY
#   define  O_BINARY    0   /* System has no binary mode                            */
#endif

#ifdef M_XENIX          /* Stupid compiler can't handle arrays with */
#   if BITS > 13            /* Code only handles BITS = 12, 13, or 16 */
#       define BITS 13
#   endif
#endif

#ifndef BITS        /* General processor calculate BITS                             */
#   if USERMEM >= (800000+SACREDMEM)
#       define FAST
#   else
#   if USERMEM >= (433484+SACREDMEM)
#       define BITS 16
#   else
#   if USERMEM >= (229600+SACREDMEM)
#       define BITS 15
#   else
#   if USERMEM >= (127536+SACREDMEM)
#       define BITS 14
#   else
#   if USERMEM >= (73464+SACREDMEM)
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

#define CHECK_GAP 10000

typedef long int            code_int;

#ifdef SIGNED_COMPARE_SLOW
    typedef unsigned long int   count_int;
    typedef unsigned short int  count_short;
    typedef unsigned long int   cmp_code_int;   /* Cast to make compare faster  */
#else
    typedef long int            count_int;
    typedef long int            cmp_code_int;
#endif

typedef unsigned char   char_type;

#define ARGVAL() (*++(*argv) || (--argc && *++argv))

#define MAXCODE(n)  (1L << (n))

#define output(b,o,c,n) {   char_type   *p = &(b)[(o)>>3];                  \
                            long         i = ((long)(c))<<((o)&0x7);        \
                            p[0] |= (char_type)(i);                         \
                            p[1] |= (char_type)(i>>8);                      \
                            p[2] |= (char_type)(i>>16);                     \
                            (o) += (n);                                     \
                        }
#define input(b,o,c,n,m){   char_type       *p = &(b)[(o)>>3];              \
                            (c) = ((((long)(p[0]))|((long)(p[1])<<8)|       \
                                     ((long)(p[2])<<16))>>((o)&0x7))&(m);   \
                            (o) += (n);                                     \
                        }

#define reset_n_bits_for_compressor(n_bits, stcode, free_ent, extcode, maxbits) {   \
    n_bits = INIT_BITS;                             \
    stcode = 1;                                 \
    free_ent = FIRST;                               \
    extcode = MAXCODE(n_bits);                          \
    if (n_bits < maxbits)                               \
        extcode++;                              \
}

#define reset_n_bits_for_decompressor(n_bits, bitmask, maxbits, maxcode, maxmaxcode) {  \
    n_bits = INIT_BITS;                             \
    bitmask = (1<<n_bits)-1;                            \
    if (n_bits == maxbits)                              \
        maxcode = maxmaxcode;                           \
    else                                        \
        maxcode = MAXCODE(n_bits)-1;                        \
}

char            *progname;          /* Program name                                 */
int             silent = 0;         /* don't tell me about errors                   */
int             quiet = 1;          /* don't tell me about compression              */
int             do_decomp = 0;      /* Decompress mode                              */
int             force = 0;          /* Force overwrite of files and links           */
int             keep = 0;           /* Keep input files                             */
int             nomagic = 0;        /* Use a 3-byte magic number header,            */
                                    /* unless old file                              */
int             maxbits = BITS;     /* user settable max # bits/code                */
int             zcat_flg = 0;       /* Write output on stdout, suppress messages    */
int             recursive = 0;      /* compress directories                         */
int             exit_code = -1;     /* Exitcode of compress (-1 no file compressed) */

char_type       inbuf[IBUFSIZ+64];  /* Input buffer                                 */
char_type       outbuf[OBUFSIZ+2048];/* Output buffer                               */

char            *ifname;            /* Input filename                               */
int             remove_ofname = 0;  /* Remove output file on a error                */
char            *ofname = NULL;     /* Output filename                              */
int             fgnd_flag = 0;      /* Running in background (SIGINT=SIGIGN)        */

long            bytes_in;           /* Total number of byte from input              */
long            bytes_out;          /* Total number of byte to output               */

count_int       htab[HSIZE];
unsigned short  codetab[HSIZE];

#define tab_prefixof(i)         codetab[i]
#define tab_suffixof(i)         ((char_type *)(htab))[i]
#define de_stack                ((char_type *)&(htab[HSIZE-1]))
#define clear_htab()            memset(htab, -1, sizeof(htab))
#define clear_tab_prefixof()    memset(codetab, 0, 256);

#ifdef FAST
static const int primetab[256] =        /* Special secudary hash table.     */
    {
         1013, -1061, 1109, -1181, 1231, -1291, 1361, -1429,
         1481, -1531, 1583, -1627, 1699, -1759, 1831, -1889,
         1973, -2017, 2083, -2137, 2213, -2273, 2339, -2383,
         2441, -2531, 2593, -2663, 2707, -2753, 2819, -2887,
         2957, -3023, 3089, -3181, 3251, -3313, 3361, -3449,
         3511, -3557, 3617, -3677, 3739, -3821, 3881, -3931,
         4013, -4079, 4139, -4219, 4271, -4349, 4423, -4493,
         4561, -4639, 4691, -4783, 4831, -4931, 4973, -5023,
         5101, -5179, 5261, -5333, 5413, -5471, 5521, -5591,
         5659, -5737, 5807, -5857, 5923, -6029, 6089, -6151,
         6221, -6287, 6343, -6397, 6491, -6571, 6659, -6709,
         6791, -6857, 6917, -6983, 7043, -7129, 7213, -7297,
         7369, -7477, 7529, -7577, 7643, -7703, 7789, -7873,
         7933, -8017, 8093, -8171, 8237, -8297, 8387, -8461,
         8543, -8627, 8689, -8741, 8819, -8867, 8963, -9029,
         9109, -9181, 9241, -9323, 9397, -9439, 9511, -9613,
         9677, -9743, 9811, -9871, 9941,-10061,10111,-10177,
        10259,-10321,10399,-10477,10567,-10639,10711,-10789,
        10867,-10949,11047,-11113,11173,-11261,11329,-11423,
        11491,-11587,11681,-11777,11827,-11903,11959,-12041,
        12109,-12197,12263,-12343,12413,-12487,12541,-12611,
        12671,-12757,12829,-12917,12979,-13043,13127,-13187,
        13291,-13367,13451,-13523,13619,-13691,13751,-13829,
        13901,-13967,14057,-14153,14249,-14341,14419,-14489,
        14557,-14633,14717,-14767,14831,-14897,14983,-15083,
        15149,-15233,15289,-15359,15427,-15497,15583,-15649,
        15733,-15791,15881,-15937,16057,-16097,16189,-16267,
        16363,-16447,16529,-16619,16691,-16763,16879,-16937,
        17021,-17093,17183,-17257,17341,-17401,17477,-17551,
        17623,-17713,17791,-17891,17957,-18041,18097,-18169,
        18233,-18307,18379,-18451,18523,-18637,18731,-18803,
        18919,-19031,19121,-19211,19273,-19381,19429,-19477
    } ;
#endif

static void Usage(int);
static void read_error(void);
static void write_error(void);
static void abort_compress(void);

void
Usage(int status)
    {
        fprintf(status ? stderr : stdout, "\
Usage: %s [-dfhvcVr] [-b maxbits] [--] [path ...]\n\
  --   Halt option processing and treat all remaining args as paths.\n\
  -d   If given, decompression is done instead.\n\
  -c   Write output on stdout, don't remove original.\n\
  -k   Keep input files (do not automatically remove).\n\
  -b   Parameter limits the max number of bits/code.\n\
  -f   Forces output file to be generated, even if one already.\n\
       exists, and even if no space is saved by compressing.\n\
       If -f is not used, the user will be prompted if stdin is.\n\
       a tty, otherwise, the output file will not be overwritten.\n\
  -h   This help output.\n\
  -v   Write compression statistics.\n\
  -V   Output version and compile options.\n\
  -r   Recursive. If a path is a directory, compress everything in it.\n",
            progname);

            exit(status);
}

void
decompress(int fdin, int fdout)
    {
        char_type *stackp;
        code_int code;
        int finchar;
        code_int oldcode;
        code_int incode;
        int inbits;
        int posbits;
        int outpos;
        int insize;
        int bitmask;
        code_int free_ent;
        code_int maxcode;
        code_int maxmaxcode;
        int n_bits;
        int rsize;
        int block_mode;

        bytes_in = 0;
        bytes_out = 0;
        insize = 0;

        while (insize < 3 && (rsize = read(fdin, inbuf+insize, IBUFSIZ)) > 0)
            insize += rsize;

        if (insize < 3 || inbuf[0] != MAGIC_1 || inbuf[1] != MAGIC_2)
        {
            if (rsize < 0)
                read_error();

            if (insize > 0)
            {
                fprintf(stderr, "%s: not in compressed format\n",
                                    (ifname[0] != '\0'? ifname : "stdin"));
                exit_code = 1;
            }

            return ;
        }

        maxbits = inbuf[2] & BIT_MASK;
        block_mode = inbuf[2] & BLOCK_MODE;

        if (maxbits > BITS)
        {
            fprintf(stderr,
                    "%s: compressed with %d bits, can only handle %d bits\n",
                    (*ifname != '\0' ? ifname : "stdin"), maxbits, BITS);
            exit_code = 4;
            return;
        }

        maxmaxcode = MAXCODE(maxbits);

        bytes_in = insize;
        reset_n_bits_for_decompressor(n_bits, bitmask, maxbits, maxcode, maxmaxcode);
        oldcode = -1;
        finchar = 0;
        outpos = 0;
        posbits = 3<<3;

        free_ent = ((block_mode) ? FIRST : 256);

        clear_tab_prefixof();   /* As above, initialize the first
                                   256 entries in the table. */

        for (code = 255 ; code >= 0 ; --code)
            tab_suffixof(code) = (char_type)code;

        do
        {
resetbuf:   ;
            {
                int i;
                int e;
                int o;

                o = posbits >> 3;
                e = o <= insize ? insize - o : 0;

                for (i = 0 ; i < e ; ++i)
                    inbuf[i] = inbuf[i+o];

                insize = e;
                posbits = 0;
            }

            if (insize < sizeof(inbuf)-IBUFSIZ)
            {
                if ((rsize = read(fdin, inbuf+insize, IBUFSIZ)) < 0)
                    read_error();

                insize += rsize;
            }

            inbits = ((rsize > 0) ? (insize - insize%n_bits)<<3 :
                                    (insize<<3)-(n_bits-1));

            while (inbits > posbits)
            {
                if (free_ent > maxcode)
                {
                    posbits = ((posbits-1) + ((n_bits<<3) -
                                     (posbits-1+(n_bits<<3))%(n_bits<<3)));

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
                        abort_compress();
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

                if (code >= free_ent)   /* Special case for KwKwK string.   */
                {
                    if (code > free_ent)
                    {
                        char_type *p;

                        posbits -= n_bits;
                        p = &inbuf[posbits>>3];
                        if (p == inbuf) p++;

                        fprintf(stderr, "insize:%d posbits:%d inbuf:%02X %02X %02X %02X %02X (%d)\n", insize, posbits,
                                p[-1],p[0],p[1],p[2],p[3], (posbits&07));
                        fprintf(stderr, "uncompress: corrupt input\n");
                        abort_compress();
                    }

                    *--stackp = (char_type)finchar;
                    code = oldcode;
                }

                while ((cmp_code_int)code >= (cmp_code_int)256)
                { /* Generate output characters in reverse order */
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
                            if (i > OBUFSIZ-outpos) i = OBUFSIZ-outpos;

                            if (i > 0)
                            {
                                memcpy(outbuf+outpos, stackp, i);
                                outpos += i;
                            }

                            if (outpos >= OBUFSIZ)
                            {
                                if (write(fdout, outbuf, outpos) != outpos)
                                    write_error();

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

                oldcode = incode;   /* Remember previous code.  */
            }

            bytes_in += rsize;
        }
        while (rsize > 0);

        if (outpos > 0 && write(fdout, outbuf, outpos) != outpos)
            write_error();
    }

void
read_error(void)
    {
        fprintf(stderr, "\nread error on");
        perror((ifname[0] != '\0') ? ifname : "stdin");
        abort_compress();
    }

void
write_error(void)
    {
        fprintf(stderr, "\nwrite error on");
        perror(ofname ? ofname : "stdout");
        abort_compress();
    }

void abort_compress(void)
{
        if (remove_ofname)
            unlink(ofname);

        exit(1);
}

int main(int argc, char **argv)
{
    char **filelist;
    char **fileptr;
    int seen_double_dash = 0;
#ifdef SIGINT
    if ((fgnd_flag = (signal(SIGINT, SIG_IGN)) != SIG_IGN))
        signal(SIGINT, (SIG_TYPE)abort_compress);
#endif

#ifdef SIGTERM
    signal(SIGTERM, (SIG_TYPE)abort_compress);
#endif
#ifdef SIGHUP
    signal(SIGHUP, (SIG_TYPE)abort_compress);
#endif

#ifdef COMPATIBLE
        nomagic = 1;    /* Original didn't have a magic number */
#endif

        filelist = (char **)malloc(argc*sizeof(char *));
        if (filelist == NULL)
        {
            fprintf(stderr, "Cannot allocate memory for file list.\n");
            exit (1);
        }
        fileptr = filelist;
        *filelist = NULL;

        if ((progname = strrchr(argv[0], '/')) != 0)
            progname++;
        else
            progname = argv[0];

        if (strcmp(progname, "uncompress") == 0)
            do_decomp = 1;
        else if (strcmp(progname, "zcat") == 0)
            do_decomp = zcat_flg = 1;

        for (argc--, argv++; argc > 0; argc--, argv++)
        {
            if (strcmp(*argv, "--") == 0)
            {
                seen_double_dash = 1;
                continue;
            }

            if (seen_double_dash == 0 && **argv == '-')
            {/* A flag argument */
                while (*++(*argv))
                {/* Process all flags in this arg */
                    switch (**argv)
                    {
                    case 's':
                        silent = 1;
                        quiet = 1;
                        break;

                    case 'v':
                        silent = 0;
                        quiet = 0;
                        break;

                    case 'd':
                        do_decomp = 1;
                        break;

                    case 'k':
                        keep = 1;
                        break;

                    case 'f':
                    case 'F':
                        force = 1;
                        break;

                    case 'n':
                        nomagic = 1;
                        break;

                    case 'b':
                        if (!ARGVAL())
                        {
                            fprintf(stderr, "Missing maxbits\n");
                            Usage(1);
                        }

                        maxbits = atoi(*argv);
                        goto nextarg;

                    case 'c':
                        zcat_flg = 1;
                        break;

                    case 'q':
                        quiet = 1;
                        break;
                    case 'r':
                    case 'R':
                        fprintf(stderr,
                            "%s -r not available (due to missing directory functions)\n", *argv);
                        break;

                    case 'h':
                        Usage(0);
                        break;

                    default:
                        fprintf(stderr, "Unknown flag: '%c'; ", **argv);
                        Usage(1);
                    }
                }
            }
            else
            {
                *fileptr++ = *argv; /* Build input file list */
                *fileptr = NULL;
            }

nextarg:    continue;
    }

    if (maxbits < INIT_BITS)    maxbits = INIT_BITS;
    if (maxbits > BITS)         maxbits = BITS;

    ifname = "";
    exit_code = 0;
    remove_ofname = 0;
    setmode(0, O_BINARY);
    setmode(1, O_BINARY);
    decompress(0, 1);

    if (recursive && exit_code == -1) {
        fprintf(stderr, "no files processed after recursive search\n");
    }
    exit((exit_code== -1) ? 1:exit_code);
}


