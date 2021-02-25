#ifndef STB_IMAGE_H
#define STB_IMAGE_H
#include <cstdint>

/* stbi-1.18 - public domain JPEG/PNG reader - http://nothings.org/stb_image.c
                      when you control the images you're loading

   QUICK NOTES:
      Primarily of interest to game developers and other people who can
          avoid problematic images and only need the trivial interface

      JPEG baseline (no JPEG progressive, no oddball channel decimations)
      PNG 8-bit only
      BMP non-1bpp, non-RLE
      TGA (not sure what subset, if a subset)
      PSD (composited view only, no extra channels)
      HDR (radiance rgbE format)
      writes BMP,TGA (define STBI_NO_WRITE to remove code)
      decoded from memory or through stdio FILE (define STBI_NO_STDIO to remove code)
      supports installable dequantizing-IDCT, YCbCr-to-RGB conversion (define STBI_SIMD)
        
   TODO:
      stbi_info_*
  
   history:
      1.18   fix a threading bug (local mutable static)
      1.17   support interlaced PNG
      1.16   major bugfix - convert_format converted one too many pixels
      1.15   initialize some fields for thread safety
      1.14   fix threadsafe conversion bug; header-file-only version (#define STBI_HEADER_FILE_ONLY before including)
      1.13   threadsafe
      1.12   const qualifiers in the API
      1.11   Support installable IDCT, colorspace conversion routines
      1.10   Fixes for 64-bit (don't use "unsigned long")
             optimized upsampling by Fabian "ryg" Giesen
      1.09   Fix format-conversion for PSD code (bad global variables!)
      1.08   Thatcher Ulrich's PSD code integrated by Nicolas Schulz
      1.07   attempt to fix C++ warning/errors again
      1.06   attempt to fix C++ warning/errors again
      1.05   fix TGA loading to return correct *comp and use good luminance calc
      1.04   default float alpha is 1, not 255; use 'void *' for stbi_image_free
      1.03   bugfixes to STBI_NO_STDIO, STBI_NO_HDR
      1.02   support for (subset of) HDR files, float interface for preferred access to them
      1.01   fix bug: possible bug in handling right-side up bmps... not sure
             fix bug: the stbi_bmp_load() and stbi_tga_load() functions didn't work at all
      1.00   interface to zlib that skips zlib header
      0.99   correct handling of alpha in palette
      0.98   TGA loader by lonesock; dynamically add loaders (untested)
      0.97   jpeg errors on too large a file; also catch another malloc failure
      0.96   fix detection of invalid v value - particleman@mollyrocket forum
      0.95   during header scan, seek to markers in case of padding
      0.94   STBI_NO_STDIO to disable stdio usage; rename all #defines the same
      0.93   handle jpegtran output; verbose errors
      0.92   read 4,8,16,24,32-bit BMP files of several formats
      0.91   output 24-bit Windows 3.0 BMP files
      0.90   fix a few more warnings; bump version number to approach 1.0
      0.61   bugfixes due to Marc LeBlanc, Christopher Lloyd
      0.60   fix compiling as c++
      0.59   fix warnings: merge Dave Moore's -Wall fixes
      0.58   fix bug: zlib uncompressed mode len/nlen was wrong endian
      0.57   fix bug: jpg last huffman symbol before marker was >9 bits but less
                      than 16 available
      0.56   fix bug: zlib uncompressed mode len vs. nlen
      0.55   fix bug: restart_interval not initialized to 0
      0.54   allow NULL for 'int *comp'
      0.53   fix bug in png 3->4; speedup png decoding
      0.52   png handles req_comp=3,4 directly; minor cleanup; jpeg comments
      0.51   obey req_comp requests, 1-component jpegs return as 1-component,
             on 'test' only check type, not whether we support this variant
*/


#ifndef STBI_INCLUDE_STB_IMAGE_H
#define STBI_INCLUDE_STB_IMAGE_H

