/*
 ***********************************************************************
 *
 *        @version  1.0
 *        @date     03/11/2014 02:09:32 PM
 *        @author   Fridolin Pokorny <fridex.devel@gmail.com>
 *
 ***********************************************************************
 */

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <fcntl.h>
#include <getopt.h>
#include <cassert>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "gif2bmp.h"
#include "common.h"


/**
 * @brief  Program description and author
 */
static const char * MSG_AUTHOR =
	"Simple GIF to BMP conversion app\n"
	"Written by Fridolin Pokorny 2014 <fridex.devel@gmail.com>\n";

/**
 * @brief  Program usage printed in help
 */
static const char * MSG_USAGE =
	"OPTIONS:\n"
	"\t-i FILE\t\t- use FILE as intut file instead of standard input\n"
	"\t-o FILE\t\t- use FILE as output file instead of standard output\n"
	"\t-l FILE\t\t- use FILE as log file\n"
	"\t-e\t\t- extract all images from GIF, cannot be used with -o\n"
	"\t\t\timages are saved as 0001.bmp, 0002.bmp...\n"
	"\t-h FILE\t\t-print this simple help";

/**
 * @brief  Clean up opened files
 *
 * @param in_file input file to be closed
 * @param out_file output file to be closed
 * @param log_file log file to be closed
 */
static inline void clean_up(FILE *in_file, FILE *out_file, FILE *log_file)
{
    if (in_file && in_file != stdin)
        fclose(in_file);

    if (out_file && out_file != stdout)
        fclose(out_file);

    if (log_file)
        fclose(log_file);
}

/**
 * @brief  Print help
 *
 * @param pname program name
 */
void print_help(const char *pname)
{
	puts(MSG_AUTHOR);
	printf("%s [OPTIONS]\n", pname);
	puts(MSG_USAGE);
}

/**
 * @brief  Print conversion statistics
 *
 * @param log_file log file to print to
 * @param status conversion statistics
 */
void print_stats(FILE * log_file, struct gif2bmp_t * status) {
	assert(status);

	if (! log_file)
		return;

	fputs("login = xlogin00\n", log_file);
	fprintf(log_file, "uncodedSize = %" PRId64 "\n", status->bmp_size);
	fprintf(log_file, "codedSize = %" PRId64 "\n", status->gif_size);
}

/**
 * @brief  Entry point
 *
 * @param argc argument count
 * @param argv[] argument vector
 *
 * @return EXIT_SUCCES (0) on success, otherwise error code
 */
int main(int argc, char * argv[]) {
	FILE * in_file  = stdin;
	FILE * out_file = stdout;
	FILE * log_file = NULL;
	struct gif2bmp_t status;
	int res = EXIT_SUCCESS;

#ifdef WIN32
    _setmode(_fileno(stdin), _O_BINARY);
#endif

	 int c;
	 while ((c = getopt(argc, argv, "i:o:l:he")) != -1) {
		 switch (c) {
			case 'i':
				in_file = fopen(optarg, "rb");
				if (! in_file) {
					clean_up(in_file, out_file, log_file);
					perror(optarg);
					return EXIT_FAILURE;
				}
				break;
			case 'o':
				if (out_file == NULL) {
					err() << "Cannot use -o and -e at the same time!\n";
					return EXIT_FAILURE;
				}
				out_file = fopen(optarg, "wb");
				if (! out_file) {
					clean_up(in_file, out_file, log_file);
					perror(optarg);
					return EXIT_FAILURE;
				}
				break;
			case 'e':
				if (out_file != stdout) {
					err() << "Cannot use -o and -e at the same time!\n";
					return EXIT_FAILURE;
				}
				out_file = NULL;
				break;
			case 'l':
				log_file = fopen(optarg, "w");
				if (! log_file) {
					clean_up(in_file, out_file, log_file);
					perror(optarg);
					return EXIT_FAILURE;
				}
				break;
			case 'h':
				print_help(argv[0]);
				clean_up(in_file, out_file, log_file);
				return EXIT_FAILURE;
				break;
			case '?':
				return EXIT_FAILURE;
				break;
			default:
				UNREACHABLE();
				break;
		 }
	 }

	for (int idx = optind; idx < argc; idx++) {
		fprintf(stderr, "Unknown option %s\n", argv[idx]);
		res = EXIT_FAILURE;
	}

	/*
	 * Just do it!
	 */
    if (res != EXIT_SUCCESS)
    {
		clean_up(in_file, out_file, log_file);
		return res;
    }
    else
    {
		res = gif2bmp(&status, in_file, out_file);

		if (res == 0 && log_file)
			print_stats(log_file, &status);

		clean_up(in_file, out_file, log_file);
		return res;
	}

	UNREACHABLE();
	return EXIT_FAILURE;
}

