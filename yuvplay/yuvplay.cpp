/*
 *  yuvplay - play YUV data using SDL
 *
 *  Copyright (C) 2000, Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#define INTERNAL_Y4M_LIBCODE_STUFF_QPX

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <SDL.h>
#include <sys/time.h>
#include <assert.h>

#include <string.h>

#include <stdlib.h>
#include <sys/types.h> /* FreeBSD, others - ssize_t */
#include <stdint.h>
#include <inttypes.h>
#include <iostream>

#ifndef _WIN32
#include <unistd.h>
#endif

#define Y4M_MAGIC "YUV4MPEG2"
#define Y4M_FRAME_MAGIC "FRAME"

/* single-character(space) separating tagged fields */
#define Y4M_DELIM " "  

/* max number of characters in a header line
   (including the '\n', but not the '\0') */
#define Y4M_LINE_MAX 256


#define Y4M_FPS_UNKNOWN    { 0, 0 }
#define Y4M_FPS_NTSC_FILM  { 24000, 1001 }
#define Y4M_FPS_FILM       { 24, 1 }
#define Y4M_FPS_PAL        { 25, 1 }
#define Y4M_FPS_NTSC       { 30000, 1001 }
#define Y4M_FPS_30         { 30, 1 }
#define Y4M_FPS_PAL_FIELD  { 50, 1 }
#define Y4M_FPS_NTSC_FIELD { 60000, 1001 }
#define Y4M_FPS_60         { 60, 1 }

#define Y4M_SAR_UNKNOWN        {   0, 0  }

/*  to avoid changing all the places log_level_t is used */
typedef int log_level_t;

#define GNUC_PRINTF( format_idx, arg_idx )    \
  __attribute__((format (printf, format_idx, arg_idx)))

#ifdef INTERNAL_Y4M_LIBCODE_STUFF_QPX
#define Y4MPRIVATIZE(identifier) identifier
#else
#define Y4MPRIVATIZE(identifier) PRIVATE##identifier
#endif


/************************************************************************
 *  error codes returned by y4m_* functions
 ************************************************************************/
#define Y4M_OK          0
#define Y4M_ERR_RANGE   1  /* argument or tag value out of range */
#define Y4M_ERR_SYSTEM  2  /* failed system call, check errno */
#define Y4M_ERR_HEADER  3  /* illegal/malformed header */
#define Y4M_ERR_BADTAG  4  /* illegal tag character */
#define Y4M_ERR_MAGIC   5  /* bad header magic */
#define Y4M_ERR_EOF     6  /* end-of-file (clean) */
#define Y4M_ERR_XXTAGS  7  /* too many xtags */
#define Y4M_ERR_BADEOF  8  /* unexpected end-of-file */
#define Y4M_ERR_FEATURE 9  /* stream requires features beyond allowed level */


/* generic 'unknown' value for integer parameters (e.g. interlace, height) */
#define Y4M_UNKNOWN -1

/************************************************************************
 * values for the "interlace" parameter [y4m_*_interlace()]
 ************************************************************************/
#define Y4M_ILACE_NONE          0   /* non-interlaced, progressive frame */
#define Y4M_ILACE_TOP_FIRST     1   /* interlaced, top-field first       */
#define Y4M_ILACE_BOTTOM_FIRST  2   /* interlaced, bottom-field first    */
#define Y4M_ILACE_MIXED         3   /* mixed, "refer to frame header"    */

/************************************************************************
 * values for the "chroma" parameter [y4m_*_chroma()]
 ************************************************************************/
#define Y4M_CHROMA_420JPEG     0  /* 4:2:0, H/V centered, for JPEG/MPEG-1 */
#define Y4M_CHROMA_420MPEG2    1  /* 4:2:0, H cosited, for MPEG-2         */
#define Y4M_CHROMA_420PALDV    2  /* 4:2:0, alternating Cb/Cr, for PAL-DV */
#define Y4M_CHROMA_444         3  /* 4:4:4, no subsampling, phew.         */
#define Y4M_CHROMA_422         4  /* 4:2:2, H cosited                     */
#define Y4M_CHROMA_411         5  /* 4:1:1, H cosited                     */
#define Y4M_CHROMA_MONO        6  /* luma plane only                      */
#define Y4M_CHROMA_444ALPHA    7  /* 4:4:4 with an alpha channel          */