////   begin header file  ////////////////////////////////////////////////////
//
// Limitations:
//    - no progressive/interlaced support (jpeg, png)
//    - 8-bit samples only (jpeg, png)
//    - not threadsafe
//    - channel subsampling of at most 2 in each dimension (jpeg)
//    - no delayed line count (jpeg) -- IJG doesn't support either
//
// Basic usage (see HDR discussion below):
//    int x,y,n;
//    unsigned char *data = stbi_load(filename, &x, &y, &n, 0);
//    // ... process data if not NULL ... 
//    // ... x = width, y = height, n = # 8-bit components per pixel ...
//    // ... replace '0' with '1'..'4' to force that many components per pixel
//    stbi_image_free(data)
//
// Standard parameters:
//    int *x       -- outputs image width in pixels
//    int *y       -- outputs image height in pixels
//    int *comp    -- outputs # of image components in image file
//    int req_comp -- if non-zero, # of image components requested in result
//
// The return value from an image loader is an 'unsigned char *' which points
// to the pixel data. The pixel data consists of *y scanlines of *x pixels,
// with each pixel consisting of N interleaved 8-bit components; the first
// pixel pointed to is top-left-most in the image. There is no padding between
// image scanlines or between pixels, regardless of format. The number of
// components N is 'req_comp' if req_comp is non-zero, or *comp otherwise.
// If req_comp is non-zero, *comp has the number of components that _would_
// have been output otherwise. E.g. if you set req_comp to 4, you will always
// get RGBA output, but you can check *comp to easily see if it's opaque.
//
// An output image with N components has the following components interleaved
// in this order in each pixel:
//
//     N=#comp     components
//       1           grey
//       2           grey, alpha
//       3           red, green, blue
//       4           red, green, blue, alpha
//
// If image loading fails for any reason, the return value will be NULL,
// and *x, *y, *comp will be unchanged. The function stbi_failure_reason()
// can be queried for an extremely brief, end-user unfriendly explanation
// of why the load failed. Define STBI_NO_FAILURE_STRINGS to avoid
// compiling these strings at all, and STBI_FAILURE_USERMSG to get slightly
// more user-friendly ones.
//
// Paletted PNG and BMP images are automatically depalettized.
//
//
// ===========================================================================
//
// HDR image support   (disable by defining STBI_NO_HDR)
//
// stb_image now supports loading HDR images in general, and currently
// the Radiance .HDR file format, although the support is provided
// generically. You can still load any file through the existing interface;
// if you attempt to load an HDR file, it will be automatically remapped to
// LDR, assuming gamma 2.2 and an arbitrary scale factor defaulting to 1;
// both of these constants can be reconfigured through this interface:
//
//     stbi_hdr_to_ldr_gamma(2.2f);
//     stbi_hdr_to_ldr_scale(1.0f);
//
// (note, do not use _inverse_ constants; stbi_image will invert them
// appropriately).
//
// Additionally, there is a new, parallel interface for loading files as
// (linear) floats to preserve the full dynamic range:
//
//    float *data = stbi_loadf(filename, &x, &y, &n, 0);
// 
// If you load LDR images through this interface, those images will
// be promoted to floating point values, run through the inverse of
// constants corresponding to the above:
//
//     stbi_ldr_to_hdr_scale(1.0f);
//     stbi_ldr_to_hdr_gamma(2.2f);
//
// Finally, given a filename (or an open file or memory block--see header
// file for details) containing image data, you can query for the "most
// appropriate" interface to use (that is, whether the image is HDR or
// not), using:
//
//     stbi_is_hdr(char *filename);

#include <stdio.h>

#define STBI_VERSION 1

enum
{
   STBI_default = 0, // only used for req_comp

   STBI_grey       = 1,
   STBI_grey_alpha = 2,
   STBI_rgb        = 3,
   STBI_rgb_alpha  = 4,
};

typedef unsigned char stbi_uc;

#ifdef __cplusplus
extern "C" {
#endif

// WRITING API

// write a BMP/TGA file given tightly packed 'comp' channels (no padding, nor bmp-stride-padding)
// (you must include the appropriate extension in the filename).
// returns TRUE on success, FALSE if couldn't open file, error writing file
extern int      stbi_write_bmp       (char const *filename,     int x, int y, int comp, void *data);
extern int      stbi_write_tga       (char const *filename,     int x, int y, int comp, void *data);

// PRIMARY API - works on images of any type

// load image by filename, open file, or memory buffer
extern stbi_uc *stbi_load            (char const *filename,     int *x, int *y, int *comp, int req_comp);
extern stbi_uc *stbi_load_from_file  (FILE *f,                  int *x, int *y, int *comp, int req_comp);
extern int      stbi_info_from_file  (FILE *f,                  int *x, int *y, int *comp);
extern stbi_uc *stbi_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);
// for stbi_load_from_file, file pointer is left pointing immediately after image


extern float *stbi_loadf            (char const *filename,     int *x, int *y, int *comp, int req_comp);
extern float *stbi_loadf_from_file  (FILE *f,                  int *x, int *y, int *comp, int req_comp);

extern float *stbi_loadf_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);

