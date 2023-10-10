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
 * $Id: mesh.c,v 1.20 2001/11/14 22:44:52 jeh Exp $
 */
#define LIB3DS_EXPORT
#include "mesh.h"
#include "io.h"
#include "chunk.h"
#include "vector.h"
#include "matrix.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "config.h"
#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


/*!
 * \defgroup mesh Meshes
 *
 * \author J.E. Hoffmann <je-h@gmx.net>
 */


static Lib3dsBool
face_array_read(Lib3dsMesh *mesh, Lib3dsIo *io)
{
  Lib3dsChunk c;
  Lib3dsWord chunk;
  int i;
  int faces;

  if (!lib3ds_chunk_read_start(&c, LIB3DS_FACE_ARRAY, io)) {
    return(LIB3DS_FALSE);
  }
  lib3ds_mesh_free_face_list(mesh);
  
  faces=lib3ds_io_read_word(io);
  if (faces) {
    if (!lib3ds_mesh_new_face_list(mesh, faces)) {
      LIB3DS_ERROR_LOG;
      return(LIB3DS_FALSE);
    }
    for (i=0; i<faces; ++i) {
      strcpy(mesh->faceL[i].material, "");
      mesh->faceL[i].points[0]=lib3ds_io_read_word(io);
      mesh->faceL[i].points[1]=lib3ds_io_read_word(io);
      mesh->faceL[i].points[2]=lib3ds_io_read_word(io);
      mesh->faceL[i].flags=lib3ds_io_read_word(io);
    }
    lib3ds_chunk_read_tell(&c, io);

    while ((chunk=lib3ds_chunk_read_next(&c, io))!=0) {
      switch (chunk) {
        case LIB3DS_SMOOTH_GROUP:
          {
            unsigned i;

            for (i=0; i<mesh->faces; ++i) {
              mesh->faceL[i].smoothing=lib3ds_io_read_dword(io);
            }
            mesh->normaltype = LIB3DS_SMOOTH_GROUP;
          }
          break;
        case LIB3DS_FACE_NORMAL_ARRAY:
          {
            unsigned i, j;

            for (i=0; i<mesh->faces; ++i) {
              for (j=0; j<3; ++j) {
                mesh->faceL[i].normal[j]=lib3ds_io_read_float(io);
              }
            }
            mesh->normaltype = LIB3DS_FACE_NORMAL_ARRAY;
          }
          break;
        case LIB3DS_MSH_MAT_GROUP:
          {
            char name[64];
            unsigned faces;
            unsigned i;
            unsigned index;

            if (!lib3ds_io_read_string(io, name, 64)) {
              return(LIB3DS_FALSE);
            }
            faces=lib3ds_io_read_word(io);
            for (i=0; i<faces; ++i) {
              index=lib3ds_io_read_word(io);
              ASSERT(index<mesh->faces);
              strcpy(mesh->faceL[index].material, name);
            }
          }
          break;
        case LIB3DS_MSH_BOXMAP:
          {
            char name[64];

            if (!lib3ds_io_read_string(io, name, 64)) {
              return(LIB3DS_FALSE);
            }
            strcpy(mesh->box_map.front, name);
            if (!lib3ds_io_read_string(io, name, 64)) {
              return(LIB3DS_FALSE);
            }
            strcpy(mesh->box_map.back, name);
            if (!lib3ds_io_read_string(io, name, 64)) {
              return(LIB3DS_FALSE);
            }
            strcpy(mesh->box_map.left, name);
            if (!lib3ds_io_read_string(io, name, 64)) {
              return(LIB3DS_FALSE);
            }
            strcpy(mesh->box_map.right, name);
            if (!lib3ds_io_read_string(io, name, 64)) {
              return(LIB3DS_FALSE);
            }
            strcpy(mesh->box_map.top, name);
            if (!lib3ds_io_read_string(io, name, 64)) {
              return(LIB3DS_FALSE);
            }
            strcpy(mesh->box_map.bottom, name);
          }
          break;
        default:
          lib3ds_chunk_unknown(chunk);
      }
    }
    
  }
  lib3ds_chunk_read_end(&c, io);
  return(LIB3DS_TRUE);
}


