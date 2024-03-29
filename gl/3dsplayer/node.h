#ifndef INCLUDED_LIB3DS_NODE_H
#define INCLUDED_LIB3DS_NODE_H

#include "tracks.h"

class Lib3dsFile;

#ifdef __cplusplus
extern "C" {
#endif

struct Lib3dsAmbientData {
    Lib3dsRgb col;
    Lib3dsLin3Track col_track;
};

/*!
 * Scene graph object instance node data
 * \ingroup node
 */
struct Lib3dsObjectData {
    Lib3dsVector pivot;
    char instance[64];
    Lib3dsVector bbox_min;
    Lib3dsVector bbox_max;
    Lib3dsVector pos;
    Lib3dsLin3Track pos_track;
    Lib3dsQuat rot;
    Lib3dsQuatTrack rot_track;
    Lib3dsVector scl;
    Lib3dsLin3Track scl_track;
    Lib3dsFloat morph_smooth;
    char morph[64];
    Lib3dsMorphTrack morph_track;
    Lib3dsBool hide;
    Lib3dsBoolTrack hide_track;
};

/*!
 * Scene graph camera node data
 * \ingroup node
 */
struct Lib3dsCameraData {
    Lib3dsVector pos;
    Lib3dsLin3Track pos_track;
    Lib3dsFloat fov;
    Lib3dsLin1Track fov_track;
    Lib3dsFloat roll;
    Lib3dsLin1Track roll_track;
};

/*!
 * Scene graph camera target node data
 * \ingroup node
 */
struct Lib3dsTargetData {
    Lib3dsVector pos;
    Lib3dsLin3Track pos_track;
};

/*!
 * Scene graph light node data
 * \ingroup node
 */
struct Lib3dsLightData {
    Lib3dsVector pos;
    Lib3dsLin3Track pos_track;
    Lib3dsRgb col;
    Lib3dsLin3Track col_track;
    Lib3dsFloat hotspot;
    Lib3dsLin1Track hotspot_track;
    Lib3dsFloat falloff;
    Lib3dsLin1Track falloff_track;
    Lib3dsFloat roll;
    Lib3dsLin1Track roll_track;
};

/*!
 * Scene graph spotlight target node data
 * \ingroup node
 */
struct Lib3dsSpotData {
    Lib3dsVector pos;
    Lib3dsLin3Track pos_track;
};

/*!
 * Scene graph node data union
 * \ingroup node
 */
union Lib3dsNodeData {
    Lib3dsAmbientData ambient;
    Lib3dsObjectData object;
    Lib3dsCameraData camera;
    Lib3dsTargetData target;
    Lib3dsLightData light;
    Lib3dsSpotData spot;
};

/*!
 * \ingroup node
 */
#define LIB3DS_NO_PARENT 65535

/*!
 * Scene graph node
 * \ingroup node
 */
struct Lib3dsNode {
    Lib3dsUserData user;
    Lib3dsNode *next;\
    Lib3dsNode *childs;\
    Lib3dsNode *parent;\
    Lib3dsNodeTypes type;\
    Lib3dsWord node_id;\
    char name[64];\
    Lib3dsWord flags1;\
    Lib3dsWord flags2;\
    Lib3dsWord parent_id;
    Lib3dsMatrix matrix;
    Lib3dsNodeData data;
};

extern Lib3dsNode* lib3ds_node_new_ambient();
extern Lib3dsNode* lib3ds_node_new_object();
extern Lib3dsNode* lib3ds_node_new_camera();
extern Lib3dsNode* lib3ds_node_new_target();
extern Lib3dsNode* lib3ds_node_new_light();
extern Lib3dsNode* lib3ds_node_new_spot();
extern void lib3ds_node_free(Lib3dsNode *node);
extern void lib3ds_node_eval(Lib3dsNode *node, Lib3dsFloat t);
extern Lib3dsNode* lib3ds_node_by_name(Lib3dsNode *node, const char* name,
  Lib3dsNodeTypes type);
extern Lib3dsNode* lib3ds_node_by_id(Lib3dsNode *node, Lib3dsWord node_id);
extern void lib3ds_node_dump(Lib3dsNode *node, Lib3dsIntd level);
extern Lib3dsBool lib3ds_node_read(Lib3dsNode *node, Lib3dsFile *file, Lib3dsIo *io);
extern Lib3dsBool lib3ds_node_write(Lib3dsNode *node, Lib3dsFile *file, Lib3dsIo *io);

#ifdef __cplusplus
};
#endif
#endif