extern void   stbi_hdr_to_ldr_gamma(float gamma);
extern void   stbi_hdr_to_ldr_scale(float scale);

extern void   stbi_ldr_to_hdr_gamma(float gamma);
extern void   stbi_ldr_to_hdr_scale(float scale);

// get a VERY brief reason for failure
// NOT THREADSAFE
extern const char    *stbi_failure_reason  (void);

// free the loaded image -- this is just free()
extern void     stbi_image_free      (void *retval_from_stbi_load);

// get image dimensions & components without fully decoding
extern int      stbi_info_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp);
extern int      stbi_is_hdr_from_memory(stbi_uc const *buffer, int len);
#ifndef STBI_NO_STDIO
extern int      stbi_info            (char const *filename,     int *x, int *y, int *comp);
extern int      stbi_is_hdr          (char const *filename);
extern int      stbi_is_hdr_from_file(FILE *f);
#endif

// ZLIB client - used by PNG, available for other purposes

extern char *stbi_zlib_decode_malloc_guesssize(const char *buffer, int len, int initial_size, int *outlen);
extern char *stbi_zlib_decode_malloc(const char *buffer, int len, int *outlen);
extern int   stbi_zlib_decode_buffer(char *obuffer, int olen, const char *ibuffer, int ilen);

extern char *stbi_zlib_decode_noheader_malloc(const char *buffer, int len, int *outlen);
extern int   stbi_zlib_decode_noheader_buffer(char *obuffer, int olen, const char *ibuffer, int ilen);

// TYPE-SPECIFIC ACCESS

// is it a jpeg?
extern int      stbi_jpeg_test_memory     (stbi_uc const *buffer, int len);
extern stbi_uc *stbi_jpeg_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);
extern int      stbi_jpeg_info_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp);

#ifndef STBI_NO_STDIO
extern stbi_uc *stbi_jpeg_load            (char const *filename,     int *x, int *y, int *comp, int req_comp);
extern int      stbi_jpeg_test_file       (FILE *f);
extern stbi_uc *stbi_jpeg_load_from_file  (FILE *f,                  int *x, int *y, int *comp, int req_comp);

extern int      stbi_jpeg_info            (char const *filename,     int *x, int *y, int *comp);
extern int      stbi_jpeg_info_from_file  (FILE *f,                  int *x, int *y, int *comp);
#endif

// is it a png?
extern int      stbi_png_test_memory      (stbi_uc const *buffer, int len);
extern stbi_uc *stbi_png_load_from_memory (stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);
extern int      stbi_png_info_from_memory (stbi_uc const *buffer, int len, int *x, int *y, int *comp);

#ifndef STBI_NO_STDIO
extern stbi_uc *stbi_png_load             (char const *filename,     int *x, int *y, int *comp, int req_comp);
extern int      stbi_png_info             (char const *filename,     int *x, int *y, int *comp);
extern int      stbi_png_test_file        (FILE *f);
extern stbi_uc *stbi_png_load_from_file   (FILE *f,                  int *x, int *y, int *comp, int req_comp);
extern int      stbi_png_info_from_file   (FILE *f,                  int *x, int *y, int *comp);
#endif

// is it a bmp?
extern int      stbi_bmp_test_memory      (stbi_uc const *buffer, int len);

extern stbi_uc *stbi_bmp_load             (char const *filename,     int *x, int *y, int *comp, int req_comp);
extern stbi_uc *stbi_bmp_load_from_memory (stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);
#ifndef STBI_NO_STDIO
extern int      stbi_bmp_test_file        (FILE *f);
extern stbi_uc *stbi_bmp_load_from_file   (FILE *f,                  int *x, int *y, int *comp, int req_comp);
#endif

// is it a tga?
extern int      stbi_tga_test_memory      (stbi_uc const *buffer, int len);

extern stbi_uc *stbi_tga_load             (char const *filename,     int *x, int *y, int *comp, int req_comp);
extern stbi_uc *stbi_tga_load_from_memory (stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);
#ifndef STBI_NO_STDIO
extern int      stbi_tga_test_file        (FILE *f);
extern stbi_uc *stbi_tga_load_from_file   (FILE *f,                  int *x, int *y, int *comp, int req_comp);
#endif

// is it a psd?
extern int      stbi_psd_test_memory      (stbi_uc const *buffer, int len);

