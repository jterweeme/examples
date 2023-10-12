/* -*- c -*- */
#ifndef INCLUDED_LIB3DS_MESH_H
#define INCLUDED_LIB3DS_MESH_H
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
 * $Id: mesh.h,v 1.13 2001/07/07 19:05:30 jeh Exp $
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Triangular mesh point
 * \ingroup mesh
 */
typedef struct _Lib3dsPoint {
    Lib3dsVector pos;
} Lib3dsPoint;

/*!
 * Triangular mesh face
 * \ingroup mesh
 */
struct _Lib3dsFace {
    Lib3dsUserData user;
    char material[64];
    Lib3dsWord points[3];
    Lib3dsWord flags;
    Lib3dsDword smoothing;
    Lib3dsVector normal;
};

/*!
 * Triangular mesh box mapping settings
 * \ingroup mesh
 */
struct _Lib3dsBoxMap {
    char front[64];
    char back[64];
    char left[64];
    char right[64];
    char top[64];
    char bottom[64];
};

/*!
 * Lib3dsMapData maptype
 * \ingroup tracks
 */
typedef enum {
  LIB3DS_MAP_NONE        =0xFFFF,
  LIB3DS_MAP_PLANAR      =0,
  LIB3DS_MAP_CYLINDRICAL =1,
  LIB3DS_MAP_SPHERICAL   =2
} Lib3dsMapType;

/*!
 * Triangular mesh texture mapping data
 * \ingroup mesh
 */
struct _Lib3dsMapData {
    uint16_t maptype;
    Lib3dsVector pos;
    Lib3dsMatrix matrix;
    float scale;
    float tile[2];
    float planar_size[2];
    float cylinder_height;
};

/*!
 * Triangular mesh object
 * \ingroup mesh
 */
struct Lib3dsMesh {
    Lib3dsUserData user;
    Lib3dsMesh *next;
    char name[64];
    uint8_t color;
    Lib3dsMatrix matrix;
    uint32_t points;
    Lib3dsPoint *pointL;
    uint32_t flags;
    uint16_t *flagL;
    uint32_t texels;
    Lib3dsTexel *texelL;
    uint32_t faces;
    Lib3dsFace *faceL;
    Lib3dsBoxMap box_map;
    Lib3dsMapData map_data;
    /* additional variables */
    uint16_t normaltype;
    uint32_t normals;
    Lib3dsPoint *normalL;
}; 

extern Lib3dsMesh* lib3ds_mesh_new(const char *name);
extern void lib3ds_mesh_free(Lib3dsMesh *mesh);
extern Lib3dsBool lib3ds_mesh_new_point_list(Lib3dsMesh *mesh, Lib3dsDword points);
extern void lib3ds_mesh_free_point_list(Lib3dsMesh *mesh);
extern Lib3dsBool lib3ds_mesh_new_flag_list(Lib3dsMesh *mesh, Lib3dsDword flags);
extern void lib3ds_mesh_free_flag_list(Lib3dsMesh *mesh);
extern Lib3dsBool lib3ds_mesh_new_texel_list(Lib3dsMesh *mesh, Lib3dsDword texels);
extern void lib3ds_mesh_free_texel_list(Lib3dsMesh *mesh);
extern Lib3dsBool lib3ds_mesh_new_face_list(Lib3dsMesh *mesh, Lib3dsDword flags);
extern void lib3ds_mesh_free_face_list(Lib3dsMesh *mesh);
extern Lib3dsBool lib3ds_mesh_new_normal_list(Lib3dsMesh *mesh, Lib3dsDword normals);
extern void lib3ds_mesh_free_normal_list(Lib3dsMesh *mesh);
extern void lib3ds_mesh_bounding_box(Lib3dsMesh *mesh, Lib3dsVector min, Lib3dsVector max);
extern void lib3ds_mesh_calculate_normals(Lib3dsMesh *mesh, Lib3dsVector *normalL);
extern void lib3ds_mesh_dump(Lib3dsMesh *mesh);
extern Lib3dsBool lib3ds_mesh_read(Lib3dsMesh *mesh, Lib3dsIo *io);

};
#endif

