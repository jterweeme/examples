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
#include <dirent.h>
#include <unistd.h>

#ifdef UTIME_H
#	include	<utime.h>
#else
	struct utimbuf {
		time_t actime;
		time_t modtime;
	};
#endif

#ifndef SIG_TYPE
#	define	SIG_TYPE	void (*)()
#endif

#ifndef	LSTAT
#define	lstat	stat
#endif

#ifndef	IBUFSIZ
#define	IBUFSIZ	BUFSIZ
#endif
#ifndef	OBUFSIZ
#define	OBUFSIZ	BUFSIZ
#endif

#define	MAGIC_1		(char_type)'\037'
#define	MAGIC_2		(char_type)'\235'
#define BIT_MASK	0x1f			
#define BLOCK_MODE	0x80			
#define FIRST	257					
#define	CLEAR	256					
#define INIT_BITS 9

#ifndef USERMEM
#define USERMEM	450000
#endif

#ifndef	O_BINARY
#define	O_BINARY 0
#endif

#ifndef BITS
#	if USERMEM >= (800000)
#		define FAST
#	else
#	if USERMEM >= (433484)
#		define BITS	16
#	else
#	if USERMEM >= (229600)
#		define BITS	15
#	else
#	if USERMEM >= (127536)
#		define BITS	14
#   else
#	if USERMEM >= (73464)
#		define BITS	13
#	else
#		define BITS	12
#	endif
#	endif
#   endif
#	endif
#	endif
#endif /* BITS */

#ifdef FAST
#	define	HBITS		17			/* 50% occupancy */
#	define	HSIZE	   (1<<HBITS)
#	define	HMASK	   (HSIZE-1)
#	define	HPRIME		 9941
#	define	BITS		   16
#else
#	if BITS == 16
#		define HSIZE	69001		/* 95% occupancy */
#	endif
#	if BITS == 15
#		define HSIZE	35023		/* 94% occupancy */
#	endif
#	if BITS == 14
#		define HSIZE	18013		/* 91% occupancy */
#	endif
#	if BITS == 13
#		define HSIZE	9001		/* 91% occupancy */
#	endif
#	if BITS <= 12
#		define HSIZE	5003		/* 80% occupancy */
#	endif
#endif

#define CHECK_GAP 10000

typedef long int			code_int;

#ifdef SIGNED_COMPARE_SLOW
	typedef unsigned long int	count_int;
	typedef unsigned short int	count_short;
	typedef unsigned long int	cmp_code_int;	/* Cast to make compare faster	*/
#else
	typedef long int	 		count_int;
	typedef long int			cmp_code_int;
#endif

typedef	unsigned char	char_type;

#define MAXCODE(n)	(1L << (n))

#define	input(b,o,c,n,m){	char_type 		*p = &(b)[(o)>>3];				\
							(c) = ((((long)(p[0]))|((long)(p[1])<<8)|		\
									 ((long)(p[2])<<16))>>((o)&0x7))&(m);	\
							(o) += (n);										\
						}

#define reset_n_bits_for_compressor(n_bits, stcode, free_ent, extcode, maxbits) {	\
	n_bits = INIT_BITS;								\
	stcode = 1;									\
	free_ent = FIRST;								\
	extcode = MAXCODE(n_bits);							\
	if (n_bits < maxbits)								\
		extcode++;								\
}

#define reset_n_bits_for_decompressor(n_bits, bitmask, maxbits, maxcode, maxmaxcode) {	\
	n_bits = INIT_BITS;								\
	bitmask = (1<<n_bits)-1;							\
	if (n_bits == maxbits)								\
		maxcode = maxmaxcode;							\
	else										\
		maxcode = MAXCODE(n_bits)-1;						\
}

int maxbits = BITS;		/* user settable max # bits/code 				*/
int 			zcat_flg = 0;		/* Write output on stdout, suppress messages 	*/
int				recursive = 0;  	/* compress directories 						*/
int				exit_code = -1;		/* Exitcode of compress (-1 no file compressed)	*/
char_type		inbuf[IBUFSIZ+64];	/* Input buffer									*/
char_type		outbuf[OBUFSIZ+2048];/* Output buffer								*/
struct stat		infstat;			/* Input file status							*/
char			*ifname;			/* Input filename								*/
int				remove_ofname = 0;	/* Remove output file on a error				*/
char			*ofname = NULL;		/* Output filename								*/
int				fgnd_flag = 0;		/* Running in background (SIGINT=SIGIGN)		*/
long 			bytes_in;			/* Total number of byte from input				*/
long 			bytes_out;			/* Total number of byte to output				*/
count_int		htab[HSIZE];
unsigned short	codetab[HSIZE];

#define	tab_prefixof(i)			codetab[i]
#define	tab_suffixof(i)			((char_type *)(htab))[i]
#define	de_stack				((char_type *)&(htab[HSIZE-1]))
#define	clear_htab()			memset(htab, -1, sizeof(htab))
#define	clear_tab_prefixof()	memset(codetab, 0, 256);