extern stbi_uc *stbi_psd_load             (char const *filename,     int *x, int *y, int *comp, int req_comp);
extern stbi_uc *stbi_psd_load_from_memory (stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);
#ifndef STBI_NO_STDIO
extern int      stbi_psd_test_file        (FILE *f);
extern stbi_uc *stbi_psd_load_from_file   (FILE *f,                  int *x, int *y, int *comp, int req_comp);
#endif

// is it an hdr?
extern int      stbi_hdr_test_memory      (stbi_uc const *buffer, int len);

extern float *  stbi_hdr_load             (char const *filename,     int *x, int *y, int *comp, int req_comp);
extern float *  stbi_hdr_load_from_memory (stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);
#ifndef STBI_NO_STDIO
extern int      stbi_hdr_test_file        (FILE *f);
extern float *  stbi_hdr_load_from_file   (FILE *f,                  int *x, int *y, int *comp, int req_comp);
#endif

// define new loaders
typedef struct
{
   int       (*test_memory)(stbi_uc const *buffer, int len);
   stbi_uc * (*load_from_memory)(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);
   #ifndef STBI_NO_STDIO
   int       (*test_file)(FILE *f);
   stbi_uc * (*load_from_file)(FILE *f, int *x, int *y, int *comp, int req_comp);
   #endif
} stbi_loader;

// register a loader by filling out the above structure (you must defined ALL functions)
// returns 1 if added or already added, 0 if not added (too many loaders)
// NOT THREADSAFE
extern int stbi_register_loader(stbi_loader *loader);

// define faster low-level operations (typically SIMD support)
#if STBI_SIMD
typedef void (*stbi_idct_8x8)(uint8 *out, int out_stride, short data[64], unsigned short *dequantize);
// compute an integer IDCT on "input"
//     input[x] = data[x] * dequantize[x]
//     write results to 'out': 64 samples, each run of 8 spaced by 'out_stride'
//                             CLAMP results to 0..255
typedef void (*stbi_YCbCr_to_RGB_run)(uint8 *output, uint8 const *y, uint8 const *cb, uint8 const *cr, int count, int step);
// compute a conversion from YCbCr to RGB
//     'count' pixels
//     write pixels to 'output'; each pixel is 'step' bytes (either 3 or 4; if 4, write '255' as 4th), order R,G,B
//     y: Y input channel
//     cb: Cb input channel; scale/biased to be 0..255
//     cr: Cr input channel; scale/biased to be 0..255

extern void stbi_install_idct(stbi_idct_8x8 func);
extern void stbi_install_YCbCr_to_RGB(stbi_YCbCr_to_RGB_run func);
#endif // STBI_SIMD

#ifdef __cplusplus
}
#endif

//
//
////   end header file   /////////////////////////////////////////////////////
#endif // STBI_INCLUDE_STB_IMAGE_H

#ifndef STBI_HEADER_FILE_ONLY

#ifndef STBI_NO_HDR
#include <math.h>  // ldexp
#include <string.h> // strcmp
#endif

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <stdarg.h>


// implementation:
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef   signed short  int16;
typedef unsigned int   uint32;
typedef   signed int    int32;
typedef unsigned int   uint;

// should produce compiler error if size is wrong
typedef unsigned char validate_uint32[sizeof(uint32)==4];

#if defined(STBI_NO_STDIO) && !defined(STBI_NO_WRITE)
#define STBI_NO_WRITE
#endif

//////////////////////////////////////////////////////////////////////////////
//
// Generic API that works on all image types
//

// this is not threadsafe
static const char *failure_reason;

static int e(const char *str)
{
   failure_reason = str;
   return 0;
}

#ifdef STBI_NO_FAILURE_STRINGS
   #define e(x,y)  0
#elif defined(STBI_FAILURE_USERMSG)
   #define e(x,y)  e(y)
#else
   #define e(x,y)  e(x)
#endif

#define epf(x,y)   ((float *) (e(x,y)?NULL:NULL))
#define epuc(x,y)  ((uint8_t *) (e(x,y)?NULL:NULL))




extern int stbi_info(char const *filename,           int *x, int *y, int *comp);
extern int stbi_info_from_file  (FILE *f,                  int *x, int *y, int *comp);
extern int stbi_info_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp);




//////////////////////////////////////////////////////////////////////////////
//
// Common code used by all image loaders
//

enum
{
   SCAN_load=0,
   SCAN_type,
   SCAN_header,
};

typedef struct
{
   uint32 img_x, img_y;
   int img_n, img_out_n;
   FILE  *img_file;
   uint8 *img_buffer, *img_buffer_end;
} stbi;






