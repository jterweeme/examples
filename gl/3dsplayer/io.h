/* -*- c -*- */
#ifndef INCLUDED_LIB3DS_IO_H
#define INCLUDED_LIB3DS_IO_H
/*
 * The 3D Studio File Format Library
 * Copyright (C) 1996-2001 by J.E. Hoffmann <je-h@gmx.net>
 * All rights reserved.
 *
 * This program is  free  software;  you can redistribute it and/or modify it
 * under the terms of the  GNU Lesser General Public License  as published by 
 * the  Free Software Foundation;  either version 2.1 of the License,  or (at 
 * your option) any later version.
 *
 * This  program  is  distributed in  the  hope that it will  be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or  FITNESS FOR A  PARTICULAR PURPOSE.  See the  GNU Lesser General Public  
 * License for more details.
 *
 * You should  have received  a copy of the GNU Lesser General Public License
 * along with  this program;  if not, write to the  Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: io.h,v 1.2 2001/07/11 13:47:35 jeh Exp $
 */

#ifndef INCLUDED_LIB3DS_TYPES_H
#include "types.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _Lib3dsIoSeek {
  LIB3DS_SEEK_SET  =0,
  LIB3DS_SEEK_CUR  =1,
  LIB3DS_SEEK_END  =2
} Lib3dsIoSeek;
  
typedef Lib3dsBool (*Lib3dsIoErrorFunc)(void *self);
typedef long (*Lib3dsIoSeekFunc)(void *self, long offset, Lib3dsIoSeek origin);
typedef long (*Lib3dsIoTellFunc)(void *self);
typedef int (*Lib3dsIoReadFunc)(void *self, Lib3dsByte *buffer, int size);
typedef int (*Lib3dsIoWriteFunc)(void *self, const Lib3dsByte *buffer, int size);

extern Lib3dsIo* lib3ds_io_new(void *self, Lib3dsIoErrorFunc error_func,
  Lib3dsIoSeekFunc seek_func, Lib3dsIoTellFunc tell_func,
  Lib3dsIoReadFunc read_func, Lib3dsIoWriteFunc write_func);
extern void lib3ds_io_free(Lib3dsIo *io);
extern Lib3dsBool lib3ds_io_error(Lib3dsIo *io);
extern long lib3ds_io_seek(Lib3dsIo *io, long offset, Lib3dsIoSeek origin);
extern long lib3ds_io_tell(Lib3dsIo *io);
extern int lib3ds_io_read(Lib3dsIo *io, Lib3dsByte *buffer, int size);
extern int lib3ds_io_write(Lib3dsIo *io, const Lib3dsByte *buffer, int size);

extern Lib3dsByte lib3ds_io_read_byte(Lib3dsIo *io);
extern Lib3dsWord lib3ds_io_read_word(Lib3dsIo *io);
extern Lib3dsDword lib3ds_io_read_dword(Lib3dsIo *io);
extern Lib3dsIntb lib3ds_io_read_intb(Lib3dsIo *io);
extern Lib3dsIntw lib3ds_io_read_intw(Lib3dsIo *io);
extern Lib3dsIntd lib3ds_io_read_intd(Lib3dsIo *io);
extern Lib3dsFloat lib3ds_io_read_float(Lib3dsIo *io);
extern Lib3dsBool lib3ds_io_read_vector(Lib3dsIo *io, Lib3dsVector v);
extern Lib3dsBool lib3ds_io_read_rgb(Lib3dsIo *io, Lib3dsRgb rgb);
extern Lib3dsBool lib3ds_io_read_string(Lib3dsIo *io, char *s, int buflen);

extern Lib3dsBool lib3ds_io_write_byte(Lib3dsIo *io, Lib3dsByte b);
extern Lib3dsBool lib3ds_io_write_word(Lib3dsIo *io, Lib3dsWord w);
extern Lib3dsBool lib3ds_io_write_dword(Lib3dsIo *io, Lib3dsDword d);
extern Lib3dsBool lib3ds_io_write_intb(Lib3dsIo *io, Lib3dsIntb b);
extern Lib3dsBool lib3ds_io_write_intw(Lib3dsIo *io, Lib3dsIntw w);
extern Lib3dsBool lib3ds_io_write_intd(Lib3dsIo *io, Lib3dsIntd d);
extern Lib3dsBool lib3ds_io_write_float(Lib3dsIo *io, Lib3dsFloat l);
extern Lib3dsBool lib3ds_io_write_vector(Lib3dsIo *io, Lib3dsVector v);
extern Lib3dsBool lib3ds_io_write_rgb(Lib3dsIo *io, Lib3dsRgb rgb);
extern Lib3dsBool lib3ds_io_write_string(Lib3dsIo *io, const char *s);

#ifdef __cplusplus
};
#endif
#endif

