#ifndef INCLUDED_LIB3DS_VECTOR_H
#define INCLUDED_LIB3DS_VECTOR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void lib3ds_vector_zero(Lib3dsVector c);
extern void lib3ds_vector_copy(Lib3dsVector dest, Lib3dsVector src);
extern void lib3ds_vector_neg(Lib3dsVector c);
extern void lib3ds_vector_add(Lib3dsVector c, Lib3dsVector a, Lib3dsVector b);
extern void lib3ds_vector_sub(Lib3dsVector c, Lib3dsVector a, Lib3dsVector b);
extern void lib3ds_vector_scalar(Lib3dsVector c, Lib3dsFloat k);
extern void lib3ds_vector_cross(Lib3dsVector c, Lib3dsVector a, Lib3dsVector b);
extern Lib3dsFloat lib3ds_vector_dot(Lib3dsVector a, Lib3dsVector b);
extern Lib3dsFloat lib3ds_vector_squared(Lib3dsVector c);
extern Lib3dsFloat lib3ds_vector_length(Lib3dsVector c);
extern void lib3ds_vector_normalize(Lib3dsVector c);
extern void lib3ds_vector_normal(Lib3dsVector n, Lib3dsVector a,
  Lib3dsVector b, Lib3dsVector c);
extern void lib3ds_vector_normal_average(Lib3dsVector n, Lib3dsVector a,
  Lib3dsVector b, Lib3dsVector c);
extern void lib3ds_vector_transform(Lib3dsVector c, Lib3dsMatrix m, Lib3dsVector a);
extern void lib3ds_vector_cubic(Lib3dsVector c, Lib3dsVector a, Lib3dsVector p,
  Lib3dsVector q, Lib3dsVector b, Lib3dsFloat t);
extern void lib3ds_vector_min(Lib3dsVector c, Lib3dsVector a);
extern void lib3ds_vector_max(Lib3dsVector c, Lib3dsVector a);
extern void lib3ds_vector_dump(Lib3dsVector c);

#ifdef __cplusplus
};
#endif
#endif

