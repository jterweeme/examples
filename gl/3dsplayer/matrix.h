/* -*- c -*- */
#ifndef INCLUDED_LIB3DS_MATRIX_H
#define INCLUDED_LIB3DS_MATRIX_H
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
 * $Id: matrix.h,v 1.4 2001/01/12 10:29:17 jeh Exp $
 */

#ifndef INCLUDED_LIB3DS_TYPES_H
#include "types.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern void lib3ds_matrix_zero(Lib3dsMatrix m);
extern void lib3ds_matrix_identity(Lib3dsMatrix m);
extern void lib3ds_matrix_copy(Lib3dsMatrix dest, Lib3dsMatrix src);
extern void lib3ds_matrix_neg(Lib3dsMatrix m);
extern void lib3ds_matrix_abs(Lib3dsMatrix m);
extern void lib3ds_matrix_transpose(Lib3dsMatrix m);
extern void lib3ds_matrix_add(Lib3dsMatrix m, Lib3dsMatrix a, Lib3dsMatrix b);
extern void lib3ds_matrix_sub(Lib3dsMatrix m, Lib3dsMatrix a, Lib3dsMatrix b);
extern void lib3ds_matrix_mul(Lib3dsMatrix m, Lib3dsMatrix a, Lib3dsMatrix b);
extern void lib3ds_matrix_scalar(Lib3dsMatrix m, Lib3dsFloat k);
extern Lib3dsFloat lib3ds_matrix_det(Lib3dsMatrix m);
extern void lib3ds_matrix_adjoint(Lib3dsMatrix m);
extern Lib3dsBool lib3ds_matrix_inv(Lib3dsMatrix m);
void lib3ds_matrix_translate_xyz(Lib3dsMatrix m, Lib3dsFloat x,
  Lib3dsFloat y, Lib3dsFloat z);
extern void lib3ds_matrix_translate(Lib3dsMatrix m, Lib3dsVector t);
extern void lib3ds_matrix_scale_xyz(Lib3dsMatrix m, Lib3dsFloat x,
  Lib3dsFloat y, Lib3dsFloat z);
extern void lib3ds_matrix_scale(Lib3dsMatrix m, Lib3dsVector s);
extern void lib3ds_matrix_rotate_x(Lib3dsMatrix m, Lib3dsFloat phi);
extern void lib3ds_matrix_rotate_y(Lib3dsMatrix m, Lib3dsFloat phi);
extern void lib3ds_matrix_rotate_z(Lib3dsMatrix m, Lib3dsFloat phi);
extern void lib3ds_matrix_rotate(Lib3dsMatrix m, Lib3dsQuat q);
extern void lib3ds_matrix_rotate_axis(Lib3dsMatrix m,
  Lib3dsVector axis, Lib3dsFloat angle);
extern void lib3ds_matrix_camera(Lib3dsMatrix matrix, Lib3dsVector pos,
  Lib3dsVector tgt, Lib3dsFloat roll);
extern void lib3ds_matrix_dump(Lib3dsMatrix matrix);

#ifdef __cplusplus
};
#endif
#endif