/************************************************************************
 * values for sampling parameters [y4m_*_spatial(), y4m_*_temporal()]
 ************************************************************************/
#define Y4M_SAMPLING_PROGRESSIVE 0
#define Y4M_SAMPLING_INTERLACED  1

/************************************************************************
 * values for "presentation" parameter [y4m_*_presentation()]
 ************************************************************************/
#define Y4M_PRESENT_TOP_FIRST         0  /* top-field-first                 */
#define Y4M_PRESENT_TOP_FIRST_RPT     1  /* top-first, repeat top           */
#define Y4M_PRESENT_BOTTOM_FIRST      2  /* bottom-field-first              */
#define Y4M_PRESENT_BOTTOM_FIRST_RPT  3  /* bottom-first, repeat bottom     */
#define Y4M_PRESENT_PROG_SINGLE       4  /* single progressive frame        */
#define Y4M_PRESENT_PROG_DOUBLE       5  /* progressive frame, repeat once  */
#define Y4M_PRESENT_PROG_TRIPLE       6  /* progressive frame, repeat twice */

#define Y4M_MAX_NUM_PLANES 4

/************************************************************************
 *  'ratio' datatype, for rational numbers
 *                                     (see 'ratio' functions down below)
 ************************************************************************/
typedef struct _y4m_ratio {
  int n;  /* numerator   */
  int d;  /* denominator */
} y4m_ratio_t;

static constexpr y4m_ratio_t y4m_fps_UNKNOWN    = Y4M_FPS_UNKNOWN;
static constexpr y4m_ratio_t y4m_sar_UNKNOWN        = Y4M_SAR_UNKNOWN;

#define Y4M_MAX_XTAGS 32        /* maximum number of xtags in list       */
#define Y4M_MAX_XTAG_SIZE 32    /* max length of an xtag (including 'X') */

typedef struct _y4m_xtag_list y4m_xtag_list_t;
typedef struct _y4m_stream_info y4m_stream_info_t;
typedef struct _y4m_frame_info y4m_frame_info_t;


typedef struct y4m_cb_reader_s
  {
  void * data;
  ssize_t (*read)(void * data, void *buf, size_t len);
  } y4m_cb_reader_t;

/* quick test of two ratios for equality (i.e. identical components) */
#define Y4M_RATIO_EQL(a,b) ( ((a).n == (b).n) && ((a).d == (b).d) )

/* quick conversion of a ratio to a double (no divide-by-zero check!) */
#define Y4M_RATIO_DBL(r) ((double)(r).n / (double)(r).d)




struct _y4m_xtag_list {
  int Y4MPRIVATIZE(count);
  char *Y4MPRIVATIZE(tags)[Y4M_MAX_XTAGS];
};

struct _y4m_stream_info {
  /* values from header/setters */
  int Y4MPRIVATIZE(width);
  int Y4MPRIVATIZE(height);
  int Y4MPRIVATIZE(interlace);            /* see Y4M_ILACE_* definitions  */
  y4m_ratio_t Y4MPRIVATIZE(framerate);    /* see Y4M_FPS_* definitions    */
  y4m_ratio_t Y4MPRIVATIZE(sampleaspect);
  int Y4MPRIVATIZE(chroma);

  /* mystical X tags */
  y4m_xtag_list_t Y4MPRIVATIZE(x_tags);
};


/************************************************************************
 *  'frame_info' --- frame header information
 *
 *     Do not touch this structure directly!
 *
 *     Use the y4m_fi_*() functions (see below).
 *     You must initialize/finalize this structure before/after use.
 ************************************************************************/

