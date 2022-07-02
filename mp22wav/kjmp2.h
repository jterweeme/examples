/******************************************************************************
** kjmp2 -- a minimal MPEG-1/2 Audio Layer II decoder library                **
** version 1.1                                                               **
*******************************************************************************
** Copyright (C) 2006-2013 Martin J. Fiedler <martin.fiedler@gmx.net>        **
**                                                                           **
** This software is provided 'as-is', without any express or implied         **
** warranty. In no event will the authors be held liable for any damages     **
** arising from the use of this software.                                    **
**                                                                           **
** Permission is granted to anyone to use this software for any purpose,     **
** including commercial applications, and to alter it and redistribute it    **
** freely, subject to the following restrictions:                            **
**   1. The origin of this software must not be misrepresented; you must not **
**      claim that you wrote the original software. If you use this software **
**      in a product, an acknowledgment in the product documentation would   **
**      be appreciated but is not required.                                  **
**   2. Altered source versions must be plainly marked as such, and must not **
**      be misrepresented as being the original software.                    **
**   3. This notice may not be removed or altered from any source            **
**      distribution.                                                        **
******************************************************************************/

#ifndef __KJMP2_H__
#define __KJMP2_H__

// GENERAL WARNING: kjmp2 is *NOT* threadsafe!

// kjmp2_context_t: A kjmp2 context record. You don't need to use or modify
// any of the values inside this structure; just pass the whole structure
// to the kjmp2_* functions
typedef struct _kjmp2_context {
    int id;
    int V[2][1024];
    int Voffs;
} kjmp2_context_t;


extern void kjmp2_init(kjmp2_context_t *mp2);


extern int kjmp2_get_sample_rate(const unsigned char *frame);



extern unsigned long kjmp2_decode_frame(
    kjmp2_context_t *mp2,
    const unsigned char *frame,
    signed short *pcm
);

#endif//__KJMP2_H__
