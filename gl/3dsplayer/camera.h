#ifndef INCLUDED_LIB3DS_CAMERA_H
#define INCLUDED_LIB3DS_CAMERA_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

class Lib3dsCamera {
public:
    Lib3dsCamera *next;
    char name[64];
    Lib3dsVector position;
    Lib3dsVector target;
    Lib3dsFloat roll;
    Lib3dsFloat fov;
    Lib3dsBool see_cone;
    Lib3dsFloat near_range;
    Lib3dsFloat far_range;
}; 

extern Lib3dsCamera* lib3ds_camera_new(const char *name);
extern void lib3ds_camera_free(Lib3dsCamera *mesh);
extern void lib3ds_camera_dump(Lib3dsCamera *camera);
extern Lib3dsBool lib3ds_camera_read(Lib3dsCamera *camera, Lib3dsIo *io);
extern Lib3dsBool lib3ds_camera_write(Lib3dsCamera *camera, Lib3dsIo *io);

#ifdef __cplusplus
};
#endif
#endif