//////////////////////////////////////////////////////////////////////////////
//
//  "baseline" JPEG/JFIF decoder (not actually fully baseline implementation)
//
//    simple implementation
//      - channel subsampling of at most 2 in each dimension
//      - doesn't support delayed output of y-dimension
//      - simple interface (only one output format: 8-bit interleaved RGB)
//      - doesn't try to recover corrupt jpegs
//      - doesn't allow partial loading, loading multiple at once
//      - still fast on x86 (copying globals into locals doesn't help x86)
//      - allocates lots of intermediate memory (full size of all components)
//        - non-interleaved case requires this anyway
//        - allows good upsampling (see next)
//    high-quality
//      - upsampled channels are bilinearly interpolated, even across blocks
//      - quality integer IDCT derived from IJG's 'slow'
//    performance
//      - fast huffman; reasonable integer IDCT
//      - uses a lot of intermediate memory, could cache poorly
//      - load http://nothings.org/remote/anemones.jpg 3 times on 2.8Ghz P4
//          stb_jpeg:   1.34 seconds (MSVC6, default release build)
//          stb_jpeg:   1.06 seconds (MSVC6, processor = Pentium Pro)
//          IJL11.dll:  1.08 seconds (compiled by intel)
//          IJG 1998:   0.98 seconds (MSVC6, makefile provided by IJG)
//          IJG 1998:   0.95 seconds (MSVC6, makefile + proc=PPro)

// huffman decoding acceleration
#define FAST_BITS   9  // larger handles more cases; smaller stomps less cache

typedef struct
{
   uint8  fast[1 << FAST_BITS];
   // weirdly, repacking this into AoS is a 10% speed loss, instead of a win
   uint16 code[256];
   uint8  values[256];
   uint8  size[257];
   unsigned int maxcode[18];
   int    delta[17];   // old 'firstsymbol' - old 'firstcode'
} huffman;

typedef struct
{
   #if STBI_SIMD
   unsigned short dequant2[4][64];
   #endif
   stbi s;
   huffman huff_dc[4];
   huffman huff_ac[4];
   uint8 dequant[4][64];

// sizes for components, interleaved MCUs
   int img_h_max, img_v_max;
   int img_mcu_x, img_mcu_y;
   int img_mcu_w, img_mcu_h;

// definition of jpeg image component
   struct
   {
      int id;
      int h,v;
      int tq;
      int hd,ha;
      int dc_pred;

      int x,y,w2,h2;
      uint8 *data;
      void *raw_data;
      uint8 *linebuf;
   } img_comp[4];

   uint32         code_buffer; // jpeg entropy-coded buffer
   int            code_bits;   // number of valid bits
   unsigned char  marker;      // marker seen while filling entropy buffer
   int            nomore;      // flag if we saw a marker so must stop

   int scan_n, order[4];
   int restart_interval, todo;
} jpeg;




#define f2f(x)  (int) (((x) * 4096 + 0.5))
#define fsh(x)  ((x) << 12)

// derived from jidctint -- DCT_ISLOW
#define IDCT_1D(s0,s1,s2,s3,s4,s5,s6,s7)       \
   int t0,t1,t2,t3,p1,p2,p3,p4,p5,x0,x1,x2,x3; \
   p2 = s2;                                    \
   p3 = s6;                                    \
   p1 = (p2+p3) * f2f(0.5411961f);             \
   t2 = p1 + p3*f2f(-1.847759065f);            \
   t3 = p1 + p2*f2f( 0.765366865f);            \
   p2 = s0;                                    \
   p3 = s4;                                    \
   t0 = fsh(p2+p3);                            \
   t1 = fsh(p2-p3);                            \
   x0 = t0+t3;                                 \
   x3 = t0-t3;                                 \
   x1 = t1+t2;                                 \
   x2 = t1-t2;                                 \
   t0 = s7;                                    \
   t1 = s5;                                    \
   t2 = s3;                                    \
   t3 = s1;                                    \
   p3 = t0+t2;                                 \
   p4 = t1+t3;                                 \
   p1 = t0+t3;                                 \
   p2 = t1+t2;                                 \
   p5 = (p3+p4)*f2f( 1.175875602f);            \
   t0 = t0*f2f( 0.298631336f);                 \
   t1 = t1*f2f( 2.053119869f);                 \
   t2 = t2*f2f( 3.072711026f);                 \
   t3 = t3*f2f( 1.501321110f);                 \
   p1 = p5 + p1*f2f(-0.899976223f);            \
   p2 = p5 + p2*f2f(-2.562915447f);            \
   p3 = p3*f2f(-1.961570560f);                 \
   p4 = p4*f2f(-0.390180644f);                 \
   t3 += p1+p4;                                \
   t2 += p2+p3;                                \
   t1 += p2+p4;                                \
   t0 += p1+p3;





