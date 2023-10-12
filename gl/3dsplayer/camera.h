#ifndef INCLUDED_LIB3DS_CAMERA_H
#define INCLUDED_LIB3DS_CAMERA_H

#include "types.h"



class Lib3dsCamera {
public:
    Lib3dsCamera *next;
    char name[64];
    Lib3dsVector position;
    Lib3dsVector target;
    float roll;
    float fov;
    int see_cone;
    float near_range;
    float far_range;
    static Lib3dsCamera* lib3ds_camera_new(const char *name);
    static void lib3ds_camera_free(Lib3dsCamera *mesh);
    static void lib3ds_camera_dump(Lib3dsCamera *camera);
    static int lib3ds_camera_read(Lib3dsCamera *camera, Lib3dsIo *io);
}; 
#endif

