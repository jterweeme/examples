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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define INTERNAL_Y4M_LIBCODE_STUFF_QPX

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "yuv4mpeg.h"
//#include "mpegtimecode.h"
#include <SDL.h>
#include <sys/time.h>
#include <assert.h>
#include "config.h"

#include <string.h>
#include "yuv4mpeg.h"
#include "yuv4mpeg_intern.h"

typedef struct {
  char h, m, s, f;
} MPEG_timecode_t;

extern int dropframetimecode;
extern int mpeg_timecode(MPEG_timecode_t *tc, int f, int fpscode, double fps);


typedef unsigned int mpeg_framerate_code_t;
typedef unsigned int mpeg_aspect_code_t;

y4m_ratio_t mpeg_framerate( mpeg_framerate_code_t code );
int mpeg_valid_framerate_code( mpeg_framerate_code_t code );
mpeg_framerate_code_t mpeg_framerate_code( y4m_ratio_t framerate );
y4m_ratio_t mpeg_conform_framerate( double fps );
y4m_ratio_t mpeg_aspect_ratio( int mpeg_version,  mpeg_aspect_code_t code );
int mpeg_valid_aspect_code( int mpeg_version, mpeg_aspect_code_t code );
mpeg_aspect_code_t mpeg_frame_aspect_code( int mpeg_version, y4m_ratio_t aspect_ratio );
const char * mpeg_aspect_code_definition( int mpeg_version,  mpeg_aspect_code_t code  );
const char *mpeg_framerate_code_definition( mpeg_framerate_code_t code  );
const char *mpeg_interlace_code_definition( int yuv4m_interlace_code );

mpeg_aspect_code_t 
mpeg_guess_mpeg_aspect_code(int mpeg_version, y4m_ratio_t sampleaspect,
                int frame_width, int frame_height);


y4m_ratio_t mpeg_guess_sample_aspect_ratio(int mpeg_version,
                   mpeg_aspect_code_t code,
                   int frame_width, int frame_height);
                   
const char *mpeg_format_code_defintion( int format_code );

/* SDL variables */
SDL_Surface *screen;
SDL_Overlay *yuv_overlay;
SDL_Rect rect;

static int got_sigint = 0;

static void usage (void) {
  fprintf(stderr, "Usage: lavpipe/lav2yuv... | yuvplay [options]\n"
	  "  -s : display size, width x height\n"
	  "  -t : set window title\n"
	  "  -f : frame rate (overrides rate in stream header)\n"
          "  -c : don't sync on frames - plays at stream speed\n"
	  "  -v : verbosity {0, 1, 2} [default: 1]\n"
	  );
}

static void sigint_handler (int signal) {
   mjpeg_warn("Caught SIGINT, exiting...");
   got_sigint = 1;
}

static long get_time_diff(struct timeval time_now) {
   struct timeval time_now2;
   gettimeofday(&time_now2,0);
   return time_now2.tv_sec*1.e6 - time_now.tv_sec*1.e6 + time_now2.tv_usec - time_now.tv_usec;
}

static char *print_status(int frame, double framerate) {
   MPEG_timecode_t tc;
   static char temp[256];

   mpeg_timecode(&tc, frame,
		 mpeg_framerate_code(mpeg_conform_framerate(framerate)),
		 framerate);
   sprintf(temp, "%d:%2.2d:%2.2d.%2.2d", tc.h, tc.m, tc.s, tc.f);
   return temp;
}

int main(int argc, char *argv[])
{
   int verbosity = 1;
   double time_between_frames = 0.0;
   double frame_rate = 0.0;
   struct timeval time_now;
   int n, frame;
   unsigned char *yuv[3];
   int in_fd = 0;
   int screenwidth=0, screenheight=0;
   y4m_stream_info_t streaminfo;
   y4m_frame_info_t frameinfo;
   int frame_width;
   int frame_height;
   int wait_for_sync = 1;
   char *window_title = NULL;

   while ((n = getopt(argc, argv, "hs:t:f:cv:")) != EOF) {
      switch (n) {
         case 'c':
            wait_for_sync = 0;
            break;
         case 's':
            if (sscanf(optarg, "%dx%d", &screenwidth, &screenheight) != 2) {
               mjpeg_error_exit1( "-s option needs two arguments: -s 10x10");
               exit(1);
            }
            break;
	  case 't':
	    window_title = optarg;
	    break;
	  case 'f':
		  frame_rate = atof(optarg);
		  if( frame_rate <= 0.0 || frame_rate > 200.0 )
			  mjpeg_error_exit1( "-f option needs argument > 0.0 and < 200.0");
		  break;
          case 'v':
	    verbosity = atoi(optarg);
	    if ((verbosity < 0) || (verbosity > 2))
	      mjpeg_error_exit1("-v needs argument from {0, 1, 2} (not %d)",
				verbosity);
	    break;
	  case 'h':
	  case '?':
            usage();
            exit(1);
            break;
         default:
            usage();
            exit(1);
      }
   }

   mjpeg_default_handler_verbosity(verbosity);

   y4m_accept_extensions(1);
   y4m_init_stream_info(&streaminfo);
   y4m_init_frame_info(&frameinfo);
   if ((n = y4m_read_stream_header(in_fd, &streaminfo)) != Y4M_OK) {
      mjpeg_error("Couldn't read YUV4MPEG2 header: %s!",
         y4m_strerr(n));
      exit (1);
   }

   switch (y4m_si_get_chroma(&streaminfo)) {
   case Y4M_CHROMA_420JPEG:
   case Y4M_CHROMA_420MPEG2:
   case Y4M_CHROMA_420PALDV:
     break;
   default:
     mjpeg_error_exit1("Cannot handle non-4:2:0 streams yet!");
   }

   frame_width = y4m_si_get_width(&streaminfo);
   frame_height = y4m_si_get_height(&streaminfo);

   if ((screenwidth <= 0) || (screenheight <= 0)) {
     /* no user supplied screen size, so let's use the stream info */
     y4m_ratio_t aspect = y4m_si_get_sampleaspect(&streaminfo);
       
     if (!(Y4M_RATIO_EQL(aspect, y4m_sar_UNKNOWN))) {
       /* if pixel aspect ratio present, use it */
#if 1
       /* scale width, but maintain height (line count) */
       screenheight = frame_height;
       screenwidth = frame_width * aspect.n / aspect.d;
#else
       if ((frame_width * aspect.d) < (frame_height * aspect.n)) {
	 screenwidth = frame_width;
	 screenheight = frame_width * aspect.d / aspect.n;
       } else {
	 screenheight = frame_height;
	 screenwidth = frame_height * aspect.n / aspect.d;
       }
#endif
     } else {
       /* unknown aspect ratio -- assume square pixels */
       screenwidth = frame_width;
       screenheight = frame_height;
     }
   }

   /* Initialize the SDL library */
   if( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
      mjpeg_error("Couldn't initialize SDL: %s", SDL_GetError());
      exit(1);
   }

   /* set window title */
   SDL_WM_SetCaption(window_title, NULL);

   /* yuv params */
   yuv[0] = malloc(frame_width * frame_height * sizeof(unsigned char));
   yuv[1] = malloc(frame_width * frame_height / 4 * sizeof(unsigned char));
   yuv[2] = malloc(frame_width * frame_height / 4 * sizeof(unsigned char));

   screen = SDL_SetVideoMode(screenwidth, screenheight, 0, SDL_SWSURFACE);
   if ( screen == NULL ) {
      mjpeg_error("SDL: Couldn't set %dx%d: %s",
		  screenwidth, screenheight, SDL_GetError());
      exit(1);
   }
   else {
      mjpeg_debug("SDL: Set %dx%d @ %d bpp",
		  screenwidth, screenheight, screen->format->BitsPerPixel);
   }

   /* since IYUV ordering is not supported by Xv accel on maddog's system
    *  (Matrox G400 --- although, the alias I420 is, but this is not
    *  recognized by SDL), we use YV12 instead, which is identical,
    *  except for ordering of Cb and Cr planes...
    * we swap those when we copy the data to the display buffer...
    */
   yuv_overlay = SDL_CreateYUVOverlay(frame_width, frame_height,
				      SDL_YV12_OVERLAY,
				      screen);
   if ( yuv_overlay == NULL ) {
      mjpeg_error("SDL: Couldn't create SDL_yuv_overlay: %s",
		      SDL_GetError());
      exit(1);
   }
   if ( yuv_overlay->hw_overlay ) 
     mjpeg_debug("SDL: Using hardware overlay.");

   rect.x = 0;
   rect.y = 0;
   rect.w = screenwidth;
   rect.h = screenheight;

   SDL_DisplayYUVOverlay(yuv_overlay, &rect);

   signal (SIGINT, sigint_handler);

   frame = 0;
   if ( frame_rate == 0.0 ) 
   {
	   /* frame rate has not been set from command-line... */
	   if (Y4M_RATIO_EQL(y4m_fps_UNKNOWN, y4m_si_get_framerate(&streaminfo))) {
	     mjpeg_info("Frame-rate undefined in stream... assuming 25Hz!" );
	     frame_rate = 25.0;
	   } else {
	     frame_rate = Y4M_RATIO_DBL(y4m_si_get_framerate(&streaminfo));
	   }
   }
   time_between_frames = 1.e6 / frame_rate;

   gettimeofday(&time_now,0);

   while ((n = y4m_read_frame(in_fd, &streaminfo, &frameinfo, yuv)) == Y4M_OK && (!got_sigint)) {

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
      mjpeg_info("Playing frame %4.4d - %s",
		 frame, print_status(frame, frame_rate));

      if (wait_for_sync)
         while(get_time_diff(time_now) < time_between_frames) {
            usleep(1000);
         }
      frame++;

      gettimeofday(&time_now,0);
   }

   if ((n != Y4M_OK) && (n != Y4M_ERR_EOF))
      mjpeg_error("Couldn't read frame: %s", y4m_strerr(n));

   for (n=0; n<3; n++) {
      free(yuv[n]);
   }

   mjpeg_info("Played %4.4d frames (%s)",
	      frame, print_status(frame, frame_rate));

   SDL_FreeYUVOverlay(yuv_overlay);
   SDL_Quit();

   y4m_fini_frame_info(&frameinfo);
   y4m_fini_stream_info(&streaminfo);
   return 0;
}

