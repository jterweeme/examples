/*
 *  yuv4mpeg.c:  Functions for reading and writing "new" YUV4MPEG streams
 *
 *  Copyright (C) 2001 Matthew J. Marjanovic <maddog@mir.com>
 *
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
 *
 */

#include "config.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define INTERNAL_Y4M_LIBCODE_STUFF_QPX
#include "yuv4mpeg.h"
#include "yuv4mpeg_intern.h"


static int _y4mparam_allow_unknown_tags = 1;  /* default is forgiveness */
static int _y4mparam_feature_level = 0;       /* default is ol YUV4MPEG2 */

static void *(*_y4m_alloc)(size_t bytes) = malloc;
static void (*_y4m_free)(void *ptr) = free;


int y4m_allow_unknown_tags(int yn)
{
  int old = _y4mparam_allow_unknown_tags;
  if (yn >= 0)
    _y4mparam_allow_unknown_tags = (yn) ? 1 : 0;
  return old;
}

int y4m_accept_extensions(int level)
{
  int old = _y4mparam_feature_level;
  if (level >= 0)
    _y4mparam_feature_level = level;
  return old;
}


/*************************************************************************
 *
 * Convenience functions for fd read/write
 *
 *   - guaranteed to transfer entire payload (or fail)
 *   - returns:
 *               0 on complete success
 *               +(# of remaining bytes) on eof (for y4m_read)
 *               -(# of rem. bytes) on error (and ERRNO should be set)
 *     
 *************************************************************************/

ssize_t y4m_read(int fd, void *buf, size_t len)
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

ssize_t y4m_write(int fd, const void *buf, size_t len)
{
   ssize_t n;
   const uint8_t *ptr = (const uint8_t *)buf;

   while (len > 0) {
     n = write(fd, ptr, len);
     if (n <= 0) return -len;  /* return amount left to write */
     ptr += n;
     len -= n;
   }
   return 0;
}

/* read len bytes from fd into buf */
ssize_t y4m_read_cb(y4m_cb_reader_t * fd, void *buf, size_t len)
  {
  return fd->read(fd->data, buf, len);
  }

/* write len bytes from fd into buf */
ssize_t y4m_write_cb(y4m_cb_writer_t * fd, const void *buf, size_t len)
  {
  return fd->write(fd->data, buf, len);
  }

/* Functions to use the callback interface from plain filedescriptors */

/* read len bytes from fd into buf */
#if 1
ssize_t y4m_read_fd(void * data, void *buf, size_t len)
  {
  int * f = (int*)data;
  return y4m_read(*f, buf, len);
  }

/* write len bytes from fd into buf */
ssize_t y4m_write_fd(void * data, const void *buf, size_t len)
  {
  int * f = (int*)data;
  return y4m_write(*f, buf, len);
  }
#endif

static void set_cb_reader_from_fd(y4m_cb_reader_t * ret, int * fd)
  {
  ret->read = y4m_read_fd;
  ret->data = fd;
  }

static char *y4m_new_xtag(void)
{
    return _y4m_alloc(Y4M_MAX_XTAG_SIZE * sizeof(char));
}

void y4m_init_xtag_list(y4m_xtag_list_t *xtags)
{
    xtags->count = 0;
    for (int i = 0; i < Y4M_MAX_XTAGS; i++)
        xtags->tags[i] = NULL;
}

void y4m_fini_xtag_list(y4m_xtag_list_t *xtags)
{
    for (int i = 0; i < Y4M_MAX_XTAGS; i++)
    {
        if (xtags->tags[i] != NULL)
        {
            _y4m_free(xtags->tags[i]);
            xtags->tags[i] = NULL;
        }
    }
    xtags->count = 0;
}

void y4m_copy_xtag_list(y4m_xtag_list_t *dest, const y4m_xtag_list_t *src)
{
  int i;
  for (i = 0; i < src->count; i++) {
    if (dest->tags[i] == NULL) 
      dest->tags[i] = y4m_new_xtag();
    strncpy(dest->tags[i], src->tags[i], Y4M_MAX_XTAG_SIZE);
  }
  dest->count = src->count;
}

