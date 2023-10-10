#ifndef INCLUDED_LIB3DS_TCB_H
#define INCLUDED_LIB3DS_TCB_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _Lib3dsTcbFlags{
  LIB3DS_USE_TENSION    =0x0001,
  LIB3DS_USE_CONTINUITY =0x0002,
  LIB3DS_USE_BIAS       =0x0004,
  LIB3DS_USE_EASE_TO    =0x0008,
  LIB3DS_USE_EASE_FROM  =0x0010
} Lib3dsTcbFlags;

struct Lib3dsTcb {
    Lib3dsIntd frame;
    Lib3dsWord flags;
    Lib3dsFloat tens;
    Lib3dsFloat cont;
    Lib3dsFloat bias;
    Lib3dsFloat ease_to;
    Lib3dsFloat ease_from;
};

extern void lib3ds_tcb(Lib3dsTcb *p, Lib3dsTcb *pc, Lib3dsTcb *c,
  Lib3dsTcb *nc, Lib3dsTcb *n, Lib3dsFloat *ksm, Lib3dsFloat *ksp,
  Lib3dsFloat *kdm, Lib3dsFloat *kdp);
extern Lib3dsBool lib3ds_tcb_read(Lib3dsTcb *tcb, Lib3dsIo *io);
extern Lib3dsBool lib3ds_tcb_write(Lib3dsTcb *tcb, Lib3dsIo *io);

#ifdef __cplusplus
};
#endif
#endif