#ifdef HAVE___PROGNAME
extern const char *__progname;
#endif

#define LOG_DEBUG 1
#define LOG_INFO 2
#define LOG_WARN 3
#define LOG_ERROR 4

static log_level_t mjpeg_log_verbosity = 0;
static char *default_handler_id = NULL;

static int default_mjpeg_log_filter( log_level_t level )
{
  int verb_from_env;
  if( mjpeg_log_verbosity == 0 )
    {
      char *mjpeg_verb_env = getenv("MJPEG_VERBOSITY");
      if( mjpeg_verb_env != NULL )
        {
          verb_from_env = LOG_WARN-atoi(mjpeg_verb_env);
          if( verb_from_env >= LOG_DEBUG && verb_from_env <= LOG_ERROR )
            mjpeg_log_verbosity = (log_level_t)verb_from_env;
        }
    }
  return (level < LOG_WARN && level < mjpeg_log_verbosity);
}

static mjpeg_log_filter_t _filter = default_mjpeg_log_filter;

static void
default_mjpeg_log_handler(log_level_t level, const char message[])
{
  const char *ids;

  if( (*_filter)( level ) )
    return;
  if (default_handler_id != NULL) {
    ids = default_handler_id;
  } else {
#ifdef HAVE___PROGNAME
    ids = __progname;
#else
    ids = "???";
#endif
  }
  switch(level) {
  case LOG_ERROR:
    fprintf(stderr, "**ERROR: [%s] %s\n", ids, message);
    break;
  case LOG_DEBUG:
    fprintf(stderr, "--DEBUG: [%s] %s\n", ids, message);
    break;
  case LOG_WARN:
    fprintf(stderr, "++ WARN: [%s] %s\n", ids, message);
    break;
  case LOG_INFO:
    fprintf(stderr, "   INFO: [%s] %s\n", ids, message);
    break;
  default:
    assert(0);
  }
}

static mjpeg_log_handler_t _handler = default_mjpeg_log_handler;


mjpeg_log_handler_t
mjpeg_log_set_handler(mjpeg_log_handler_t new_handler)
{
  mjpeg_log_handler_t old_handler = _handler;

  _handler = new_handler;
  return old_handler;
}

/***************
 * Set default log handlers degree of verboseity.
 * 0 = quiet, 1 = info, 2 = debug
 *************/

int
mjpeg_default_handler_verbosity(int verbosity)
{
  int prev_verb = mjpeg_log_verbosity;
  mjpeg_log_verbosity = (log_level_t)(LOG_WARN - verbosity);
  return prev_verb;
}

/*
 * Set identifier string used by default handler
 */
int
mjpeg_default_handler_identifier(const char *new_id)
{
  const char *s;
  if (new_id == NULL) {
    if (default_handler_id != NULL)
       free(default_handler_id);
    default_handler_id = NULL;
    return 0;
  }
  /* find basename of new_id (remove any directory prefix) */
  if ((s = strrchr(new_id, '/')) == NULL)
    s = new_id;
  else
    s = s + 1;
  default_handler_id = strdup(s);
  return 0;
}

static void
mjpeg_logv(log_level_t level, const char format[], va_list args)
{
  char buf[1024] = { 0, };

  /* TODO: Original had a re-entrancy error trap to assist bug
     finding.  To make this work with multi-threaded applications a
     lock is needed hence delete.
  */

  vsnprintf(buf, sizeof(buf)-1, format, args);
  _handler(level, buf);
}

void
mjpeg_log(log_level_t level, const char format[], ...)
{
  va_list args;
  va_start (args, format);
  mjpeg_logv(level, format, args);
  va_end (args);
}

