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
 * $Id: atmosphere.c,v 1.9 2001/07/07 19:05:30 jeh Exp $
 */
#define LIB3DS_EXPORT
#include "atmosphere.h"
#include "chunk.h"
#include "io.h"


/*!
 * \defgroup atmosphere Atmosphere Settings
 *
 * \author J.E. Hoffmann <je-h@gmx.net>
 */


static Lib3dsBool
fog_read(Lib3dsFog *fog, Lib3dsIo *io)
{
  Lib3dsChunk c;
  Lib3dsWord chunk;

  if (!lib3ds_chunk_read_start(&c, LIB3DS_FOG, io)) {
    return(LIB3DS_FALSE);
  }
  fog->near_plane = lib3ds_io_read_float(io);
  fog->near_density=lib3ds_io_read_float(io);
  fog->far_plane=lib3ds_io_read_float(io);
  fog->far_density=lib3ds_io_read_float(io);
  lib3ds_chunk_read_tell(&c, io);
  
  while ((chunk=lib3ds_chunk_read_next(&c, io))!=0) {
    switch (chunk) {
      case LIB3DS_LIN_COLOR_F:
        {
          int i;
          for (i=0; i<3; ++i) {
            fog->col[i]=lib3ds_io_read_float(io);
          }
        }
        break;
      case LIB3DS_COLOR_F:
        break;
      case LIB3DS_FOG_BGND:
        {
          fog->fog_background=LIB3DS_TRUE;
        }
        break;
      default:
        lib3ds_chunk_unknown(chunk);
    }
  }
  
  lib3ds_chunk_read_end(&c, io);
  return(LIB3DS_TRUE);
}


static Lib3dsBool
layer_fog_read(Lib3dsLayerFog *fog, Lib3dsIo *io)
{
  Lib3dsChunk c;
  Lib3dsWord chunk;
  Lib3dsBool have_lin=LIB3DS_FALSE;

  if (!lib3ds_chunk_read_start(&c, LIB3DS_LAYER_FOG, io)) {
    return(LIB3DS_FALSE);
  }
  fog->near_y=lib3ds_io_read_float(io);
  fog->far_y=lib3ds_io_read_float(io);
  fog->density=lib3ds_io_read_float(io);
  fog->flags=lib3ds_io_read_dword(io);
  lib3ds_chunk_read_tell(&c, io);
  
  while ((chunk=lib3ds_chunk_read_next(&c, io))!=0) {
    switch (chunk) {
      case LIB3DS_LIN_COLOR_F:
        lib3ds_io_read_rgb(io, fog->col);
        have_lin=LIB3DS_TRUE;
        break;
      case LIB3DS_COLOR_F:
        lib3ds_io_read_rgb(io, fog->col);
        break;
      default:
        lib3ds_chunk_unknown(chunk);
    }
  }
  
  lib3ds_chunk_read_end(&c, io);
  return(LIB3DS_TRUE);
}


static Lib3dsBool
distance_cue_read(Lib3dsDistanceCue *cue, Lib3dsIo *io)
{
  Lib3dsChunk c;
  Lib3dsWord chunk;

  if (!lib3ds_chunk_read_start(&c, LIB3DS_DISTANCE_CUE, io)) {
    return(LIB3DS_FALSE);
  }
  cue->near_plane=lib3ds_io_read_float(io);
  cue->near_dimming=lib3ds_io_read_float(io);
  cue->far_plane=lib3ds_io_read_float(io);
  cue->far_dimming=lib3ds_io_read_float(io);
  lib3ds_chunk_read_tell(&c, io);
  
  while ((chunk=lib3ds_chunk_read_next(&c, io))!=0) {
    switch (chunk) {
      case LIB3DS_DCUE_BGND:
        {
          cue->cue_background=LIB3DS_TRUE;
        }
        break;
      default:
        lib3ds_chunk_unknown(chunk);
    }
  }
  
  lib3ds_chunk_read_end(&c, io);
  return(LIB3DS_TRUE);
}


/*!
 * \ingroup atmosphere
 */
Lib3dsBool
lib3ds_atmosphere_read(Lib3dsAtmosphere *atmosphere, Lib3dsIo *io)
{
  Lib3dsChunk c;

  if (!lib3ds_chunk_read(&c, io)) {
    return(LIB3DS_FALSE);
  }
  
  switch (c.chunk) {
      case LIB3DS_FOG:
        {
          lib3ds_chunk_read_reset(&c, io);
          if (!fog_read(&atmosphere->fog, io)) {
            return(LIB3DS_FALSE);
          }
        }
        break;
      case LIB3DS_LAYER_FOG:
        {
          lib3ds_chunk_read_reset(&c, io);
          if (!layer_fog_read(&atmosphere->layer_fog, io)) {
            return(LIB3DS_FALSE);
          }
        }
        break;
      case LIB3DS_DISTANCE_CUE:
        {
          lib3ds_chunk_read_reset(&c, io);
          if (!distance_cue_read(&atmosphere->dist_cue, io)) {
            return(LIB3DS_FALSE);
          }
        }
        break;
      case LIB3DS_USE_FOG:
        {
          atmosphere->fog.use=LIB3DS_TRUE;
        }
        break;
      case LIB3DS_USE_LAYER_FOG:
        {
          atmosphere->fog.use=LIB3DS_TRUE;
        }
        break;
      case LIB3DS_USE_DISTANCE_CUE:
        {
          atmosphere->dist_cue.use=LIB3DS_TRUE;
        }
        break;
  }

  return(LIB3DS_TRUE);
}