/*!
 * \ingroup mesh
 */
Lib3dsMesh*
lib3ds_mesh_new(const char *name)
{
  Lib3dsMesh *mesh;

  ASSERT(name);
  ASSERT(strlen(name)<64);
  
  mesh=(Lib3dsMesh*)calloc(sizeof(Lib3dsMesh), 1);
  if (!mesh) {
    return(0);
  }
  strcpy(mesh->name, name);
  lib3ds_matrix_identity(mesh->matrix);
  mesh->map_data.maptype=LIB3DS_MAP_NONE;
  return(mesh);
}


/*!
 * \ingroup mesh
 */
void
lib3ds_mesh_free(Lib3dsMesh *mesh)
{
  lib3ds_mesh_free_point_list(mesh);
  lib3ds_mesh_free_flag_list(mesh);
  lib3ds_mesh_free_texel_list(mesh);
  lib3ds_mesh_free_normal_list(mesh);
  lib3ds_mesh_free_face_list(mesh);
  memset(mesh, 0, sizeof(Lib3dsMesh));
  free(mesh);
}


/*!
 * \ingroup mesh
 */
Lib3dsBool
lib3ds_mesh_new_point_list(Lib3dsMesh *mesh, Lib3dsDword points)
{
  ASSERT(mesh);
  if (mesh->pointL) {
    ASSERT(mesh->points);
    lib3ds_mesh_free_point_list(mesh);
  }
  ASSERT(!mesh->pointL && !mesh->points);
  mesh->points=0;
  mesh->pointL = (Lib3dsPoint *)calloc(sizeof(Lib3dsPoint)*points,1);
  if (!mesh->pointL) {
    LIB3DS_ERROR_LOG;
    return(LIB3DS_FALSE);
  }
  mesh->points=points;
  return(LIB3DS_TRUE);
}


/*!
 * \ingroup mesh
 */
void
lib3ds_mesh_free_point_list(Lib3dsMesh *mesh)
{
  ASSERT(mesh);
  if (mesh->pointL) {
    ASSERT(mesh->points);
    free(mesh->pointL);
    mesh->pointL=0;
    mesh->points=0;
  }
  else {
    ASSERT(!mesh->points);
  }
}


/*!
 * \ingroup mesh
 */
Lib3dsBool
lib3ds_mesh_new_flag_list(Lib3dsMesh *mesh, Lib3dsDword flags)
{
  ASSERT(mesh);
  if (mesh->flagL) {
    ASSERT(mesh->flags);
    lib3ds_mesh_free_flag_list(mesh);
  }
  ASSERT(!mesh->flagL && !mesh->flags);
  mesh->flags=0;
  mesh->flagL = (Lib3dsWord *)calloc(sizeof(Lib3dsWord)*flags,1);
  if (!mesh->flagL) {
    LIB3DS_ERROR_LOG;
    return(LIB3DS_FALSE);
  }
  mesh->flags=flags;
  return(LIB3DS_TRUE);
}


/*!
 * \ingroup mesh
 */
void
lib3ds_mesh_free_flag_list(Lib3dsMesh *mesh)
{
  ASSERT(mesh);
  if (mesh->flagL) {
    ASSERT(mesh->flags);
    free(mesh->flagL);
    mesh->flagL=0;
    mesh->flags=0;
  }
  else {
    ASSERT(!mesh->flags);
  }
}


/*!
 * \ingroup mesh
 */