void
mjpeg_debug(const char format[], ...)
{
  va_list args;
  va_start (args, format);
  mjpeg_logv(LOG_DEBUG, format, args);
  va_end (args);
}

void
mjpeg_info(const char format[], ...)
{
  va_list args;
  va_start (args, format);
  mjpeg_logv(LOG_INFO, format, args);
  va_end (args);
}

void
mjpeg_warn(const char format[], ...)
{
  va_list args;
  va_start (args, format);
  mjpeg_logv(LOG_WARN, format, args);
  va_end (args);
}

void
mjpeg_error(const char format[], ...)
{
  va_list args;
  va_start (args, format);
  mjpeg_logv(LOG_ERROR, format, args);
  va_end (args);
}

void
mjpeg_error_exit1(const char format[], ...)
{
  va_list args;
  va_start( args, format );
  mjpeg_logv( LOG_ERROR, format, args);
  va_end(args);           
  exit(EXIT_FAILURE);
}

log_level_t
mjpeg_loglev_t(const char *level)
{
    if (strcasecmp("debug", level) == 0) return(LOG_DEBUG);
    else if (strcasecmp("info", level) == 0) return(LOG_INFO);
    else if (strcasecmp("warn", level) == 0) return(LOG_WARN);
    else if (strcasecmp("error", level) == 0) return(LOG_ERROR);
    return(0);
}

/**************************************************************
 * // NTSC DROP FRAME TIMECODE / 29.97fps (SMTPE)
 * //    hh:mm:ss:ff
 * //       hh: 0..
 * //       mm: 0..59
 * //       ss: 0..59
 * //       ff: 0..29 # ss != 0 || mm % 10 == 0
 * //           2..29 # ss == 0 && mm % 10 != 0
 * //
 * // 00:00:00:00 00:00:00:01 00:00:00:02 ... 00:00:00:29
 * // 00:00:01:00 00:00:01:01 00:00:01:02 ... 00:00:01:29
 * //                        :
 * // 00:00:59:00 00:00:59:01 00:00:59:02 ... 00:00:59:29
 * //                         00:01:00:02 ... 00:01:00:29
 * // 00:01:01:00 00:01:01:01 00:01:01:02 ... 00:01:00:29
 * //                        :
 * // 00:01:59:00 00:01:59:01 00:01:59:02 ... 00:01:59:29
 * //                         00:02:00:02 ... 00:02:00:29
 * // 00:02:01:00 00:02:01:01 00:02:01:02 ... 00:02:00:29
 * //                        :
 * //                        :
 * // 00:09:59:00 00:09:59:01 00:09:59:02 ... 00:09:59:29
 * // 00:10:00:00 00:10:00:01 00:10:00:02 ... 00:10:00:29
 * // 00:10:01:00 00:10:01:01 00:10:01:02 ... 00:10:01:29
 * //                        :
 * // 00:10:59:00 00:10:59:01 00:10:59:02 ... 00:10:59:29
 * //                         00:11:00:02 ... 00:11:00:29
 * // 00:11:01:00 00:11:01:01 00:11:01:02 ... 00:11:00:29
 * //                        :
 * //                        :
 * // DROP FRAME / 59.94fps (no any standard)
 * // DROP FRAME / 23.976fps (no any standard)
 ***************************************************************/

int dropframetimecode = -1;

/* mpeg_timecode() return -tc->f on first frame in the minute, tc->f on other. */
int
mpeg_timecode(MPEG_timecode_t *tc, int f, int fpscode, double fps)
{
  static const int ifpss[] = { 0, 24, 24, 25, 30, 30, 50, 60, 60, };
  int h, m, s;

  if (dropframetimecode < 0) {
    char *env = getenv("MJPEG_DROP_FRAME_TIME_CODE");
    dropframetimecode = (env && *env != '0' && *env != 'n' && *env != 'N');
  }
  if (dropframetimecode &&
      0 < fpscode && fpscode + 1 < sizeof ifpss / sizeof ifpss[0] &&
      ifpss[fpscode] == ifpss[fpscode + 1]) {
    int topinmin = 0, k = (30*4) / ifpss[fpscode];
    f *= k;			/* frame# when 119.88fps */
    h = (f / ((10*60*30-18)*4)); /* # of 10min. */
    f %= ((10*60*30-18)*4);	/* frame# in 10min. */
    f -= (2*4);			/* frame# in 10min. - (2*4) */
    m = (f / ((60*30-2)*4));	/* min. in 10min. */
    topinmin = ((f - k) / ((60*30-2)*4) < m);
    m += (h % 6 * 10);		/* min. */
    h /= 6;			/* hour */
    f %= ((60*30-2)*4);		/* frame# in min. - (2*4)*/
    f += (2*4);			/* frame# in min. */
    s = f / (30*4);		/* sec. */
    f %= (30*4);		/* frame# in sec. */
    f /= k;			/* frame# in sec. on original fps */
    tc->f = f;
    if (topinmin)
      f = -f;
  } else {
    int ifps = ((0 < fpscode && fpscode < sizeof ifpss / sizeof ifpss[0])?
		ifpss[fpscode]: (int)(fps + .5));
    s = f / ifps;
    f %= ifps;
    m = s / 60;
    s %= 60;
    h = m / 60;
    m %= 60;
    tc->f = f;
  }
  tc->s = s;
  tc->m = m;
  tc->h = h;
  return f;
}



/* useful list of standard framerates */
const y4m_ratio_t y4m_fps_UNKNOWN    = Y4M_FPS_UNKNOWN;
const y4m_ratio_t y4m_fps_NTSC_FILM  = Y4M_FPS_NTSC_FILM;
const y4m_ratio_t y4m_fps_FILM       = Y4M_FPS_FILM;
const y4m_ratio_t y4m_fps_PAL        = Y4M_FPS_PAL;
const y4m_ratio_t y4m_fps_NTSC       = Y4M_FPS_NTSC;
const y4m_ratio_t y4m_fps_30         = Y4M_FPS_30;
const y4m_ratio_t y4m_fps_PAL_FIELD  = Y4M_FPS_PAL_FIELD;
const y4m_ratio_t y4m_fps_NTSC_FIELD = Y4M_FPS_NTSC_FIELD;
const y4m_ratio_t y4m_fps_60         = Y4M_FPS_60;