static int y4m_snprint_xtags(char *s, int maxn, const y4m_xtag_list_t *xtags)
{
  int i, room;
  
  for (i = 0, room = maxn - 1; i < xtags->count; i++) {
    int n = snprintf(s, room + 1, " %s", xtags->tags[i]);
    if ((n < 0) || (n > room)) return Y4M_ERR_HEADER;
    s += n;
    room -= n;
  }
  s[0] = '\n';  /* finish off header with newline */
  s[1] = '\0';  /* ...and end-of-string           */
  return Y4M_OK;
}

int y4m_xtag_count(const y4m_xtag_list_t *xtags)
{
  return xtags->count;
}

const char *y4m_xtag_get(const y4m_xtag_list_t *xtags, int n)
{
  if (n >= xtags->count)
    return NULL;
  else
    return xtags->tags[n];
}

int y4m_xtag_add(y4m_xtag_list_t *xtags, const char *tag)
{
  if (xtags->count >= Y4M_MAX_XTAGS) return Y4M_ERR_XXTAGS;
  if (xtags->tags[xtags->count] == NULL) 
    xtags->tags[xtags->count] = y4m_new_xtag();
  strncpy(xtags->tags[xtags->count], tag, Y4M_MAX_XTAG_SIZE);
  (xtags->count)++;
  return Y4M_OK;
}

int y4m_xtag_remove(y4m_xtag_list_t *xtags, int n)
{
  int i;
  char *q;

  if ((n < 0) || (n >= xtags->count)) return Y4M_ERR_RANGE;
  q = xtags->tags[n];
  for (i = n; i < (xtags->count - 1); i++)
    xtags->tags[i] = xtags->tags[i+1];
  xtags->tags[i] = q;
  (xtags->count)--;
  return Y4M_OK;
}

int y4m_xtag_clearlist(y4m_xtag_list_t *xtags)
{
  xtags->count = 0;
  return Y4M_OK;
}

int y4m_xtag_addlist(y4m_xtag_list_t *dest, const y4m_xtag_list_t *src)
{
  int i, j;

  if ((dest->count + src->count) > Y4M_MAX_XTAGS) return Y4M_ERR_XXTAGS;
  for (i = dest->count, j = 0;
       j < src->count;
       i++, j++) {
    if (dest->tags[i] == NULL) 
      dest->tags[i] = y4m_new_xtag();
    strncpy(dest->tags[i], src->tags[i], Y4M_MAX_XTAG_SIZE);
  }
  dest->count += src->count;
  return Y4M_OK;
}  

void y4m_init_stream_info(y4m_stream_info_t *info)
{
  if (info == NULL) return;
  /* init substructures */
  y4m_init_xtag_list(&(info->x_tags));
  /* set defaults */
  y4m_clear_stream_info(info);
}

void y4m_clear_stream_info(y4m_stream_info_t *info)
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

void y4m_copy_stream_info(y4m_stream_info_t *dest,
			  const y4m_stream_info_t *src)
{
  if ((dest == NULL) || (src == NULL)) return;
  /* copy info */
  dest->width = src->width;
  dest->height = src->height;
  dest->interlace = src->interlace;
  dest->framerate = src->framerate;
  dest->sampleaspect = src->sampleaspect;
  dest->chroma = src->chroma;
  y4m_copy_xtag_list(&(dest->x_tags), &(src->x_tags));
}

// returns 0 if equal, nonzero if different
#if 1
static int y4m_compare_stream_info(const y4m_stream_info_t *s1,const y4m_stream_info_t *s2)
{
    int i,j;

    if( s1->width          != s2->width          ||
        s1->height         != s2->height         ||
        s1->interlace      != s2->interlace      ||
        s1->framerate.n    != s2->framerate.n    ||
        s1->framerate.d    != s2->framerate.d    ||
        s1->sampleaspect.n != s2->sampleaspect.n ||
        s1->sampleaspect.d != s2->sampleaspect.d ||
        s1->chroma         != s2->chroma         ||
        s1->x_tags.count   != s2->x_tags.count   )
        return 1;

    // the tags may not be in the same order
    for( i=0; i<s1->x_tags.count; i++ ) {
        for( j=0; j<s2->x_tags.count; j++ )
            if( !strncmp(s1->x_tags.tags[i],s2->x_tags.tags[j],Y4M_MAX_XTAG_SIZE) )
                goto next;
        return 1;
    next:;
    }
    return 0;
}
#endif