// in each scan, we'll have scan_n components, and the order
// of the components is specified by order[]
#define RESTART(x)     ((x) >= 0xd0 && (x) <= 0xd7)







// use comparisons since in some cases we handle more than one case (e.g. SOF)
#define DNL(x)         ((x) == 0xdc)
#define SOI(x)         ((x) == 0xd8)
#define EOI(x)         ((x) == 0xd9)
#define SOF(x)         ((x) == 0xc0 || (x) == 0xc1)
#define SOS(x)         ((x) == 0xda)





// static jfif-centered resampling (across block boundaries)

typedef uint8 *(*resample_row_func)(uint8 *out, uint8 *in0, uint8 *in1,
                                    int w, int hs);

#define div4(x) ((uint8) ((x) >> 2))



#define div16(x) ((uint8) ((x) >> 4))





#define float2fixed(x)  ((int) ((x) * 65536 + 0.5))



#if STBI_SIMD
static stbi_YCbCr_to_RGB_run stbi_YCbCr_installed = YCbCr_to_RGB_row;

void stbi_install_YCbCr_to_RGB(stbi_YCbCr_to_RGB_run func)
{
   stbi_YCbCr_installed = func;
}
#endif




typedef struct
{
   resample_row_func resample;
   uint8 *line0,*line1;
   int hs,vs;   // expansion factor in each axis
   int w_lores; // horizontal pixels pre-expansion 
   int ystep;   // how far through vertical expansion we are
   int ypos;    // which pre-expansion row we're on
} stbi_resample;





// @TODO:
#ifndef STBI_NO_STDIO
extern int      stbi_jpeg_info            (char const *filename,           int *x, int *y, int *comp);
extern int      stbi_jpeg_info_from_file  (FILE *f,                  int *x, int *y, int *comp);
#endif
extern int      stbi_jpeg_info_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp);

// public domain zlib decode    v0.2  Sean Barrett 2006-11-18
//    simple implementation
//      - all input must be provided in an upfront buffer
//      - all output is written to a single output buffer (can malloc/realloc)
//    performance
//      - fast huffman

// fast-way is faster to check than jpeg huffman, but slow way is slower
#define ZFAST_BITS  9 // accelerate all cases in default tables
#define ZFAST_MASK  ((1 << ZFAST_BITS) - 1)

// zlib-style huffman encoding
// (jpegs packs from left, zlib from right, so can't share code)
typedef struct
{
   uint16 fast[1 << ZFAST_BITS];
   uint16 firstcode[16];
   int maxcode[17];
   uint16 firstsymbol[16];
   uint8  size[288];
   uint16 value[288]; 
} zhuffman;



// zlib-from-memory implementation for PNG reading
//    because PNG allows splitting the zlib stream arbitrarily,
//    and it's annoying structurally to have PNG call ZLIB call PNG,
//    we require PNG read all the IDATs and combine them into a single
//    memory buffer

typedef struct
{
   uint8 *zbuffer, *zbuffer_end;
   int num_bits;
   uint32 code_buffer;

   char *zout;
   char *zout_start;
   char *zout_end;
   int   z_expandable;

   zhuffman z_length, z_distance;
} zbuf;






// public domain "baseline" PNG decoder   v0.10  Sean Barrett 2006-11-18
//    simple implementation
//      - only 8-bit samples
//      - no CRC checking
//      - allocates lots of intermediate memory
//        - avoids problem of streaming data between subsystems
//        - avoids explicit window management
//    performance
//      - uses stb_zlib, a PD zlib implementation with fast huffman decoding


typedef struct
{
   uint32 length;
   uint32 type;
} chunk;

#define PNG_TYPE(a,b,c,d)  (((a) << 24) + ((b) << 16) + ((c) << 8) + (d))



typedef struct
{
   stbi s;
   uint8 *idata, *expanded, *out;
} png;


enum {
   F_none=0, F_sub=1, F_up=2, F_avg=3, F_paeth=4,
   F_avg_first, F_paeth_first,
};




extern int stbi_png_info_from_file   (FILE *f,                  int *x, int *y, int *comp);
extern int stbi_png_info_from_memory (stbi_uc const *buffer, int len, int *x, int *y, int *comp);





#endif // STBI_HEADER_FILE_ONLY

#endif