/* useful list of standard sample aspect ratios */
const y4m_ratio_t y4m_sar_UNKNOWN        = Y4M_SAR_UNKNOWN;
const y4m_ratio_t y4m_sar_SQUARE         = Y4M_SAR_SQUARE;
const y4m_ratio_t y4m_sar_SQR_ANA_16_9   = Y4M_SAR_SQR_ANA_16_9;
const y4m_ratio_t y4m_sar_NTSC_CCIR601   = Y4M_SAR_NTSC_CCIR601;
const y4m_ratio_t y4m_sar_NTSC_16_9      = Y4M_SAR_NTSC_16_9;
const y4m_ratio_t y4m_sar_NTSC_SVCD_4_3  = Y4M_SAR_NTSC_SVCD_4_3;
const y4m_ratio_t y4m_sar_NTSC_SVCD_16_9 = Y4M_SAR_NTSC_SVCD_16_9;
const y4m_ratio_t y4m_sar_PAL_CCIR601    = Y4M_SAR_PAL_CCIR601;
const y4m_ratio_t y4m_sar_PAL_16_9       = Y4M_SAR_PAL_16_9;
const y4m_ratio_t y4m_sar_PAL_SVCD_4_3   = Y4M_SAR_PAL_SVCD_4_3;
const y4m_ratio_t y4m_sar_PAL_SVCD_16_9  = Y4M_SAR_PAL_SVCD_16_9;

/* useful list of standard display aspect ratios */
const y4m_ratio_t y4m_dar_UNKNOWN        = Y4M_DAR_UNKNOWN;
const y4m_ratio_t y4m_dar_4_3            = Y4M_DAR_4_3;
const y4m_ratio_t y4m_dar_16_9           = Y4M_DAR_16_9;
const y4m_ratio_t y4m_dar_221_100        = Y4M_DAR_221_100;

/*
 *  Euler's algorithm for greatest common divisor
 */

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
    

/*************************************************************************
 *
 * Remove common factors from a ratio
 *
 *************************************************************************/


void y4m_ratio_reduce(y4m_ratio_t *r)
{
  int d;
  if ((r->n == 0) && (r->d == 0)) return;  /* "unknown" */
  d = gcd(r->n, r->d);
  r->n /= d;
  r->d /= d;
}



/*************************************************************************
 *
 * Parse "nnn:ddd" into a ratio
 *
 * returns:         Y4M_OK  - success
 *           Y4M_ERR_RANGE  - range error 
 *
 *************************************************************************/

int y4m_parse_ratio(y4m_ratio_t *r, const char *s)
{
  char *t = strchr(s, ':');
  if (t == NULL) return Y4M_ERR_RANGE;
  r->n = atoi(s);
  r->d = atoi(t+1);
  if (r->d < 0) return Y4M_ERR_RANGE;
  /* 0:0 == unknown, so that is ok, otherwise zero denominator is bad */
  if ((r->d == 0) && (r->n != 0)) return Y4M_ERR_RANGE;
  y4m_ratio_reduce(r);
  return Y4M_OK;
}



/*************************************************************************
 *
 * Guess the true SAR (sample aspect ratio) from a list of commonly 
 * encountered values, given the "suggested" display aspect ratio, and
 * the true frame width and height.
 *
 * Returns y4m_sar_UNKNOWN if no match is found.
 *
 *************************************************************************/

/* this is big enough to accommodate the difference between 720 and 704 */
#define GUESS_ASPECT_TOLERANCE 0.03

y4m_ratio_t y4m_guess_sar(int width, int height, y4m_ratio_t dar)
{
  int i;
  double implicit_sar = (double)(dar.n * height) / (double)(dar.d * width);
  const y4m_ratio_t *sarray[] =
    {
      &y4m_sar_SQUARE,
      &y4m_sar_NTSC_CCIR601,
      &y4m_sar_NTSC_16_9,
      &y4m_sar_NTSC_SVCD_4_3,
      &y4m_sar_NTSC_SVCD_16_9,
      &y4m_sar_PAL_CCIR601,
      &y4m_sar_PAL_16_9,
      &y4m_sar_PAL_SVCD_4_3,
      &y4m_sar_PAL_SVCD_16_9,
      &y4m_sar_UNKNOWN
    };
  for (i = 0; !(Y4M_RATIO_EQL(*(sarray[i]),y4m_sar_UNKNOWN)); i++) {
    double ratio = implicit_sar / Y4M_RATIO_DBL(*(sarray[i]));
    if ( (ratio > (1.0 - GUESS_ASPECT_TOLERANCE)) &&
	 (ratio < (1.0 + GUESS_ASPECT_TOLERANCE)) )
      return *(sarray[i]);
  }
  return y4m_sar_UNKNOWN;
} 

#define MPEG_FORMAT_MPEG1   0
#define MPEG_FORMAT_VCD     1
#define MPEG_FORMAT_VCD_NSR 2
#define MPEG_FORMAT_MPEG2   3
#define MPEG_FORMAT_SVCD     4
#define MPEG_FORMAT_SVCD_NSR 5
#define MPEG_FORMAT_VCD_STILL 6
#define MPEG_FORMAT_SVCD_STILL 7
#define MPEG_FORMAT_DVD_NAV 8
#define MPEG_FORMAT_DVD      9
#define MPEG_FORMAT_ATSC480i 10
#define MPEG_FORMAT_ATSC480p 11
#define MPEG_FORMAT_ATSC720p 12
#define MPEG_FORMAT_ATSC1080i 13

#define MPEG_FORMAT_FIRST 0
#define MPEG_FORMAT_LAST MPEG_FORMAT_ATSC1080i

#define MPEG_STILLS_FORMAT(x) ((x)==MPEG_FORMAT_VCD_STILL||(x)==MPEG_FORMAT_SVCD_STILL)
#define MPEG_ATSC_FORMAT(x) ((x)>=MPEG_FORMAT_ATSC480i && (x)<=MPEG_FORMAT_ATSC1080i)
#define MPEG_HDTV_FORMAT(x) MPEG_ATSC_FORMAT(x)
#define MPEG_SDTV_FORMAT(x) (!MPEG_HDTV_FORMAT(x))


static y4m_ratio_t
mpeg_framerates[] = {
  Y4M_FPS_UNKNOWN,
  Y4M_FPS_NTSC_FILM,
  Y4M_FPS_FILM,
  Y4M_FPS_PAL,
  Y4M_FPS_NTSC,
  Y4M_FPS_30,
  Y4M_FPS_PAL_FIELD,
  Y4M_FPS_NTSC_FIELD,
  Y4M_FPS_60
};


#define MPEG_NUM_RATES (sizeof(mpeg_framerates)/sizeof(mpeg_framerates[0]))
static const mpeg_framerate_code_t mpeg_num_framerates = MPEG_NUM_RATES;

static const char *
framerate_definitions[MPEG_NUM_RATES] =
{
   "illegal", 
  "24000.0/1001.0 (NTSC 3:2 pulldown converted FILM)",
  "24.0 (NATIVE FILM)",
  "25.0 (PAL/SECAM VIDEO / converted FILM)",
  "30000.0/1001.0 (NTSC VIDEO)",
  "30.0",
  "50.0 (PAL FIELD RATE)",
  "60000.0/1001.0 (NTSC FIELD RATE)",
  "60.0"
};