struct _y4m_frame_info {
  int Y4MPRIVATIZE(spatial);      /* see Y4M_SAMPLING_* definitions */
  int Y4MPRIVATIZE(temporal);     /* see Y4M_SAMPLING_* definitions */
  int Y4MPRIVATIZE(presentation); /* see Y4M_PRESENT_* definitions  */
  /* mystical X tags */
  y4m_xtag_list_t Y4MPRIVATIZE(x_tags);
};


#undef Y4MPRIVATIZE


typedef struct {
  char h, m, s, f;
} MPEG_timecode_t;

typedef unsigned int mpeg_framerate_code_t;
typedef unsigned int mpeg_aspect_code_t;

static SDL_Surface *screen;
static SDL_Overlay *yuv_overlay;
static SDL_Rect rect;

static int gcd(int a, int b)
{
  a = (a >= 0) ? a : -a;
  b = (b >= 0) ? b : -b;

  while (b > 0) {
    int x = b;
    b = a % b;
    a = x;
  }
  return a;
}
    
static void y4m_ratio_reduce(y4m_ratio_t *r)
{
  int d;
  if ((r->n == 0) && (r->d == 0)) return;  /* "unknown" */
  d = gcd(r->n, r->d);
  r->n /= d;
  r->d /= d;
}

#ifdef HAVE___PROGNAME
extern const char *__progname;
#endif

#define LOG_DEBUG 1
#define LOG_INFO 2
#define LOG_WARN 3
#define LOG_ERROR 4

static log_level_t mjpeg_log_verbosity = 0;

typedef int(*mjpeg_log_filter_t)(log_level_t level);
typedef void(*mjpeg_log_handler_t)(log_level_t level, const char message[]);

static int mjpeg_default_handler_verbosity(int verbosity)
{
  int prev_verb = mjpeg_log_verbosity;
  mjpeg_log_verbosity = (log_level_t)(LOG_WARN - verbosity);
  return prev_verb;
}

static ssize_t y4m_read(int fd, void *buf, size_t len)
{
   ssize_t n;
   uint8_t *ptr = (uint8_t *)buf;

   while (len > 0) {
     n = read(fd, ptr, len);
     if (n <= 0) {
       /* return amount left to read */
       if (n == 0)
     return len;  /* n == 0 --> eof */
       else
     return -len; /* n < 0 --> error */
     }
     ptr += n;
     len -= n;
   }
   return 0;
}

static int y4m_read_frame_data_cb(y4m_cb_reader_t * fd, const y4m_stream_info_t *si, 
                           y4m_frame_info_t *fi, uint8_t * const *planes);

static int y4m_read_frame(int fd, const y4m_stream_info_t *si, 
           y4m_frame_info_t *fi, uint8_t * const *planes);

static int y4m_read_frame_cb(y4m_cb_reader_t * fd, const y4m_stream_info_t *si, 
                      y4m_frame_info_t *fi, uint8_t * const *planes);

static int y4m_parse_ratio(y4m_ratio_t *r, const char *s)
{
  const char *t = strchr(s, ':');
  if (t == NULL) return Y4M_ERR_RANGE;
  r->n = atoi(s);
  r->d = atoi(t+1);
  if (r->d < 0) return Y4M_ERR_RANGE;
  /* 0:0 == unknown, so that is ok, otherwise zero denominator is bad */
  if ((r->d == 0) && (r->n != 0)) return Y4M_ERR_RANGE;
  y4m_ratio_reduce(r);
  return Y4M_OK;
}

/* small enough to distinguish 1/1000 from 1/1001 */
#define MPEG_FPS_TOLERANCE 0.0001

static ssize_t y4m_read_fd(void * data, void *buf, size_t len)
{
  int * f = (int*)data;
  return y4m_read(*f, buf, len);
}

static void set_cb_reader_from_fd(y4m_cb_reader_t * ret, int * fd)
{
  ret->read = y4m_read_fd;
  ret->data = fd;
}

static int _y4mparam_feature_level = 0;       /* default is ol YUV4MPEG2 */

static int y4m_xtag_clearlist(y4m_xtag_list_t *xtags)
{
  xtags->count = 0;
  return Y4M_OK;
}



