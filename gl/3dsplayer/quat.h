#ifndef INCLUDED_LIB3DS_QUAT_H
#define INCLUDED_LIB3DS_QUAT_H


#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void lib3ds_quat_zero(Lib3dsQuat c);
extern void lib3ds_quat_identity(Lib3dsQuat c);
extern void lib3ds_quat_copy(Lib3dsQuat dest, Lib3dsQuat src);
extern void lib3ds_quat_axis_angle(Lib3dsQuat c, Lib3dsVector axis, Lib3dsFloat angle);
extern void lib3ds_quat_neg(Lib3dsQuat c);
extern void lib3ds_quat_abs(Lib3dsQuat c);
extern void lib3ds_quat_cnj(Lib3dsQuat c);
extern void lib3ds_quat_mul(Lib3dsQuat c, Lib3dsQuat a, Lib3dsQuat b);
extern void lib3ds_quat_scalar(Lib3dsQuat c, Lib3dsFloat k);
extern void lib3ds_quat_normalize(Lib3dsQuat c);
extern void lib3ds_quat_inv(Lib3dsQuat c);
extern Lib3dsFloat lib3ds_quat_dot(Lib3dsQuat a, Lib3dsQuat b);
extern Lib3dsFloat lib3ds_quat_squared(Lib3dsQuat c);
extern Lib3dsFloat lib3ds_quat_length(Lib3dsQuat c);
extern void lib3ds_quat_ln(Lib3dsQuat c);
extern void lib3ds_quat_ln_dif(Lib3dsQuat c, Lib3dsQuat a, Lib3dsQuat b);
extern void lib3ds_quat_exp(Lib3dsQuat c);
extern void lib3ds_quat_slerp(Lib3dsQuat c, Lib3dsQuat a, Lib3dsQuat b, Lib3dsFloat t);
extern void lib3ds_quat_squad(Lib3dsQuat c, Lib3dsQuat a, Lib3dsQuat p, Lib3dsQuat q,
  Lib3dsQuat b, Lib3dsFloat t);
extern void lib3ds_quat_tangent(Lib3dsQuat c, Lib3dsQuat p, Lib3dsQuat q, Lib3dsQuat n);
extern void lib3ds_quat_dump(Lib3dsQuat q);

#ifdef __cplusplus
};
#endif
#endif