static const char *mpeg1_aspect_ratio_definitions[] =
{
    "illegal",
	"1:1 (square pixels)",
	"1:0.6735",
	"1:0.7031 (16:9 Anamorphic PAL/SECAM for 720x578/352x288 images)",
	"1:0.7615",
	"1:0.8055",
	"1:0.8437 (16:9 Anamorphic NTSC for 720x480/352x240 images)",
	"1:0.8935",
	"1:0.9375 (4:3 PAL/SECAM for 720x578/352x288 images)",
	"1:0.9815",
	"1:1.0255",
	"1:1:0695",
	"1:1.1250 (4:3 NTSC for 720x480/352x240 images)",
	"1:1.1575",
	"1:1.2015"
};

static const y4m_ratio_t mpeg1_aspect_ratios[] =
{
    Y4M_SAR_UNKNOWN,
	Y4M_SAR_MPEG1_1,
	Y4M_SAR_MPEG1_2,
	Y4M_SAR_MPEG1_3, /* Anamorphic 16:9 PAL */
	Y4M_SAR_MPEG1_4,
	Y4M_SAR_MPEG1_5,
	Y4M_SAR_MPEG1_6, /* Anamorphic 16:9 NTSC */
	Y4M_SAR_MPEG1_7,
	Y4M_SAR_MPEG1_8, /* PAL/SECAM 4:3 */
	Y4M_SAR_MPEG1_9,
	Y4M_SAR_MPEG1_10,
	Y4M_SAR_MPEG1_11,
	Y4M_SAR_MPEG1_12, /* NTSC 4:3 */
	Y4M_SAR_MPEG1_13,
	Y4M_SAR_MPEG1_14,
};

static const char *mpeg2_aspect_ratio_definitions[] = 
{
    "illegal",
	"1:1 pixels",
	"4:3 display",
	"16:9 display",
	"2.21:1 display"
};


static const y4m_ratio_t mpeg2_aspect_ratios[] =
{
    Y4M_DAR_UNKNOWN,
	Y4M_DAR_MPEG2_1,
	Y4M_DAR_MPEG2_2,
 	Y4M_DAR_MPEG2_3,
	Y4M_DAR_MPEG2_4
};

static const char **aspect_ratio_definitions[2] = 
{
	mpeg1_aspect_ratio_definitions,
	mpeg2_aspect_ratio_definitions
};

static const y4m_ratio_t *mpeg_aspect_ratios[2] = 
{
	mpeg1_aspect_ratios,
	mpeg2_aspect_ratios
};

static const mpeg_aspect_code_t mpeg_num_aspect_ratios[2] = 
{
  sizeof(mpeg1_aspect_ratios)/sizeof(mpeg1_aspect_ratios[0]),
  sizeof(mpeg2_aspect_ratios)/sizeof(mpeg2_aspect_ratios[0])
};

static const char *mjpegtools_format_code_definitions[MPEG_FORMAT_LAST+1] =
{
    "Generic MPEG1",
    "Standard VCD",
    "Stretched VCD",
    "Generic MPEG2",
    "Standard SVCD",
    "Stretched SVCD",
    "VCD Still",
    "SVCD Still",
    "DVD with dummy navigation packets",
    "Standard DVD",
    "ATSC 480i",
    "ATSC 480p",
    "ATSC 720p",
    "ATSC 1080i"
};

/*
 * Is code a valid MPEG framerate code?
 */

int
mpeg_valid_framerate_code( mpeg_framerate_code_t code )
{
    return ((code > 0) && (code < mpeg_num_framerates)) ? 1 : 0;
}


/*
 * Convert MPEG frame-rate code to corresponding frame-rate
 */

y4m_ratio_t
mpeg_framerate( mpeg_framerate_code_t code )
{
    if ((code > 0) && (code < mpeg_num_framerates))
		return mpeg_framerates[code];
    else
		return y4m_fps_UNKNOWN;
}

/*
 * Look-up MPEG frame rate code for a (exact) frame rate.
 */


mpeg_framerate_code_t 
mpeg_framerate_code( y4m_ratio_t framerate )
{
	mpeg_framerate_code_t i;
  
	y4m_ratio_reduce(&framerate);
    /* start at '1', because 0 is unknown/illegal */
	for (i = 1; i < mpeg_num_framerates; ++i) {
		if (Y4M_RATIO_EQL(framerate, mpeg_framerates[i]))
			return i;
	}
	return 0;
}


/* small enough to distinguish 1/1000 from 1/1001 */
#define MPEG_FPS_TOLERANCE 0.0001


y4m_ratio_t
mpeg_conform_framerate( double fps )
{
	mpeg_framerate_code_t i;
	y4m_ratio_t result;

	/* try to match it to a standard frame rate */
    /* (start at '1', because 0 is unknown/illegal) */
	for (i = 1; i < mpeg_num_framerates; i++) 
	{
		double deviation = 1.0 - (Y4M_RATIO_DBL(mpeg_framerates[i]) / fps);
        if ((deviation > -MPEG_FPS_TOLERANCE) && (deviation < +MPEG_FPS_TOLERANCE))
			return mpeg_framerates[i];
	}
	/* no luck?  just turn it into a ratio (6 decimal place accuracy) */
	result.n = (int)((fps * 1000000.0) + 0.5);
	result.d = 1000000;
	y4m_ratio_reduce(&result);
	return result;
}

  

/*
 * Is code a valid MPEG aspect-ratio code?
 */

int
mpeg_valid_aspect_code( int version, mpeg_framerate_code_t c )
{
	if ((version == 1) || (version == 2))
        return ((c > 0) && (c < mpeg_num_aspect_ratios[version-1])) ? 1 : 0;
    return 0;
}


/*
 * Convert MPEG aspect-ratio code to corresponding aspect-ratio
 */

y4m_ratio_t 
mpeg_aspect_ratio( int mpeg_version,  mpeg_aspect_code_t code )
{
	y4m_ratio_t ratio;
    if ((mpeg_version >= 1) && (mpeg_version <= 2) &&
        (code > 0) && (code < mpeg_num_aspect_ratios[mpeg_version-1]))
	{
		ratio = mpeg_aspect_ratios[mpeg_version-1][code];
		y4m_ratio_reduce(&ratio);
		return ratio;
	}
    else
		return y4m_sar_UNKNOWN;
}



/*
 * Look-up corresponding MPEG aspect ratio code given an exact aspect ratio.
 *
 * WARNING: The semantics of aspect ratio coding *changed* between
 * MPEG1 and MPEG2.  In MPEG1 it is the *pixel* aspect ratio. In
 * MPEG2 it is the (far more sensible) aspect ratio of the eventual
 * display.
 *
 */

mpeg_aspect_code_t 
mpeg_frame_aspect_code( int mpeg_version, y4m_ratio_t aspect_ratio )
{
	mpeg_aspect_code_t i;
	y4m_ratio_t red_ratio = aspect_ratio;
	y4m_ratio_reduce( &red_ratio );
	if( mpeg_version < 1 || mpeg_version > 2 )
		return 0;
    /* (start at '1', because 0 is unknown/illegal) */
	for( i = 1; i < mpeg_num_aspect_ratios[mpeg_version-1]; ++i )
	{
		y4m_ratio_t red_entry =  mpeg_aspect_ratios[mpeg_version-1][i];
		y4m_ratio_reduce( &red_entry );
		if(  Y4M_RATIO_EQL( red_entry, red_ratio) )
			return i;
	}

	return 0;
			
}