static void y4m_clear_stream_info(y4m_stream_info_t *info)
{
  if (info == NULL) return;
  /* clear/initialize info */
  info->width = Y4M_UNKNOWN;
  info->height = Y4M_UNKNOWN;
  info->interlace = Y4M_UNKNOWN;
  info->framerate = y4m_fps_UNKNOWN;
  info->sampleaspect = y4m_sar_UNKNOWN;
  if (_y4mparam_feature_level < 1) {
    info->chroma = Y4M_CHROMA_420JPEG;
  } else {
    info->chroma = Y4M_UNKNOWN;
  }
  y4m_xtag_clearlist(&(info->x_tags));
}

static ssize_t y4m_read_cb(y4m_cb_reader_t * fd, void *buf, size_t len);
static int y4m_parse_stream_tags(char *s, y4m_stream_info_t *i);

#if 1
static int
y4m_read_stream_header_line_cb(y4m_cb_reader_t *fd, y4m_stream_info_t *i, char *line,int n)  
{     
    int err;
    
    /* start with a clean slate */
    y4m_clear_stream_info(i);
    /* read the header line */
    for (; n < Y4M_LINE_MAX; n++) {
        if (y4m_read_cb(fd, line+n, 1))
            return Y4M_ERR_SYSTEM;
        if (line[n] == '\n') {
            line[n] = '\0';           /* Replace linefeed by end of string */
            break;
        }
    }
    /* look for keyword in header */
    if (strncmp(line, Y4M_MAGIC, strlen(Y4M_MAGIC)))
        return Y4M_ERR_MAGIC;
    if (n >= Y4M_LINE_MAX)
        return Y4M_ERR_HEADER;
    if ((err = y4m_parse_stream_tags(line + strlen(Y4M_MAGIC), i)) != Y4M_OK)
        return err;

    return Y4M_OK; 
}
#endif



static int y4m_read_stream_header_cb(y4m_cb_reader_t *fd, y4m_stream_info_t *i)
{
    char line[Y4M_LINE_MAX];
    return y4m_read_stream_header_line_cb(fd,i,line,0);
}

static int y4m_read_stream_header(int fd, y4m_stream_info_t *i)
{
  y4m_cb_reader_t r;
  set_cb_reader_from_fd(&r, &fd);
  return y4m_read_stream_header_cb(&r, i);
}

static int y4m_si_get_plane_width(const y4m_stream_info_t *si, int plane);
static int y4m_si_get_plane_height(const y4m_stream_info_t *si, int plane);

static int y4m_read_frame_data_cb(y4m_cb_reader_t * fd, const y4m_stream_info_t *si, 
                        y4m_frame_info_t *fi, uint8_t * const *frame)
{
    for (int p = 0; p < 3; ++p)
    {
        int w = y4m_si_get_plane_width(si, p);
        int h = y4m_si_get_plane_height(si, p);
        if (y4m_read_cb(fd, frame[p], w*h))
            return Y4M_ERR_SYSTEM;
    }
    return Y4M_OK;
}





static int y4m_read_frame(int fd, const y4m_stream_info_t *si,
           y4m_frame_info_t *fi, uint8_t * const *frame)
{
    y4m_cb_reader_t r;
    set_cb_reader_from_fd(&r, &fd);
    return y4m_read_frame_cb(&r, si, fi, frame);
}
    
static int y4m_si_get_plane_width(const y4m_stream_info_t *si, int plane)
{   
    switch (plane) {
    case 0:
    return (si->width);
    case 1:
    case 2:
    switch (si->chroma) {
    case Y4M_CHROMA_420JPEG:
    case Y4M_CHROMA_420MPEG2:
    case Y4M_CHROMA_420PALDV:
      return (si->width) / 2;
    case Y4M_CHROMA_444:
    case Y4M_CHROMA_444ALPHA:
      return (si->width);
    case Y4M_CHROMA_422:
      return (si->width) / 2;
    case Y4M_CHROMA_411:
      return (si->width) / 4;
    default:
      return Y4M_UNKNOWN;
    } 
    case 3:
        switch (si->chroma)
        {
        case Y4M_CHROMA_444ALPHA:
            return si->width;
        default:
            return Y4M_UNKNOWN;
        }
        default:
            return Y4M_UNKNOWN;
    }
}