static void read_error(void);
static void write_error(void);
static void abort_compress(void);

void decompress(int fdin, int fdout)
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
		fprintf(stderr, "%s: compressed with %d bits, can only handle %d bits\n",
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
	clear_tab_prefixof();

    for (code = 255 ; code >= 0 ; --code)
		tab_suffixof(code) = (char_type)code;

resetbuf:
	{
		int o = posbits >> 3;
		int e = o <= insize ? insize - o : 0;

		for (int i = 0 ; i < e ; ++i)
			inbuf[i] = inbuf[i+o];

		insize = e;
		posbits = 0;

		if (insize < sizeof(inbuf)-IBUFSIZ)
		{
				if ((rsize = read(fdin, inbuf+insize, IBUFSIZ)) < 0)
					read_error();

				insize += rsize;
		}

		inbits = ((rsize > 0) ? (insize - insize%n_bits)<<3 : (insize<<3)-(n_bits-1));

		while (inbits > posbits)
		{
			if (free_ent > maxcode)
			{
				posbits = ((posbits-1) + ((n_bits<<3) - (posbits-1+(n_bits<<3))%(n_bits<<3)));
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
				posbits = ((posbits-1) + ((n_bits<<3) - (posbits-1+(n_bits<<3))%(n_bits<<3)));
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
                        p++;
					fprintf(stderr, "insize:%d posbits:%d inbuf:%02X %02X %02X %02X %02X (%d)\n",
                        insize, posbits, p[-1],p[0],p[1],p[2],p[3], (posbits&07));
		    		fprintf(stderr, "uncompress: corrupt input\n");
					abort_compress();
				}

       	    	*--stackp = (char_type)finchar;
	    		code = oldcode;
			}

			while ((cmp_code_int)code >= (cmp_code_int)256)
			{
		    	*--stackp = tab_suffixof(code);
		    	code = tab_prefixof(code);
			}

			*--stackp =	(char_type)(finchar = tab_suffixof(code));

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
                            memcpy(outbuf + outpos, stackp, i);
                            outpos += i;
                        }

                        if (outpos >= OBUFSIZ)
                        {
                            if (write(fdout, outbuf, outpos) != outpos)
                                write_error();

                            outpos = 0;
                        }
                        stackp += i;
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

            oldcode = incode;	/* Remember previous code.	*/
        }

        bytes_in += rsize;
    }

    if (rsize > 0)
        goto resetbuf;

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

void
abort_compress(void)
	{
		if (remove_ofname)
	    	unlink(ofname);

		exit(1);
	}

void comprexx(const char *fileptr)
{
	int fdin = -1;
	int fdout = -1;
	int has_z_suffix;
    char *tempname;
	unsigned long namesize = strlen(fileptr);
	tempname = malloc(namesize + 3);
	if (tempname == NULL)
	{
		perror("malloc");
		goto error;
	}

	strcpy(tempname,fileptr);
	has_z_suffix = (namesize >= 2 && strcmp(&tempname[namesize - 2], ".Z") == 0);
	errno = 0;
	lstat(tempname,&infstat);

	switch (infstat.st_mode & S_IFMT)
	{
	case S_IFREG:	/* regular file */
		free(ofname);
		ofname = strdup(tempname);
		if (ofname == NULL)
		{
			perror("strdup");
			goto error;
		}

        if (has_z_suffix)
            ofname[namesize - 2] = '\0';

    	if ((fdin = open(ifname = tempname, O_RDONLY|O_BINARY)) == -1)
		{
	      	perror(tempname);
			goto error;
    	}

		fdout = 1;
        remove_ofname = 0;
        decompress(fdin, fdout);
		close(fdin);

		if (fdout != 1 && close(fdout))
			write_error();

		if (exit_code == -1)
			exit_code = 0;
  		break;
	default:
  		fprintf(stderr,"%s is not a directory or a regular file - ignored\n", tempname);
  		break;
	}

	free(tempname);
	if (!remove_ofname)
	{
		free(ofname);
		ofname = NULL;
	}
	return;
error:
	free(ofname);
	ofname = NULL;
	free(tempname);
	exit_code = 1;
	if (fdin != -1)
		close(fdin);
	if (fdout != -1)
		close(fdout);
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
   	filelist = (char **)malloc(argc*sizeof(char *));
   	if (filelist == NULL)
	{
		fprintf(stderr, "Cannot allocate memory for file list.\n");
		exit (1);
	}
   	fileptr = filelist;
   	*filelist = NULL;
	zcat_flg = 1;

   	for (argc--, argv++; argc > 0; argc--, argv++)
	{
    	*fileptr++ = *argv;	/* Build input file list */
    	*fileptr = NULL;
        continue;
    }

   	if (maxbits < INIT_BITS)	maxbits = INIT_BITS;
   	if (maxbits > BITS) 		maxbits = BITS;

   	if (*filelist != NULL)
	{
        for (fileptr = filelist; *fileptr; fileptr++)
            comprexx(*fileptr);
   	}

	exit((exit_code== -1) ? 1:exit_code);
}

