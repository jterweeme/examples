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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "yuv4mpeg.h"
#include "mjpeg_logging.h"
#include "mpegconsts.h"
#include "mpegtimecode.h"
#include <SDL.h>
#include <sys/time.h>
#include <assert.h>
#include "config.h"

#include <string.h>
#include "yuv4mpeg.h"
#include "yuv4mpeg_intern.h"

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