Lib3dsBool
lib3ds_mesh_new_texel_list(Lib3dsMesh *mesh, Lib3dsDword texels)
{
  ASSERT(mesh);
  if (mesh->texelL) {
    ASSERT(mesh->texels);
    lib3ds_mesh_free_texel_list(mesh);
  }
  ASSERT(!mesh->texelL && !mesh->texels);
  mesh->texels=0;
  mesh->texelL=(Lib3dsTexel *)calloc(sizeof(Lib3dsTexel)*texels,1);
  if (!mesh->texelL) {
    LIB3DS_ERROR_LOG;
    return(LIB3DS_FALSE);
  }
  mesh->texels=texels;
  return(LIB3DS_TRUE);
}


/*!
 * \ingroup mesh
 */
void
lib3ds_mesh_free_texel_list(Lib3dsMesh *mesh)
{
  ASSERT(mesh);
  if (mesh->texelL) {
    ASSERT(mesh->texels);
    free(mesh->texelL);
    mesh->texelL=0;
    mesh->texels=0;
  }
  else {
    ASSERT(!mesh->texels);
  }
}


/*!
 * \ingroup mesh
 */
Lib3dsBool
lib3ds_mesh_new_normal_list(Lib3dsMesh *mesh, Lib3dsDword normals)
{
  ASSERT(mesh);
  if (mesh->normalL) {
    ASSERT(mesh->normals);
    lib3ds_mesh_free_normal_list(mesh);
  }
  ASSERT(!mesh->normalL && !mesh->normals);
  mesh->normals=0;
  mesh->normalL=(Lib3dsPoint *)calloc(sizeof(Lib3dsPoint)*normals,1);
  if (!mesh->normalL) {
    LIB3DS_ERROR_LOG;
    return(LIB3DS_FALSE);
  }
  mesh->normals=normals;
  return(LIB3DS_TRUE);
}


/*!
 * \ingroup mesh
 */
void
lib3ds_mesh_free_normal_list(Lib3dsMesh *mesh)
{
  ASSERT(mesh);
  if (mesh->normalL) {
    ASSERT(mesh->normals);
    free(mesh->normalL);
    mesh->normalL=0;
    mesh->normals=0;
  }
  else {
    ASSERT(!mesh->normals);
  }
}


/*!
 * \ingroup mesh
 */
Lib3dsBool
lib3ds_mesh_new_face_list(Lib3dsMesh *mesh, Lib3dsDword faces)
{
  ASSERT(mesh);
  if (mesh->faceL) {
    ASSERT(mesh->faces);
    lib3ds_mesh_free_face_list(mesh);
  }
  ASSERT(!mesh->faceL && !mesh->faces);
  mesh->faces=0;
  mesh->faceL = (Lib3dsFace *)calloc(sizeof(Lib3dsFace)*faces,1);
  if (!mesh->faceL) {
    LIB3DS_ERROR_LOG;
    return(LIB3DS_FALSE);
  }
  mesh->faces=faces;
  return(LIB3DS_TRUE);
}


/*!
 * \ingroup mesh
 */
void
lib3ds_mesh_free_face_list(Lib3dsMesh *mesh)
{
  ASSERT(mesh);
  if (mesh->faceL) {
    ASSERT(mesh->faces);
    free(mesh->faceL);
    mesh->faceL=0;
    mesh->faces=0;
  }
  else {
    ASSERT(!mesh->faces);
  }
}


typedef struct _Lib3dsFaces Lib3dsFaces; 

struct _Lib3dsFaces {
  Lib3dsFaces *next;
  Lib3dsFace *face;
};




/*!
 * \ingroup mesh
 */
void
lib3ds_mesh_bounding_box(Lib3dsMesh *mesh, Lib3dsVector min, Lib3dsVector max)
{
  unsigned i,j;
  Lib3dsFloat v;

  if (!mesh->points) {
    lib3ds_vector_zero(min);
    lib3ds_vector_zero(max);
    return;
  }
 
  lib3ds_vector_copy(min, mesh->pointL[0].pos);
  lib3ds_vector_copy(max, mesh->pointL[0].pos);
  for (i=1; i<mesh->points; ++i) {
    for (j=0; j<3; ++j) {
      v=mesh->pointL[i].pos[j];
      if (v<min[j]) {
        min[j]=v;
      }
      if (v>max[j]) {
        max[j]=v;
      }
    }
  };
}


