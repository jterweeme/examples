#include <iostream>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef int32_t code_int;
typedef uint32_t count_int;
typedef uint16_t count_short;
typedef uint32_t cmp_code_int;
typedef uint8_t char_type;

#define	MAGIC_1		(char_type)'\037'
#define	MAGIC_2		(char_type)'\235'
#define BIT_MASK	0x1f
#define BLOCK_MODE	0x80
#define FIRST	257	
#define	CLEAR	256
#define INIT_BITS 9			/* initial number of bits/code */
#define	HBITS		17			/* 50% occupancy */
#define	HSIZE	   (1<<HBITS)
#define	BITS		   16
#define MAXCODE(n)	(1L << (n))
#define	tab_prefixof(i)			codetab[i]
#define	tab_suffixof(i)			((char_type *)(htab))[i]
#define	de_stack				((char_type *)&(htab[HSIZE-1]))

#define	input(b,o,c,n,m){	char_type 		*p = &(b)[(o)>>3];				\
							(c) = ((((long)(p[0]))|((long)(p[1])<<8)|		\
									 ((long)(p[2])<<16))>>((o)&0x7))&(m);	\
							(o) += (n);										\
						}

#define reset_n_bits_for_decompressor(n_bits, bitmask, maxbits, maxcode, maxmaxcode) {	\
	n_bits = INIT_BITS;								\
	bitmask = (1<<n_bits)-1;							\
	if (n_bits == maxbits)								\
		maxcode = maxmaxcode;							\
	else										\
		maxcode = MAXCODE(n_bits)-1;						\
}

int main()
{
    int maxbits = BITS;		/* user settable max # bits/code 				*/
    char_type inbuf[BUFSIZ+64];	/* Input buffer									*/
    char_type outbuf[BUFSIZ+2048];/* Output buffer								*/
    long bytes_in;			/* Total number of byte from input				*/
    count_int htab[HSIZE];
    uint16_t codetab[HSIZE];
	char_type *stackp;
	code_int code;
	int finchar;
	code_int oldcode;
	code_int incode;
	int inbits;
	int posbits;
    int outpos;
	int bitmask;
	code_int free_ent;
	code_int maxcode;
	code_int maxmaxcode;
	int n_bits;
	std::streamsize rsize = 0;
	int block_mode;
	bytes_in = 0;
	int insize = 0;
    std::cin.read((char *)(inbuf + insize), BUFSIZ);
    rsize = std::cin.gcount();
    insize += rsize;
	maxbits = inbuf[2] & BIT_MASK;
	block_mode = inbuf[2] & BLOCK_MODE;
    assert(maxbits <= BITS);
	maxmaxcode = 1 << maxbits;
	bytes_in = insize;
	reset_n_bits_for_decompressor(n_bits, bitmask, maxbits, maxcode, maxmaxcode);
	oldcode = -1;
	finchar = 0;
	outpos = 0;
	posbits = 3<<3;
    free_ent = ((block_mode) ? FIRST : 256);
    memset(codetab, 0, 256);

    for (code = 255 ; code >= 0 ; --code)
		tab_suffixof(code) = (char_type)code;

	do
	{
resetbuf:	;
		{
            int o = posbits >> 3;
            int e = o <= insize ? insize - o : 0;

            for (int i = 0 ; i < e ; ++i)
				inbuf[i] = inbuf[i+o];

			insize = e;
			posbits = 0;
		}

		if (insize < int(sizeof(inbuf) - BUFSIZ))
		{
            std::cin.read((char *)(inbuf + insize), BUFSIZ);
            rsize = std::cin.gcount();
            assert(rsize >= 0);
			insize += rsize;
		}

		inbits = rsize > 0 ? insize - insize % n_bits << 3 : (insize << 3) - (n_bits - 1);

		while (inbits > posbits)
		{
			if (free_ent > maxcode)
			{
				posbits = ((posbits-1) + ((n_bits<<3) - (posbits-1+(n_bits<<3))%(n_bits<<3)));
				++n_bits;
                maxcode = n_bits == maxbits ? maxmaxcode : MAXCODE(n_bits) - 1;
				bitmask = (1<<n_bits)-1;
				goto resetbuf;
			}

			input(inbuf,posbits,code,n_bits,bitmask);

			if (oldcode == -1)
			{
                assert(code < 256);
				outbuf[outpos++] = (char_type)(finchar = (int)(oldcode = code));
				continue;
			}

			if (code == CLEAR && block_mode)
			{
                memset(codetab, 0, 256);
    			free_ent = FIRST - 1;
				posbits = ((posbits-1) + ((n_bits<<3) - (posbits-1+(n_bits<<3))%(n_bits<<3)));
				reset_n_bits_for_decompressor(n_bits, bitmask, maxbits, maxcode, maxmaxcode);
				goto resetbuf;
			}

			incode = code;
		    stackp = de_stack;

			if (code >= free_ent)	/* Special case for KwKwK string.	*/
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
                    exit(1);
				}

       	    	*--stackp = (char_type)finchar;
	    		code = oldcode;
			}

			while ((cmp_code_int)code >= (cmp_code_int)256)
			{
                // Generate output characters in reverse order
		    	*--stackp = tab_suffixof(code);
		    	code = tab_prefixof(code);
			}

			*--stackp =	(char_type)(finchar = tab_suffixof(code));

			// And put them out in forward order
			{
				int i;

				if (outpos+(i = (de_stack-stackp)) >= BUFSIZ)
				{
					do
					{
                        i = std::min(i, BUFSIZ - outpos);

						if (i > 0)
						{
							memcpy(outbuf+outpos, stackp, i);
							outpos += i;
						}

						if (outpos >= BUFSIZ)
						{
                            std::cout.write((const char *)(outbuf), outpos);
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
		    	tab_prefixof(code) = uint16_t(oldcode);
		    	tab_suffixof(code) = (char_type)finchar;
    			free_ent = code+1;
			}

			oldcode = incode;	/* Remember previous code.	*/
		}

		bytes_in += rsize;
    }
	while (rsize > 0);

    return 0;
}