static int y4m_si_get_plane_height(const y4m_stream_info_t *si, int plane)
{ 
  switch (plane) {
  case 0:
    return (si->height);
  case 1:
  case 2:
    switch (si->chroma) {
    case Y4M_CHROMA_420JPEG: 
    case Y4M_CHROMA_420MPEG2:
    case Y4M_CHROMA_420PALDV:
      return (si->height) / 2;
    case Y4M_CHROMA_444:
    case Y4M_CHROMA_444ALPHA:
    case Y4M_CHROMA_422:
    case Y4M_CHROMA_411:
      return (si->height);
    default: 
      return Y4M_UNKNOWN;
    }
  case 3:
    switch (si->chroma) {
    case Y4M_CHROMA_444ALPHA:
      return (si->height);
    default: 
      return Y4M_UNKNOWN;
    }
  default: 
    return Y4M_UNKNOWN;
  }
}

static int y4m_accept_extensions(int level)
{
  int old = _y4mparam_feature_level;
  if (level >= 0)
    _y4mparam_feature_level = level;
  return old;
}

static int y4m_read_frame_header_cb(y4m_cb_reader_t * fd, const y4m_stream_info_t *si,
                             y4m_frame_info_t *fi);

static ssize_t y4m_read_cb(y4m_cb_reader_t * fd, void *buf, size_t len)
  {
  return fd->read(fd->data, buf, len);
  }

static int y4m_si_get_width(const y4m_stream_info_t *si)
{ return si->width; }

static int y4m_si_get_height(const y4m_stream_info_t *si)
{ return si->height; }

static y4m_ratio_t y4m_si_get_sampleaspect(const y4m_stream_info_t *si)
{ return si->sampleaspect; }

static int y4m_si_get_chroma(const y4m_stream_info_t *si)
{ return si->chroma; }

static void y4m_clear_frame_info(y4m_frame_info_t *info)
{
  if (info == NULL) return;
  /* clear/initialize info */
  info->spatial = Y4M_UNKNOWN;
  info->temporal = Y4M_UNKNOWN;
  info->presentation = Y4M_UNKNOWN;
  y4m_xtag_clearlist(&(info->x_tags));
}

static int y4m_parse_stream_tags(char *s, y4m_stream_info_t *i)
{
    char *token, *value;
    char tag;
    int err;

    /* parse fields */
    for (token = strtok(s, Y4M_DELIM); 
       token != NULL; 
       token = strtok(NULL, Y4M_DELIM))
    {
        if (token[0] == '\0')
            continue;   /* skip empty strings */

        tag = token[0];
        value = token + 1;
        switch (tag)
        {
        case 'W':  /* width */
            i->width = atoi(value);
            if (i->width <= 0)
                return Y4M_ERR_RANGE;
            break;
        case 'H':  /* height */
            i->height = atoi(value); 
            if (i->height <= 0)
                return Y4M_ERR_RANGE;
            break;
        case 'F':  /* frame rate (fps) */
            if ((err = y4m_parse_ratio(&(i->framerate), value)) != Y4M_OK)
                return err;
            if (i->framerate.n < 0)
                return Y4M_ERR_RANGE;
            break;
        case 'I':  /* interlacing */
            switch (value[0]) {
            case 'p':  i->interlace = Y4M_ILACE_NONE; break;
            case 't':  i->interlace = Y4M_ILACE_TOP_FIRST; break;
            case 'b':  i->interlace = Y4M_ILACE_BOTTOM_FIRST; break;
            case 'm':  i->interlace = Y4M_ILACE_MIXED; break;
            case '?':
            default:
                i->interlace = Y4M_UNKNOWN;
                break;
        }
        break;
        case 'A':  /* sample (pixel) aspect ratio */
        if ((err = y4m_parse_ratio(&(i->sampleaspect), value)) != Y4M_OK)
        return err;
        if (i->sampleaspect.n < 0) return Y4M_ERR_RANGE;
        break;
        case 'C':
            i->chroma = Y4M_CHROMA_420JPEG;
            if (i->chroma == Y4M_UNKNOWN)
                return Y4M_ERR_HEADER;
        break;
        default:
        break;
        }
    }

  /* Without 'C' tag or any other chroma spec, default to 420jpeg */
  if (i->chroma == Y4M_UNKNOWN) 
    i->chroma = Y4M_CHROMA_420JPEG;

  /* Error checking... */
  /*      - Width and Height are required. */
  if ((i->width == Y4M_UNKNOWN) || (i->height == Y4M_UNKNOWN))
    return Y4M_ERR_HEADER;
  /*      - Non-420 chroma and mixed interlace require level >= 1 */
  if (_y4mparam_feature_level < 1) {
    if ((i->chroma != Y4M_CHROMA_420JPEG) &&
    (i->chroma != Y4M_CHROMA_420MPEG2) &&
    (i->chroma != Y4M_CHROMA_420PALDV))
      return Y4M_ERR_FEATURE;
    if (i->interlace == Y4M_ILACE_MIXED)
      return Y4M_ERR_FEATURE;
  }

  /* ta da!  done. */
  return Y4M_OK;
}

