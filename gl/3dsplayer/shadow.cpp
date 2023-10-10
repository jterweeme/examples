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
 * $Id: shadow.c,v 1.8 2001/07/07 19:05:30 jeh Exp $
 */
#include "shadow.h"
#include "chunk.h"
#include "io.h"
#include <math.h>

Lib3dsBool lib3ds_shadow_read(Lib3dsShadow *shadow, Lib3dsIo *io)
{
  Lib3dsChunk c;

  if (!lib3ds_chunk_read(&c, io))
    return(LIB3DS_FALSE);
  
  switch (c.chunk) {
    case LIB3DS_SHADOW_MAP_SIZE:
        shadow->map_size=lib3ds_io_read_intw(io);
        break;
    case LIB3DS_LO_SHADOW_BIAS:
        shadow->lo_bias=lib3ds_io_read_float(io);
        break;
    case LIB3DS_HI_SHADOW_BIAS:
        shadow->hi_bias=lib3ds_io_read_float(io);
        break;
    case LIB3DS_SHADOW_SAMPLES:
      shadow->samples=lib3ds_io_read_intw(io);
      break;
    case LIB3DS_SHADOW_RANGE:
      shadow->range=lib3ds_io_read_intd(io);
      break;
    case LIB3DS_SHADOW_FILTER:
      shadow->filter=lib3ds_io_read_float(io);
      break;
    case LIB3DS_RAY_BIAS:
      shadow->ray_bias=lib3ds_io_read_float(io);
      break;
  }
  
  return LIB3DS_TRUE;
}