void y4m_fini_stream_info(y4m_stream_info_t *info)
{
  if (info == NULL) return;
  y4m_fini_xtag_list(&(info->x_tags));
}

void y4m_si_set_width(y4m_stream_info_t *si, int width)
{
  si->width = width;
}

int y4m_si_get_width(const y4m_stream_info_t *si)
{ return si->width; }

void y4m_si_set_height(y4m_stream_info_t *si, int height)
{
  si->height = height; 
}

int y4m_si_get_height(const y4m_stream_info_t *si)
{ return si->height; }

void y4m_si_set_interlace(y4m_stream_info_t *si, int interlace)
{ si->interlace = interlace; }

int y4m_si_get_interlace(const y4m_stream_info_t *si)
{ return si->interlace; }

void y4m_si_set_framerate(y4m_stream_info_t *si, y4m_ratio_t framerate)
{ si->framerate = framerate; }

y4m_ratio_t y4m_si_get_framerate(const y4m_stream_info_t *si)
{ return si->framerate; }

void y4m_si_set_sampleaspect(y4m_stream_info_t *si, y4m_ratio_t sar)
{ si->sampleaspect = sar; }

y4m_ratio_t y4m_si_get_sampleaspect(const y4m_stream_info_t *si)
{ return si->sampleaspect; }

void y4m_si_set_chroma(y4m_stream_info_t *si, int chroma_mode)
{ si->chroma = chroma_mode; }

int y4m_si_get_chroma(const y4m_stream_info_t *si)
{ return si->chroma; }

int y4m_si_get_plane_length(const y4m_stream_info_t *si, int plane)
{
  int w = y4m_si_get_plane_width(si, plane);
  int h = y4m_si_get_plane_height(si, plane);
  if ((w != Y4M_UNKNOWN) && (h != Y4M_UNKNOWN))
    return (w * h);
  else
    return Y4M_UNKNOWN;
}

int y4m_si_get_framelength(const y4m_stream_info_t *si)
{
  int total = 0;
  int planes = y4m_si_get_plane_count(si);
  int p;
  for (p = 0; p < planes; p++) {
    int plen = y4m_si_get_plane_length(si, p);
    if (plen == Y4M_UNKNOWN) return Y4M_UNKNOWN;
    total += plen;
  }
  return total;
}

y4m_xtag_list_t *y4m_si_xtags(y4m_stream_info_t *si)
{ return &(si->x_tags); }


void y4m_init_frame_info(y4m_frame_info_t *info)
{
  if (info == NULL) return;
  /* init substructures */
  y4m_init_xtag_list(&(info->x_tags));
  /* set defaults */
  y4m_clear_frame_info(info);
}

void y4m_clear_frame_info(y4m_frame_info_t *info)
{
  if (info == NULL) return;
  /* clear/initialize info */
  info->spatial = Y4M_UNKNOWN;
  info->temporal = Y4M_UNKNOWN;
  info->presentation = Y4M_UNKNOWN;
  y4m_xtag_clearlist(&(info->x_tags));
}

void y4m_copy_frame_info(y4m_frame_info_t *dest, const y4m_frame_info_t *src)
{
  if ((dest == NULL) || (src == NULL)) return;
  /* copy info */
  dest->spatial = src->spatial;
  dest->temporal = src->temporal;
  dest->presentation = src->presentation;
  y4m_copy_xtag_list(&(dest->x_tags), &(src->x_tags));
}

void y4m_fini_frame_info(y4m_frame_info_t *info)
{
  if (info == NULL) return;
  y4m_fini_xtag_list(&(info->x_tags));
}

void y4m_fi_set_presentation(y4m_frame_info_t *fi, int pres)
{ fi->presentation = pres; }

int y4m_fi_get_presentation(const y4m_frame_info_t *fi)
{ return fi->presentation; }

void y4m_fi_set_temporal(y4m_frame_info_t *fi, int sampling)
{ fi->temporal = sampling; }

int y4m_fi_get_temporal(const y4m_frame_info_t *fi)
{ return fi->temporal; }