/*
 * Guess the correct MPEG aspect ratio code,
 *  given the true sample aspect ratio and frame size of a video stream
 *  (and the MPEG version, 1 or 2).
 *
 * Returns 0 if it has no good guess.
 *
 */


/* this is big enough to accommodate the difference between 720 and 704 */
#define GUESS_ASPECT_TOLERANCE 0.03

mpeg_aspect_code_t 
mpeg_guess_mpeg_aspect_code(int mpeg_version, y4m_ratio_t sampleaspect,
							int frame_width, int frame_height)
{
	if (Y4M_RATIO_EQL(sampleaspect, y4m_sar_UNKNOWN))
		return 0;
    
	switch (mpeg_version) {
	case 1:
		if (Y4M_RATIO_EQL(sampleaspect, y4m_sar_SQUARE))
			return 1;
		
        if (Y4M_RATIO_EQL(sampleaspect, y4m_sar_NTSC_CCIR601))
			return 12;
		
        if (Y4M_RATIO_EQL(sampleaspect, y4m_sar_NTSC_16_9))
			return 6;
		
        if (Y4M_RATIO_EQL(sampleaspect, y4m_sar_PAL_CCIR601))
			return 8;
		
        if (Y4M_RATIO_EQL(sampleaspect, y4m_sar_PAL_16_9))
			return 3;
		
		return 0;
	case 2:
		if (Y4M_RATIO_EQL(sampleaspect, y4m_sar_SQUARE))
		{
			return 1;  /* '1' means square *pixels* in MPEG-2; go figure. */
		}
		
		int i;
		double true_far;  /* true frame aspect ratio */
		true_far = 
			(double)(sampleaspect.n * frame_width) /
			(double)(sampleaspect.d * frame_height);
		/* start at '2'... */
		for (i = 2; i < (int)(mpeg_num_aspect_ratios[mpeg_version-1]); i++) 
		{
			double ratio = 
                true_far / Y4M_RATIO_DBL(mpeg_aspect_ratios[mpeg_version-1][i]);

			if ( (ratio > (1.0 - GUESS_ASPECT_TOLERANCE)) &&
				 (ratio < (1.0 + GUESS_ASPECT_TOLERANCE)) )
				return i;
		}
		return 0;
	default:
		return 0;
	}
}




/*
 * Guess the true sample aspect ratio of a video stream,
 *  given the MPEG aspect ratio code and the actual frame size
 *  (and the MPEG version, 1 or 2).
 *
 * Returns y4m_sar_UNKNOWN if it has no good guess.
 *
 */
y4m_ratio_t 
mpeg_guess_sample_aspect_ratio(int mpeg_version,
							   mpeg_aspect_code_t code,
							   int frame_width, int frame_height)
{
	switch (mpeg_version) 
	{
	case 1:
		/* MPEG-1 codes turn into SAR's, just not quite the right ones.
		   For the common/known values, we provide the ratio used in practice,
		   otherwise say we don't know.*/
		switch (code)
		{
		case 1:  return y4m_sar_SQUARE;        break;
		case 3:  return y4m_sar_PAL_16_9;      break;
		case 6:  return y4m_sar_NTSC_16_9;     break;
		case 8:  return y4m_sar_PAL_CCIR601;   break;
		case 12: return y4m_sar_NTSC_CCIR601;  break;
		default:
			return y4m_sar_UNKNOWN;       break;
		}
		break;
	case 2:
		/* MPEG-2 codes turn into Display Aspect Ratios, though not exactly the
		   DAR's used in practice.  For common/standard frame sizes, we provide
		   the original SAR; otherwise, we say we don't know. */
		if (code == 1) 
			return y4m_sar_SQUARE; /* '1' means square *pixels* in MPEG-2 */
		
		if ((code >= 2) && (code <= 4))
            return y4m_guess_sar(frame_width, frame_height, mpeg2_aspect_ratios[code]);
		
        return y4m_sar_UNKNOWN;
    default:
        return y4m_sar_UNKNOWN;
	}
}





/*
 * Look-up MPEG explanatory definition string for frame rate code
 *
 */


const char *
mpeg_framerate_code_definition(   mpeg_framerate_code_t code  )
{
	if( code == 0 || code >=  mpeg_num_framerates )
		return "UNDEFINED: illegal/reserved frame-rate ratio code";

	return framerate_definitions[code];
}

/*
 * Look-up MPEG explanatory definition string aspect ratio code for an
 * aspect ratio code
 *
 */

const char *
mpeg_aspect_code_definition( int mpeg_version,  mpeg_aspect_code_t code  )
{
	if( mpeg_version < 1 || mpeg_version > 2 )
		return "UNDEFINED: illegal MPEG version";
	
	if( code < 1 || code >=  mpeg_num_aspect_ratios[mpeg_version-1] )
		return "UNDEFINED: illegal aspect ratio code";

	return aspect_ratio_definitions[mpeg_version-1][code];
}


/*
 * Look-up explanatory definition of interlace field order code
 *
 */

const char *
mpeg_interlace_code_definition( int yuv4m_interlace_code )
{
	const char *def;
	switch( yuv4m_interlace_code )
	{
	case Y4M_UNKNOWN :
		def = "unknown";
		break;
	case Y4M_ILACE_NONE :
		def = "none/progressive";
		break;
	case Y4M_ILACE_TOP_FIRST :
		def = "top-field-first";
		break;
	case Y4M_ILACE_BOTTOM_FIRST :
		def = "bottom-field-first";
		break;
	default :
		def = "UNDEFINED: illegal video interlacing type-code!";
		break;
	}
	return def;
}

/*
 * Look-up explanatory definition of mjepgtools preset format code
 *
 */
const char *mpeg_format_code_defintion( int format_code )
{
    if(format_code >= MPEG_FORMAT_FIRST && format_code <= MPEG_FORMAT_LAST )
        return mjpegtools_format_code_definitions[format_code];
    else
        return "UNDEFINED: illegal format code!";
};


const char *y4m_strerr(int err)
{
  switch (err) {
  case Y4M_OK:          return "no error";
  case Y4M_ERR_RANGE:   return "parameter out of range";
  case Y4M_ERR_SYSTEM:  return "system error (failed read/write)";
  case Y4M_ERR_HEADER:  return "bad stream or frame header";
  case Y4M_ERR_BADTAG:  return "unknown header tag";
  case Y4M_ERR_MAGIC:   return "bad header magic";
  case Y4M_ERR_XXTAGS:  return "too many xtags";
  case Y4M_ERR_EOF:     return "end-of-file";
  case Y4M_ERR_BADEOF:  return "stream ended unexpectedly (EOF)";
  case Y4M_ERR_FEATURE: return "stream requires unsupported features";
  default: 
    return "unknown error code";
  }
}