/*!
 * Calculates the vertex normals corresponding to the smoothing group
 * settings for each face of a mesh.
 *
 * \param mesh      A pointer to the mesh to calculate the normals for.
 * \param normalL   A pointer to a buffer to store the calculated
 *                  normals. The buffer must have the size:
 *                  3*sizeof(Lib3dsVector)*mesh->faces. 
 *
 * To allocate the normal buffer do for example the following:
 * \code
 *  Lib3dsVector *normalL = malloc(3*sizeof(Lib3dsVector)*mesh->faces);
 * \endcode
 *
 * To access the normal of the i-th vertex of the j-th face do the 
 * following:
 * \code
 *   normalL[3*j+i]
 * \endcode
 */
void
lib3ds_mesh_calculate_normals(Lib3dsMesh *mesh, Lib3dsVector *normalL)
{
  Lib3dsFaces **fl; 
  Lib3dsFaces *fa; 
  unsigned i,j,k;

  if (!mesh->faces) {
    return;
  }

  fl=(Lib3dsFaces **)calloc(sizeof(Lib3dsFaces*),mesh->points);
  ASSERT(fl);
  fa=(Lib3dsFaces *)calloc(sizeof(Lib3dsFaces),3*mesh->faces);
  ASSERT(fa);
  k=0;
  for (i=0; i<mesh->faces; ++i) {
    Lib3dsFace *f=&mesh->faceL[i];
    for (j=0; j<3; ++j) {
      Lib3dsFaces* l=&fa[k++];
      ASSERT(f->points[j]<mesh->points);
      l->face=f;
      l->next=fl[f->points[j]];
      fl[f->points[j]]=l;
    }
  }
  
  for (i=0; i<mesh->faces; ++i) {
    Lib3dsFace *f=&mesh->faceL[i];
    for (j=0; j<3; ++j) {
      // FIXME: static array needs at least check!!
      Lib3dsVector n,N[64];
      Lib3dsFaces *p;
      int k,l;
      int found;

      ASSERT(f->points[j]<mesh->points);

      if (f->smoothing) {
        lib3ds_vector_zero(n);
        k=0;
        for (p=fl[f->points[j]]; p; p=p->next) {
          found=0;
          for (l=0; l<k; ++l) {
            if (fabs(lib3ds_vector_dot(N[l], p->face->normal)-1.0)<1e-5) {
              found=1;
              break;
            }
          }
          if (!found) {
            if (f->smoothing & p->face->smoothing) {
              lib3ds_vector_add(n,n, p->face->normal);
              lib3ds_vector_copy(N[k], p->face->normal);
              ++k;
            }
          }
        }
      } 
      else {
        lib3ds_vector_copy(n, f->normal);
      }
      lib3ds_vector_normalize(n);

      lib3ds_vector_copy(normalL[3*i+j], n);
    }
  }

  free(fa);
  free(fl);
}


/*!
 * This function prints data associated with the specified mesh such as
 * vertex and point lists.
 *
 * \param mesh  Points to a mesh that you wish to view the data for.
 *
 * \return None
 *
 * \warning WIN32: Should only be used in a console window not in a GUI.
 *
 * \ingroup mesh
 */
void
lib3ds_mesh_dump(Lib3dsMesh *mesh)
{
  unsigned i;
  Lib3dsVector p;

  ASSERT(mesh);
  printf("  %s vertices=%ld faces=%ld\n",
    mesh->name,
    mesh->points,
    mesh->faces
  );
  printf("  matrix:\n");
  lib3ds_matrix_dump(mesh->matrix);
  printf("  point list:\n");
  for (i=0; i<mesh->points; ++i) {
    lib3ds_vector_copy(p, mesh->pointL[i].pos);
    printf ("    %8f %8f %8f\n", p[0], p[1], p[2]);
  }
  printf("  facelist:\n");
  for (i=0; i<mesh->faces; ++i) {
    printf ("    %4d %4d %4d  smoothing:%X\n",
      mesh->faceL[i].points[0],
      mesh->faceL[i].points[1],
      mesh->faceL[i].points[2],
      (unsigned)mesh->faceL[i].smoothing
    );
  }
}


