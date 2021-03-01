/*
 ***********************************************************************
 *
 *        @version  1.0
 *        @date     03/11/2014 02:09:23 PM
 *        @author   Fridolin Pokorny <fridex.devel@gmail.com>
 *
 ***********************************************************************
 */

#ifndef GIF2BMP_H_
#define GIF2BMP_H_

#include <inttypes.h>
#include <cstdio>

/**
 * @brief  Sizes of input/output in total
 */
struct gif2bmp_t {
	int64_t bmp_size;
	int64_t gif_size;
};

int gif2bmp(struct gif2bmp_t * status, FILE * in_file, FILE * out_file);

#endif // GIF2BMP_H_