void y4m_fi_set_spatial(y4m_frame_info_t *fi, int sampling)
{ fi->spatial = sampling; }

int y4m_fi_get_spatial(const y4m_frame_info_t *fi)
{ return fi->spatial; }

y4m_xtag_list_t *y4m_fi_xtags(y4m_frame_info_t *fi)
{ return &(fi->x_tags); }


/*************************************************************************
 *
 * Tag parsing 
 *
 *************************************************************************/

int y4m_parse_stream_tags(char *s, y4m_stream_info_t *i)
{
  char *token, *value;
  char tag;
  int err;

  /* parse fields */
  for (token = strtok(s, Y4M_DELIM); 
       token != NULL; 
       token = strtok(NULL, Y4M_DELIM)) {
    if (token[0] == '\0') continue;   /* skip empty strings */
    tag = token[0];
    value = token + 1;
    switch (tag) {
    case 'W':  /* width */
      i->width = atoi(value);
      if (i->width <= 0) return Y4M_ERR_RANGE;
      break;
    case 'H':  /* height */
      i->height = atoi(value); 
      if (i->height <= 0) return Y4M_ERR_RANGE;
      break;
    case 'F':  /* frame rate (fps) */
      if ((err = y4m_parse_ratio(&(i->framerate), value)) != Y4M_OK)
	return err;
      if (i->framerate.n < 0) return Y4M_ERR_RANGE;
      break;
    case 'I':  /* interlacing */
      switch (value[0]) {
      case 'p':  i->interlace = Y4M_ILACE_NONE; break;
      case 't':  i->interlace = Y4M_ILACE_TOP_FIRST; break;
      case 'b':  i->interlace = Y4M_ILACE_BOTTOM_FIRST; break;
      case 'm':  i->interlace = Y4M_ILACE_MIXED; break;
      case '?':
      default:
	i->interlace = Y4M_UNKNOWN; break;
      }
      break;
    case 'A':  /* sample (pixel) aspect ratio */
      if ((err = y4m_parse_ratio(&(i->sampleaspect), value)) != Y4M_OK)
	return err;
      if (i->sampleaspect.n < 0) return Y4M_ERR_RANGE;
      break;
    case 'C':
      i->chroma = y4m_chroma_parse_keyword(value);
      if (i->chroma == Y4M_UNKNOWN)
	return Y4M_ERR_HEADER;
      break;
    case 'X':  /* 'X' meta-tag */
      if ((err = y4m_xtag_add(&(i->x_tags), token)) != Y4M_OK) return err;
      break;
    default:
      /* possible error on unknown options */
      if (_y4mparam_allow_unknown_tags) {
	/* unknown tags ok:  store in xtag list and warn... */
	if ((err = y4m_xtag_add(&(i->x_tags), token)) != Y4M_OK) return err;
	mjpeg_warn("Unknown stream tag encountered:  '%s'", token);
      } else {
	/* unknown tags are *not* ok */
	return Y4M_ERR_BADTAG;
      }
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



static int y4m_parse_frame_tags(char *s, const y4m_stream_info_t *si,
				y4m_frame_info_t *fi)
{
  char *token, *value;
  char tag;
  int err;

  /* parse fields */
  for (token = strtok(s, Y4M_DELIM); 
       token != NULL; 
       token = strtok(NULL, Y4M_DELIM)) {
    if (token[0] == '\0') continue;   /* skip empty strings */
    tag = token[0];
    value = token + 1;
    switch (tag) {
    case 'I':
      /* frame 'I' tag requires feature level >= 1 */
      if (_y4mparam_feature_level < 1) return Y4M_ERR_FEATURE;
      if (si->interlace != Y4M_ILACE_MIXED) return Y4M_ERR_BADTAG;
      switch (value[0]) {
      case 't':  fi->presentation = Y4M_PRESENT_TOP_FIRST;        break;
      case 'T':  fi->presentation = Y4M_PRESENT_TOP_FIRST_RPT;    break;
      case 'b':  fi->presentation = Y4M_PRESENT_BOTTOM_FIRST;     break;
      case 'B':  fi->presentation = Y4M_PRESENT_BOTTOM_FIRST_RPT; break;
      case '1':  fi->presentation = Y4M_PRESENT_PROG_SINGLE;      break;
      case '2':  fi->presentation = Y4M_PRESENT_PROG_DOUBLE;      break;
      case '3':  fi->presentation = Y4M_PRESENT_PROG_TRIPLE;      break;
      default: 
	return Y4M_ERR_BADTAG;
      }
      switch (value[1]) {
      case 'p':  fi->temporal = Y4M_SAMPLING_PROGRESSIVE; break;
      case 'i':  fi->temporal = Y4M_SAMPLING_INTERLACED;  break;
      default: 
	return Y4M_ERR_BADTAG;
      }
      switch (value[2]) {
      case 'p':  fi->spatial = Y4M_SAMPLING_PROGRESSIVE; break;
      case 'i':  fi->spatial = Y4M_SAMPLING_INTERLACED;  break;
      case '?':  fi->spatial = Y4M_UNKNOWN;              break;
      default: 
	return Y4M_ERR_BADTAG;
      }
      break;
    case 'X':  /* 'X' meta-tag */
      if ((err = y4m_xtag_add(&(fi->x_tags), token)) != Y4M_OK) return err;
      break;
    default:
      /* possible error on unknown options */
      if (_y4mparam_allow_unknown_tags) {
	/* unknown tags ok:  store in xtag list and warn... */
	if ((err = y4m_xtag_add(&(fi->x_tags), token)) != Y4M_OK) return err;
	mjpeg_warn("Unknown frame tag encountered:  '%s'", token);
      } else {
	/* unknown tags are *not* ok */
	return Y4M_ERR_BADTAG;
      }
      break;
    }
  }
  /* error-checking and/or non-mixed defaults */
  switch (si->interlace) {
  case Y4M_ILACE_MIXED:
    /* T and P are required if stream "Im" */
    if ((fi->presentation == Y4M_UNKNOWN) || (fi->temporal == Y4M_UNKNOWN))
      return Y4M_ERR_HEADER;
    /* and S is required if stream is also 4:2:0 */
    if ( ((si->chroma == Y4M_CHROMA_420JPEG) ||
          (si->chroma == Y4M_CHROMA_420MPEG2) ||
          (si->chroma == Y4M_CHROMA_420PALDV)) &&
         (fi->spatial == Y4M_UNKNOWN) )
      return Y4M_ERR_HEADER;
    break;
  case Y4M_ILACE_NONE:
    /* stream "Ip" --> equivalent to frame "I1pp" */
    fi->spatial = Y4M_SAMPLING_PROGRESSIVE;
    fi->temporal = Y4M_SAMPLING_PROGRESSIVE;
    fi->presentation = Y4M_PRESENT_PROG_SINGLE;
    break;
  case Y4M_ILACE_TOP_FIRST:
    /* stream "It" --> equivalent to frame "Itii" */
    fi->spatial = Y4M_SAMPLING_INTERLACED;
    fi->temporal = Y4M_SAMPLING_INTERLACED;
    fi->presentation = Y4M_PRESENT_TOP_FIRST;
    break;
  case Y4M_ILACE_BOTTOM_FIRST:
    /* stream "Ib" --> equivalent to frame "Ibii" */
    fi->spatial = Y4M_SAMPLING_INTERLACED;
    fi->temporal = Y4M_SAMPLING_INTERLACED;
    fi->presentation = Y4M_PRESENT_BOTTOM_FIRST;
    break;
  default:
    /* stream unknown:  then, whatever */
    break;
  }
  /* ta da!  done. */
  return Y4M_OK;
}

#if 1
static int y4m_read_stream_header_line_cb(y4m_cb_reader_t * fd, y4m_stream_info_t *i,char *line,int n)
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

static int y4m_reread_stream_header_line_cb(y4m_cb_reader_t *fd,const y4m_stream_info_t *si,char *line,int n)
{
    y4m_stream_info_t i;
    int err=y4m_read_stream_header_line_cb(fd,&i,line,n);
    if( err==Y4M_OK && y4m_compare_stream_info(si,&i) )
        err=Y4M_ERR_HEADER;
    y4m_fini_stream_info(&i);
    return err;
}

int y4m_write_stream_header_cb(y4m_cb_writer_t * fd, const y4m_stream_info_t *i)
{
  char s[Y4M_LINE_MAX+1];
  int n;
  int err;
  y4m_ratio_t rate = i->framerate;
  y4m_ratio_t aspect = i->sampleaspect;
  const char *chroma_keyword = y4m_chroma_keyword(i->chroma);

  if ((i->chroma == Y4M_UNKNOWN) || (chroma_keyword == NULL))
    return Y4M_ERR_HEADER;
  if (_y4mparam_feature_level < 1) {
    if ((i->chroma != Y4M_CHROMA_420JPEG) &&
	(i->chroma != Y4M_CHROMA_420MPEG2) &&
	(i->chroma != Y4M_CHROMA_420PALDV))
      return Y4M_ERR_FEATURE;
    if (i->interlace == Y4M_ILACE_MIXED)
      return Y4M_ERR_FEATURE;
  }
  y4m_ratio_reduce(&rate);
  y4m_ratio_reduce(&aspect);
  n = snprintf(s, sizeof(s), "%s W%d H%d F%d:%d I%s A%d:%d C%s",
	       Y4M_MAGIC,
	       i->width,
	       i->height,
	       rate.n, rate.d,
	       (i->interlace == Y4M_ILACE_NONE) ? "p" :
	       (i->interlace == Y4M_ILACE_TOP_FIRST) ? "t" :
	       (i->interlace == Y4M_ILACE_BOTTOM_FIRST) ? "b" :
	       (i->interlace == Y4M_ILACE_MIXED) ? "m" : "?",
	       aspect.n, aspect.d,
	       chroma_keyword
	       );
  if ((n < 0) || (n > Y4M_LINE_MAX)) return Y4M_ERR_HEADER;
  if ((err = y4m_snprint_xtags(s + n, sizeof(s) - n - 1, &(i->x_tags))) 
      != Y4M_OK) 
    return err;
  /* non-zero on error */
  return (y4m_write_cb(fd, s, strlen(s)) ? Y4M_ERR_SYSTEM : Y4M_OK);
}

#if 1
int y4m_read_frame_header_cb(y4m_cb_reader_t * fd,
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
  return y4m_parse_frame_tags(line, si, fi);
}
#endif

int y4m_read_frame_header(int fd,
			  const y4m_stream_info_t *si,
			  y4m_frame_info_t *fi)
  {
  y4m_cb_reader_t r;
  set_cb_reader_from_fd(&r, &fd);
  return y4m_read_frame_header_cb(&r, si, fi);
  }

int y4m_write_frame_header_cb(y4m_cb_writer_t * fd,
			   const y4m_stream_info_t *si,
			   const y4m_frame_info_t *fi)
{
  char s[Y4M_LINE_MAX+1];
  int n, err;

  if (si->interlace == Y4M_ILACE_MIXED) {
    if (_y4mparam_feature_level < 1) return Y4M_ERR_FEATURE;
    n = snprintf(s, sizeof(s), "%s I%c%c%c", Y4M_FRAME_MAGIC,
		 (fi->presentation == Y4M_PRESENT_TOP_FIRST)        ? 't' :
		 (fi->presentation == Y4M_PRESENT_TOP_FIRST_RPT)    ? 'T' :
		 (fi->presentation == Y4M_PRESENT_BOTTOM_FIRST)     ? 'b' :
		 (fi->presentation == Y4M_PRESENT_BOTTOM_FIRST_RPT) ? 'B' :
		 (fi->presentation == Y4M_PRESENT_PROG_SINGLE) ? '1' :
		 (fi->presentation == Y4M_PRESENT_PROG_DOUBLE) ? '2' :
		 (fi->presentation == Y4M_PRESENT_PROG_TRIPLE) ? '3' :
		 '?',
		 (fi->temporal == Y4M_SAMPLING_PROGRESSIVE) ? 'p' :
		 (fi->temporal == Y4M_SAMPLING_INTERLACED)  ? 'i' :
		 '?',
		 (fi->spatial == Y4M_SAMPLING_PROGRESSIVE) ? 'p' :
		 (fi->spatial == Y4M_SAMPLING_INTERLACED)  ? 'i' :
		 '?'
		 );
  } else {
    n = snprintf(s, sizeof(s), "%s", Y4M_FRAME_MAGIC);
  }
  
  if ((n < 0) || (n > Y4M_LINE_MAX)) return Y4M_ERR_HEADER;
  if ((err = y4m_snprint_xtags(s + n, sizeof(s) - n - 1, &(fi->x_tags))) 
      != Y4M_OK) 
    return err;
  /* non-zero on error */
  return (y4m_write_cb(fd, s, strlen(s)) ? Y4M_ERR_SYSTEM : Y4M_OK);
}

int y4m_read_fields_data_cb(y4m_cb_reader_t * fd, const y4m_stream_info_t *si,
                         y4m_frame_info_t *fi,
                         uint8_t * const *upper_field, 
                         uint8_t * const *lower_field)
{
  int p;
  int planes = y4m_si_get_plane_count(si);
  const int maxrbuf=32*1024;
  uint8_t *rbuf=_y4m_alloc(maxrbuf);
  int rbufpos=0,rbuflen=0;
  
  /* Read each plane */
  for (p = 0; p < planes; p++) {
    uint8_t *dsttop = upper_field[p];
    uint8_t *dstbot = lower_field[p];
    int height = y4m_si_get_plane_height(si, p);
    int width = y4m_si_get_plane_width(si, p);
    int y;
    /* alternately read one line into each field */
    for (y = 0; y < height; y += 2) {
      if( width*2 >= maxrbuf ) {
        if (y4m_read_cb(fd, dsttop, width)) goto y4merr;
        if (y4m_read_cb(fd, dstbot, width)) goto y4merr;
      } else {
        if( rbufpos==rbuflen ) {
          rbuflen=(height-y)*width;
          if( rbuflen>maxrbuf )
            rbuflen=maxrbuf-maxrbuf%(2*width);
          if( y4m_read_cb(fd,rbuf,rbuflen) )
            goto y4merr;
          rbufpos=0;
        }
            
        memcpy(dsttop,rbuf+rbufpos,width); rbufpos+=width;
        memcpy(dstbot,rbuf+rbufpos,width); rbufpos+=width;
      }
      dsttop+=width;
      dstbot+=width;
    }
  }
  _y4m_free(rbuf);
  return Y4M_OK;

 y4merr:
  _y4m_free(rbuf);
  return Y4M_ERR_SYSTEM;
}

#if 1
int y4m_write_fields_cb(y4m_cb_writer_t * fd, const y4m_stream_info_t *si,
		     const y4m_frame_info_t *fi,
		     uint8_t * const *upper_field, 
		     uint8_t * const *lower_field)
{
  int p, err;
  int planes = y4m_si_get_plane_count(si);
  int numwbuf=0;
  const int maxwbuf=32*1024;
  uint8_t *wbuf;
  
  /* Write frame header */
  if ((err = y4m_write_frame_header_cb(fd, si, fi)) != Y4M_OK) return err;
  /* Write each plane */
  wbuf=_y4m_alloc(maxwbuf);
  for (p = 0; p < planes; p++) {
    uint8_t *srctop = upper_field[p];
    uint8_t *srcbot = lower_field[p];
    int height = y4m_si_get_plane_height(si, p);
    int width = y4m_si_get_plane_width(si, p);
    int y;
    /* alternately write one line from each field */
    for (y = 0; y < height; y += 2) {
      if( width*2 >= maxwbuf ) {
        if (y4m_write_cb(fd, srctop, width)) goto y4merr;
        if (y4m_write_cb(fd, srcbot, width)) goto y4merr;
      } else {
        if (numwbuf + 2 * width > maxwbuf) {
          if(y4m_write_cb(fd, wbuf, numwbuf)) goto y4merr;
          numwbuf=0;
        }

        memcpy(wbuf+numwbuf,srctop,width); numwbuf += width;
        memcpy(wbuf+numwbuf,srcbot,width); numwbuf += width;
      }
      srctop  += width;
      srcbot  += width;
    }
  }
  if( numwbuf )
    if( y4m_write_cb(fd, wbuf, numwbuf) )
      goto y4merr;
  _y4m_free(wbuf);
  return Y4M_OK;

 y4merr:
  _y4m_free(wbuf);
  return Y4M_ERR_SYSTEM;
}
#endif