/*!
 * \ingroup mesh
 */
Lib3dsBool
lib3ds_mesh_read(Lib3dsMesh *mesh, Lib3dsIo *io)
{
  Lib3dsChunk c;
  Lib3dsWord chunk;

  if (!lib3ds_chunk_read_start(&c, LIB3DS_N_TRI_OBJECT, io)) {
    return(LIB3DS_FALSE);
  }

  while ((chunk=lib3ds_chunk_read_next(&c, io))!=0) {
    switch (chunk) {
      case LIB3DS_MESH_MATRIX:
        {
          int i,j;
          
          lib3ds_matrix_identity(mesh->matrix);
          for (i=0; i<4; i++) {
            for (j=0; j<3; j++) {
              mesh->matrix[i][j]=lib3ds_io_read_float(io);
            }
          }
        }
        break;
      case LIB3DS_MESH_COLOR:
        {
          mesh->color=lib3ds_io_read_byte(io);
        }
        break;
      case LIB3DS_POINT_ARRAY:
        {
          unsigned i,j;
          unsigned points;
          
          lib3ds_mesh_free_point_list(mesh);
          points=lib3ds_io_read_word(io);
          if (points) {
            if (!lib3ds_mesh_new_point_list(mesh, points)) {
              LIB3DS_ERROR_LOG;
              return(LIB3DS_FALSE);
            }
            for (i=0; i<mesh->points; ++i) {
              for (j=0; j<3; ++j) {
                mesh->pointL[i].pos[j]=lib3ds_io_read_float(io);
              }
            }
            ASSERT((!mesh->flags) || (mesh->points==mesh->flags));
            ASSERT((!mesh->texels) || (mesh->points==mesh->texels));
            ASSERT((!mesh->normals) || (mesh->points==mesh->normals));
          }
        }
        break;
      case LIB3DS_POINT_FLAG_ARRAY:
        {
          unsigned i;
          unsigned flags;
          
          lib3ds_mesh_free_flag_list(mesh);
          flags=lib3ds_io_read_word(io);
          if (flags) {
            if (!lib3ds_mesh_new_flag_list(mesh, flags)) {
              LIB3DS_ERROR_LOG;
              return(LIB3DS_FALSE);
            }
            for (i=0; i<mesh->flags; ++i) {
              mesh->flagL[i]=lib3ds_io_read_word(io);
            }
            ASSERT((!mesh->points) || (mesh->flags==mesh->points));
            ASSERT((!mesh->texels) || (mesh->flags==mesh->texels));
            ASSERT((!mesh->normals) || (mesh->flags==mesh->normals));
          }
        }
        break;
      case LIB3DS_FACE_ARRAY:
        {
          lib3ds_chunk_read_reset(&c, io);
          if (!face_array_read(mesh, io)) {
            return(LIB3DS_FALSE);
          }
        }
        break;
      case LIB3DS_MESH_TEXTURE_INFO:
        {
          int i,j;

          for (i=0; i<2; ++i) {
            mesh->map_data.tile[i]=lib3ds_io_read_float(io);
          }
          for (i=0; i<3; ++i) {
            mesh->map_data.pos[i]=lib3ds_io_read_float(io);
          }
          mesh->map_data.scale=lib3ds_io_read_float(io);

          lib3ds_matrix_identity(mesh->map_data.matrix);
          for (i=0; i<4; i++) {
            for (j=0; j<3; j++) {
              mesh->map_data.matrix[i][j]=lib3ds_io_read_float(io);
            }
          }
          for (i=0; i<2; ++i) {
            mesh->map_data.planar_size[i]=lib3ds_io_read_float(io);
          }
          mesh->map_data.cylinder_height=lib3ds_io_read_float(io);
        }
        break;
      case LIB3DS_TEX_VERTS:
        {
          unsigned i;
          unsigned texels;
          
          lib3ds_mesh_free_texel_list(mesh);
          texels=lib3ds_io_read_word(io);
          if (texels) {
            if (!lib3ds_mesh_new_texel_list(mesh, texels)) {
              LIB3DS_ERROR_LOG;
              return(LIB3DS_FALSE);
            }
            for (i=0; i<mesh->texels; ++i) {
              mesh->texelL[i][0]=lib3ds_io_read_float(io);
              mesh->texelL[i][1]=lib3ds_io_read_float(io);
            }
            ASSERT((!mesh->points) || (mesh->texels==mesh->points));
            ASSERT((!mesh->flags) || (mesh->texels==mesh->flags));
            ASSERT((!mesh->normals) || (mesh->texels==mesh->normals));
          }
        }
        break;
      case LIB3DS_NORMAL_ARRAY:
        {
          unsigned i,j;
          unsigned normals;
          
          lib3ds_mesh_free_normal_list(mesh);
          normals=lib3ds_io_read_word(io);
          if (normals) {
            if (!lib3ds_mesh_new_normal_list(mesh, normals)) {
              LIB3DS_ERROR_LOG;
              return(LIB3DS_FALSE);
            }
            for (i=0; i<mesh->normals; ++i) {
              for (j=0; j<3; ++j) {
                mesh->normalL[i].pos[j]=lib3ds_io_read_float(io);
              }
            }
            ASSERT((!mesh->points) || (mesh->normals==mesh->points));
            ASSERT((!mesh->flags) || (mesh->normals==mesh->flags));
            ASSERT((!mesh->texels) || (mesh->normals==mesh->texels));
          }
        }
        break;
      default:
        lib3ds_chunk_unknown(chunk);
    }
  }
  if (mesh->normaltype == LIB3DS_FACE_NORMAL_ARRAY) {
    unsigned  j;
    for (j=0; j<mesh->faces; ++j) {
      ASSERT(mesh->faceL[j].points[0]<mesh->points);
      ASSERT(mesh->faceL[j].points[1]<mesh->points);
      ASSERT(mesh->faceL[j].points[2]<mesh->points);
      mesh->faceL[j].smoothing = 0;
    }
  } else
  if (mesh->normals) {
    unsigned j;
    for (j=0; j<mesh->faces; ++j) {
      ASSERT(mesh->faceL[j].points[0]<mesh->points);
      ASSERT(mesh->faceL[j].points[1]<mesh->points);
      ASSERT(mesh->faceL[j].points[2]<mesh->points);
      mesh->faceL[j].smoothing = 0;
      lib3ds_vector_normal_average(
        mesh->faceL[j].normal,
        mesh->normalL[mesh->faceL[j].points[0]].pos,
        mesh->normalL[mesh->faceL[j].points[1]].pos,
        mesh->normalL[mesh->faceL[j].points[2]].pos
      );
    }
   
  } else {
    unsigned j;
    for (j=0; j<mesh->faces; ++j) {
      ASSERT(mesh->faceL[j].points[0]<mesh->points);
      ASSERT(mesh->faceL[j].points[1]<mesh->points);
      ASSERT(mesh->faceL[j].points[2]<mesh->points);
      lib3ds_vector_normal(
        mesh->faceL[j].normal,
        mesh->pointL[mesh->faceL[j].points[0]].pos,
        mesh->pointL[mesh->faceL[j].points[1]].pos,
        mesh->pointL[mesh->faceL[j].points[2]].pos
      );
    }
    /*if (mesh->normaltype == LIB3DS_SMOOTH_GROUP) {
      lib3ds_mesh_calculate_normals(mesh, list);
    }*/
  }
  
  lib3ds_chunk_read_end(&c, io);
  return(LIB3DS_TRUE);
}