#if 1
static int y4m_reread_stream_header_line_cb(y4m_cb_reader_t *fd,const y4m_stream_info_t *si,char *line,int n)
{
    y4m_stream_info_t i;
    int err=y4m_read_stream_header_line_cb(fd,&i,line,n);
    return err;
}
#endif

#if 1
static int y4m_read_frame_header_cb(y4m_cb_reader_t * fd,
              const y4m_stream_info_t *si,
              y4m_frame_info_t *fi)
{
  char line[Y4M_LINE_MAX];
  char *p;
  int n;
  ssize_t remain;

 again:  
  /* start with a clean slate */
  y4m_clear_frame_info(fi);
  /* This is more clever than read_stream_header...
     Try to read "FRAME\n" all at once, and don't try to parse
     if nothing else is there...
  */
  remain = y4m_read_cb(fd, line, sizeof(Y4M_FRAME_MAGIC)-1+1); /* -'\0', +'\n' */
  if (remain < 0) return Y4M_ERR_SYSTEM;
  if (remain > 0) {
    /* A clean EOF should end exactly at a frame-boundary */
    if (remain == sizeof(Y4M_FRAME_MAGIC))
      return Y4M_ERR_EOF;
    else
      return Y4M_ERR_BADEOF;
  }
  if (strncmp(line, Y4M_FRAME_MAGIC, sizeof(Y4M_FRAME_MAGIC)-1)) {
      int err=y4m_reread_stream_header_line_cb(fd,si,line,sizeof(Y4M_FRAME_MAGIC)-1+1);
      if( err!=Y4M_OK )
          return err;
      goto again;
  }
  if (line[sizeof(Y4M_FRAME_MAGIC)-1] == '\n')
    return Y4M_OK; /* done -- no tags:  that was the end-of-line. */

  if (line[sizeof(Y4M_FRAME_MAGIC)-1] != Y4M_DELIM[0]) {
    return Y4M_ERR_MAGIC; /* wasn't a space -- what was it? */
  }

  /* proceed to get the tags... (overwrite the magic) */
  for (n = 0, p = line; n < Y4M_LINE_MAX; n++, p++) {
    if (y4m_read_cb(fd, p, 1))
      return Y4M_ERR_SYSTEM;
    if (*p == '\n') {
      *p = '\0';           /* Replace linefeed by end of string */
      break;
    }
  }
  if (n >= Y4M_LINE_MAX) return Y4M_ERR_HEADER;
  /* non-zero on error */
    return Y4M_OK;
}
#endif