y4m_ratio_t y4m_chroma_ss_x_ratio(int chroma_mode)
{ 
    y4m_ratio_t r;
    switch (chroma_mode)
    {
    case Y4M_CHROMA_444ALPHA:
    case Y4M_CHROMA_444:
    case Y4M_CHROMA_MONO:
        r.n = 1;
        r.d = 1;
        break;
    case Y4M_CHROMA_420JPEG:
    case Y4M_CHROMA_420MPEG2:
    case Y4M_CHROMA_420PALDV:
    case Y4M_CHROMA_422:
        r.n = 1;
        r.d = 2;
        break;
    case Y4M_CHROMA_411:
        r.n = 1;
        r.d = 4;
        break;
    default:
        r.n = 0;
        r.d = 0;
    }
    return r;
}

y4m_ratio_t y4m_chroma_ss_y_ratio(int chroma_mode)
{ 
  y4m_ratio_t r;
  switch (chroma_mode) {
  case Y4M_CHROMA_444ALPHA:
  case Y4M_CHROMA_444:
  case Y4M_CHROMA_MONO:
  case Y4M_CHROMA_422:
  case Y4M_CHROMA_411:
    r.n = 1; r.d = 1; break;
  case Y4M_CHROMA_420JPEG:
  case Y4M_CHROMA_420MPEG2:
  case Y4M_CHROMA_420PALDV:
    r.n = 1; r.d = 2; break;
  default:
    r.n = 0; r.d = 0;
  }
  return r;
}

int y4m_chroma_parse_keyword(const char *s)
{ 
    if (!strcasecmp("420jpeg", s))
        return Y4M_CHROMA_420JPEG;
    if (!strcasecmp("420mpeg2", s))
        return Y4M_CHROMA_420MPEG2;
    if (!strcasecmp("420paldv", s))
        return Y4M_CHROMA_420PALDV;
    if (!strcasecmp("444", s))
        return Y4M_CHROMA_444;
    if (!strcasecmp("422", s))
        return Y4M_CHROMA_422;
    if (!strcasecmp("411", s))
        return Y4M_CHROMA_411;
    if (!strcasecmp("mono", s))
        return Y4M_CHROMA_MONO;
    if (!strcasecmp("444alpha", s))
        return Y4M_CHROMA_444ALPHA;
    return Y4M_UNKNOWN;
}

const char *y4m_chroma_keyword(int chroma_mode)
{
  switch (chroma_mode) {
  case Y4M_CHROMA_420JPEG:  return "420jpeg";
  case Y4M_CHROMA_420MPEG2: return "420mpeg2";
  case Y4M_CHROMA_420PALDV: return "420paldv";
  case Y4M_CHROMA_444:      return "444";
  case Y4M_CHROMA_422:      return "422";
  case Y4M_CHROMA_411:      return "411";
  case Y4M_CHROMA_MONO:     return "mono";
  case Y4M_CHROMA_444ALPHA: return "444alpha";
  default:
    return NULL;
  }
}  

const char *y4m_chroma_description(int chroma_mode)
{           
  switch (chroma_mode) {
  case Y4M_CHROMA_420JPEG:  return "4:2:0 JPEG/MPEG-1 (interstitial)";
  case Y4M_CHROMA_420MPEG2: return "4:2:0 MPEG-2 (horiz. cositing)";
  case Y4M_CHROMA_420PALDV: return "4:2:0 PAL-DV (altern. siting)";
  case Y4M_CHROMA_444:      return "4:4:4 (no subsampling)";
  case Y4M_CHROMA_422:      return "4:2:2 (horiz. cositing)";
  case Y4M_CHROMA_411:      return "4:1:1 (horiz. cositing)";
  case Y4M_CHROMA_MONO:     return "luma plane only";
  case Y4M_CHROMA_444ALPHA: return "4:4:4 with alpha channel";
  default:
    return NULL;
  }
}

#if 1
static ssize_t y4m_read_fd(void * data, void *buf, size_t len)
  {
  int * f = (int*)data;
  return y4m_read(*f, buf, len);
  }

/* write len bytes from fd into buf */
static ssize_t y4m_write_fd(void * data, const void *buf, size_t len)
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

static void set_cb_writer_from_fd(y4m_cb_writer_t * ret, int * fd)
  {
  ret->write = y4m_write_fd;
  ret->data = fd;
  }

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

// returns 0 if equal, nonzero if different
#if 0
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

static int y4m_reread_stream_header_line_cb(y4m_cb_reader_t *fd,const y4m_stream_info_t *si,char *line,int n)
{
    y4m_stream_info_t i;
    int err=y4m_read_stream_header_line_cb(fd,&i,line,n);
    if( err==Y4M_OK && y4m_compare_stream_info(si,&i) )
        err=Y4M_ERR_HEADER;
    y4m_fini_stream_info(&i);
    return err;
}
#endif

int y4m_read_stream_header_cb(y4m_cb_reader_t *fd, y4m_stream_info_t *i)
{
    char line[Y4M_LINE_MAX];

    return y4m_read_stream_header_line_cb(fd,i,line,0);
}

int y4m_read_stream_header(int fd, y4m_stream_info_t *i)
{
  y4m_cb_reader_t r;
  set_cb_reader_from_fd(&r, &fd);
  return y4m_read_stream_header_cb(&r, i);
}

int y4m_write_stream_header(int fd, const y4m_stream_info_t *i)
{
  y4m_cb_writer_t w;
  set_cb_writer_from_fd(&w, &fd);
  return y4m_write_stream_header_cb(&w, i);
}

#if 0
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

#if 0
int y4m_read_frame_header(int fd,
              const y4m_stream_info_t *si,
              y4m_frame_info_t *fi)
  {
  y4m_cb_reader_t r;
  set_cb_reader_from_fd(&r, &fd);
  return y4m_read_frame_header_cb(&r, si, fi);
  }
#endif

int y4m_write_frame_header(int fd,
               const y4m_stream_info_t *si,
               const y4m_frame_info_t *fi)
{
  y4m_cb_writer_t w;
  set_cb_writer_from_fd(&w, &fd);
  return y4m_write_frame_header_cb(&w, si, fi);
}

int y4m_read_frame_data_cb(y4m_cb_reader_t * fd, const y4m_stream_info_t *si, 
                        y4m_frame_info_t *fi, uint8_t * const *frame)
{
  int planes = y4m_si_get_plane_count(si);
  int p;

  /* Read each plane */
  for (p = 0; p < planes; p++) {
    int w = y4m_si_get_plane_width(si, p);
    int h = y4m_si_get_plane_height(si, p);
    if (y4m_read_cb(fd, frame[p], w*h)) return Y4M_ERR_SYSTEM;
  }
  return Y4M_OK;
}

int y4m_read_frame_data(int fd, const y4m_stream_info_t *si,
                        y4m_frame_info_t *fi, uint8_t * const *frame)
{
  y4m_cb_reader_t r;
  set_cb_reader_from_fd(&r, &fd);
  return y4m_read_frame_data_cb(&r, si, fi, frame);
}

