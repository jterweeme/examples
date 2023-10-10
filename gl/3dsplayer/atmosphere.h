#ifndef INCLUDED_LIB3DS_ATMOSPHERE_H
#define INCLUDED_LIB3DS_ATMOSPHERE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Fog atmosphere settings
 * \ingroup atmosphere
 */
typedef struct _Lib3dsFog {
    Lib3dsBool use;
    Lib3dsRgb col;
    Lib3dsBool fog_background;
    Lib3dsFloat near_plane;
    Lib3dsFloat near_density;
    Lib3dsFloat far_plane;
    Lib3dsFloat far_density;
} Lib3dsFog;

/*!
 * Layer fog atmosphere flags
 * \ingroup atmosphere
 */
typedef enum _Lib3dsLayerFogFlags {
  LIB3DS_BOTTOM_FALL_OFF =0x00000001,
  LIB3DS_TOP_FALL_OFF    =0x00000002,
  LIB3DS_FOG_BACKGROUND  =0x00100000
} Lib3dsLayerFogFlags;

/*!
 * Layer fog atmosphere settings
 * \ingroup atmosphere
 */
typedef struct _Lib3dsLayerFog {
    Lib3dsBool use;
    Lib3dsDword flags;
    Lib3dsRgb col;
    Lib3dsFloat near_y;
    Lib3dsFloat far_y;
    Lib3dsFloat density;
} Lib3dsLayerFog;

/*!
 * Distance cue atmosphere settings
 * \ingroup atmosphere
 */
typedef struct _Lib3dsDistanceCue {
    Lib3dsBool use;
    Lib3dsBool cue_background;
    Lib3dsFloat near_plane;
    Lib3dsFloat near_dimming;
    Lib3dsFloat far_plane;
    Lib3dsFloat far_dimming;
} Lib3dsDistanceCue;

/*!
 * Atmosphere settings
 * \ingroup atmosphere
 */
struct _Lib3dsAtmosphere {
    Lib3dsFog fog;
    Lib3dsLayerFog layer_fog;
    Lib3dsDistanceCue dist_cue;
};

extern Lib3dsBool lib3ds_atmosphere_read(Lib3dsAtmosphere *atmosphere, Lib3dsIo *io);
extern Lib3dsBool lib3ds_atmosphere_write(Lib3dsAtmosphere *atmosphere, Lib3dsIo *io);

#ifdef __cplusplus
};
#endif
#endif