static int y4m_read_frame_cb(y4m_cb_reader_t * fd, const y4m_stream_info_t *si,
           y4m_frame_info_t *fi, uint8_t * const *frame)
{
    int err;

    if ((err = y4m_read_frame_header_cb(fd, si, fi)) != Y4M_OK)
        return err;
    
    return y4m_read_frame_data_cb(fd, si, fi, frame);
}

int main(int argc, char *argv[])
{
    int verbosity = 1;
    struct timeval time_now;
    int n, frame;
    unsigned char *yuv[3];
    int in_fd = 0;
    int screenwidth=0, screenheight=0;
    y4m_stream_info_t streaminfo;
    y4m_frame_info_t frameinfo;
    int frame_width;
    int frame_height;
    mjpeg_default_handler_verbosity(verbosity);
    y4m_accept_extensions(1);
    y4m_clear_stream_info(&streaminfo);

    if ((n = y4m_read_stream_header(in_fd, &streaminfo)) != Y4M_OK)
    {
      exit (1);
    }

    y4m_si_get_chroma(&streaminfo);
    frame_width = y4m_si_get_width(&streaminfo);
    frame_height = y4m_si_get_height(&streaminfo);

    /* no user supplied screen size, so let's use the stream info */
    y4m_ratio_t aspect = y4m_si_get_sampleaspect(&streaminfo);
       
    if (!(Y4M_RATIO_EQL(aspect, y4m_sar_UNKNOWN))) {
       /* if pixel aspect ratio present, use it */
       /* scale width, but maintain height (line count) */
       screenheight = frame_height;
       screenwidth = frame_width * aspect.n / aspect.d;
    } else {
       /* unknown aspect ratio -- assume square pixels */
       screenwidth = frame_width;
       screenheight = frame_height;
    }

    /* Initialize the SDL library */
    if( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
      exit(1);
    }


    /* yuv params */
    yuv[0] = (uint8_t *)malloc(frame_width * frame_height * sizeof(unsigned char));
    yuv[1] = (uint8_t *)malloc(frame_width * frame_height / 4 * sizeof(unsigned char));
    yuv[2] = (uint8_t *)malloc(frame_width * frame_height / 4 * sizeof(unsigned char));
    screen = SDL_SetVideoMode(screenwidth, screenheight, 0, SDL_SWSURFACE);
    yuv_overlay = SDL_CreateYUVOverlay(frame_width, frame_height, SDL_YV12_OVERLAY, screen);
    rect.x = 0;
    rect.y = 0;
    rect.w = screenwidth;
    rect.h = screenheight;

    SDL_DisplayYUVOverlay(yuv_overlay, &rect);

    frame = 0;

    gettimeofday(&time_now,0);

    while ((n = y4m_read_frame(in_fd, &streaminfo, &frameinfo, yuv)) == Y4M_OK)
    {
      /* Lock SDL_yuv_overlay */
      if ( SDL_MUSTLOCK(screen) ) {
         if ( SDL_LockSurface(screen) < 0 ) break;
      }
      if (SDL_LockYUVOverlay(yuv_overlay) < 0) break;

      /* let's draw the data (*yuv[3]) on a SDL screen (*screen) */
      memcpy(yuv_overlay->pixels[0], yuv[0], frame_width * frame_height);
      memcpy(yuv_overlay->pixels[1], yuv[2], frame_width * frame_height / 4);
      memcpy(yuv_overlay->pixels[2], yuv[1], frame_width * frame_height / 4);

      /* Unlock SDL_yuv_overlay */
      if ( SDL_MUSTLOCK(screen) ) {
         SDL_UnlockSurface(screen);
      }
      SDL_UnlockYUVOverlay(yuv_overlay);

      /* Show, baby, show! */
      SDL_DisplayYUVOverlay(yuv_overlay, &rect);
  
      frame++;

      gettimeofday(&time_now,0);
   }

   for (n=0; n<3; n++) {
      free(yuv[n]);
   }

   SDL_FreeYUVOverlay(yuv_overlay);
   SDL_Quit();
   return 0;
}