int y4m_read_frame_cb(y4m_cb_reader_t * fd, const y4m_stream_info_t *si,
           y4m_frame_info_t *fi, uint8_t * const *frame)
{
  int err;

  /* Read frame header */
  if ((err = y4m_read_frame_header_cb(fd, si, fi)) != Y4M_OK) return err;
  /* Read date */
  return y4m_read_frame_data_cb(fd, si, fi, frame);
}

int y4m_read_frame(int fd, const y4m_stream_info_t *si,
           y4m_frame_info_t *fi, uint8_t * const *frame)
{
  y4m_cb_reader_t r;
  set_cb_reader_from_fd(&r, &fd);
  return y4m_read_frame_cb(&r, si, fi, frame);
}

int y4m_write_frame_cb(y4m_cb_writer_t * fd, const y4m_stream_info_t *si,
            const y4m_frame_info_t *fi, uint8_t * const *frame)
{
  int planes = y4m_si_get_plane_count(si);
  int err, p;

  /* Write frame header */
  if ((err = y4m_write_frame_header_cb(fd, si, fi)) != Y4M_OK) return err;
  /* Write each plane */
  for (p = 0; p < planes; p++) {
    int w = y4m_si_get_plane_width(si, p);
    int h = y4m_si_get_plane_height(si, p);
    if (y4m_write_cb(fd, frame[p], w*h)) return Y4M_ERR_SYSTEM;
  }
  return Y4M_OK;
}

int y4m_write_frame(int fd, const y4m_stream_info_t *si,
            const y4m_frame_info_t *fi, uint8_t * const *frame)
{
  y4m_cb_writer_t w;
  set_cb_writer_from_fd(&w, &fd);
  return y4m_write_frame_cb(&w, si, fi, frame);
}

int y4m_read_fields_data(int fd, const y4m_stream_info_t *si,
                         y4m_frame_info_t *fi,
                         uint8_t * const *upper_field, 
                         uint8_t * const *lower_field)
{
  y4m_cb_reader_t r;
  set_cb_reader_from_fd(&r, &fd);
  return y4m_read_fields_data_cb(&r, si, fi, upper_field, lower_field);
}


int y4m_read_fields_cb(y4m_cb_reader_t * fd, const y4m_stream_info_t *si, y4m_frame_info_t *fi,
                       uint8_t * const *upper_field, 
                       uint8_t * const *lower_field)
{
  int err;
  /* Read frame header */
  if ((err = y4m_read_frame_header_cb(fd, si, fi)) != Y4M_OK) return err;
  /* Read data */
  return y4m_read_fields_data_cb(fd, si, fi, upper_field, lower_field);
}

int y4m_read_fields(int fd, const y4m_stream_info_t *si, y4m_frame_info_t *fi,
                    uint8_t * const *upper_field, 
                    uint8_t * const *lower_field)
{
  y4m_cb_reader_t r;
  set_cb_reader_from_fd(&r, &fd);
  return y4m_read_fields_cb(&r, si, fi, upper_field, lower_field);
}




int y4m_write_fields(int fd, const y4m_stream_info_t *si,
                     const y4m_frame_info_t *fi,
                     uint8_t * const *upper_field, 
                     uint8_t * const *lower_field)
{ 
  y4m_cb_writer_t w; 
  set_cb_writer_from_fd(&w, &fd);
  return y4m_write_fields_cb(&w, si, fi, upper_field, lower_field);
}

#if 0
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

void y4m_log_stream_info(log_level_t level, const char *prefix,
             const y4m_stream_info_t *i)
{ 
  char s[256];
  
  snprintf(s, sizeof(s), "  frame size:  ");
  if (i->width == Y4M_UNKNOWN) 
    snprintf(s+strlen(s), sizeof(s)-strlen(s), "(?)x");
  else 
    snprintf(s+strlen(s), sizeof(s)-strlen(s), "%dx", i->width);
  if (i->height == Y4M_UNKNOWN)
    snprintf(s+strlen(s), sizeof(s)-strlen(s), "(?) pixels ");
  else 
    snprintf(s+strlen(s), sizeof(s)-strlen(s), "%d pixels ", i->height);
  { 
    int framelength = y4m_si_get_framelength(i);
    if (framelength == Y4M_UNKNOWN)
      snprintf(s+strlen(s), sizeof(s)-strlen(s), "(? bytes)");
    else
      snprintf(s+strlen(s), sizeof(s)-strlen(s), "(%d bytes)", framelength);
    mjpeg_log(level, "%s%s", prefix, s);
  }     
  { 
    const char *desc = y4m_chroma_description(i->chroma);
    if (desc == NULL) desc = "unknown!";
    mjpeg_log(level, "%s      chroma:  %s", prefix, desc);
  }
  if ((i->framerate.n == 0) && (i->framerate.d == 0))
    mjpeg_log(level, "%s  frame rate:  ??? fps", prefix);
  else
    mjpeg_log(level, "%s  frame rate:  %d/%d fps (~%f)", prefix,
          i->framerate.n, i->framerate.d,
          (double) i->framerate.n / (double) i->framerate.d);
  mjpeg_log(level, "%s   interlace:  %s", prefix,
      (i->interlace == Y4M_ILACE_NONE) ? "none/progressive" :
      (i->interlace == Y4M_ILACE_TOP_FIRST) ? "top-field-first" :
      (i->interlace == Y4M_ILACE_BOTTOM_FIRST) ? "bottom-field-first" :
      (i->interlace == Y4M_ILACE_MIXED) ? "mixed-mode" :
      "anyone's guess"); 
  if ((i->sampleaspect.n == 0) && (i->sampleaspect.d == 0))
    mjpeg_log(level, "%ssample aspect ratio:  ?:?", prefix);
  else
    mjpeg_log(level, "%ssample aspect ratio:  %d:%d", prefix,
          i->sampleaspect.n, i->sampleaspect.d);
}

int y4m_si_get_plane_count(const y4m_stream_info_t *si)
{   
  switch (si->chroma) {
  case Y4M_CHROMA_420JPEG:
  case Y4M_CHROMA_420MPEG2:
  case Y4M_CHROMA_420PALDV: 
  case Y4M_CHROMA_444:
  case Y4M_CHROMA_422:
  case Y4M_CHROMA_411:
    return 3;
  case Y4M_CHROMA_MONO:
    return 1;
  case Y4M_CHROMA_444ALPHA:
    return 4;
  default:
    return Y4M_UNKNOWN; 
  } 
}     
    
int y4m_si_get_plane_width(const y4m_stream_info_t *si, int plane)
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
    switch (si->chroma) {
    case Y4M_CHROMA_444ALPHA:
      return (si->width);
    default:
      return Y4M_UNKNOWN;
    }
  default:
    return Y4M_UNKNOWN;
  }
}

int y4m_si_get_plane_height(const y4m_stream_info_t *si, int plane)
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



