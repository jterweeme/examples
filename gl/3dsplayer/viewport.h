/* -*- c -*- */
#ifndef INCLUDED_LIB3DS_VIEWPORT_H
#define INCLUDED_LIB3DS_VIEWPORT_H
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
 * $Id: viewport.h,v 1.4 2001/07/07 19:05:30 jeh Exp $
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum Lib3dsViewType {
  LIB3DS_VIEW_TYPE_NOT_USED  =0,
  LIB3DS_VIEW_TYPE_TOP       =1,
  LIB3DS_VIEW_TYPE_BOTTOM    =2,
  LIB3DS_VIEW_TYPE_LEFT      =3,
  LIB3DS_VIEW_TYPE_RIGHT     =4,
  LIB3DS_VIEW_TYPE_FRONT     =5,
  LIB3DS_VIEW_TYPE_BACK      =6,
  LIB3DS_VIEW_TYPE_USER      =7,
  LIB3DS_VIEW_TYPE_SPOTLIGHT =18,
  LIB3DS_VIEW_TYPE_CAMERA    =65535
};

/*!
 * Layout view settings
 * \ingroup viewport
 */
typedef struct _Lib3dsView {
    Lib3dsWord type;
    Lib3dsWord axis_lock;
    Lib3dsIntw position[2];
    Lib3dsIntw size[2];
    Lib3dsFloat zoom;
    Lib3dsVector center;
    Lib3dsFloat horiz_angle;
    Lib3dsFloat vert_angle;
    char camera[11];
} Lib3dsView;

/*!
 * Layout styles
 * \ingroup viewport
 */
typedef enum _Lib3dsLayoutStyle {
  LIB3DS_LAYOUT_SINGLE                  =0,
  LIB3DS_LAYOUT_TWO_PANE_VERT_SPLIT     =1,
  LIB3DS_LAYOUT_TWO_PANE_HORIZ_SPLIT    =2,
  LIB3DS_LAYOUT_FOUR_PANE               =3,
  LIB3DS_LAYOUT_THREE_PANE_LEFT_SPLIT   =4,
  LIB3DS_LAYOUT_THREE_PANE_BOTTOM_SPLIT =5,
  LIB3DS_LAYOUT_THREE_PANE_RIGHT_SPLIT  =6,
  LIB3DS_LAYOUT_THREE_PANE_TOP_SPLIT    =7,
  LIB3DS_LAYOUT_THREE_PANE_VERT_SPLIT   =8,
  LIB3DS_LAYOUT_THREE_PANE_HORIZ_SPLIT  =9,
  LIB3DS_LAYOUT_FOUR_PANE_LEFT_SPLIT    =10,
  LIB3DS_LAYOUT_FOUR_PANE_RIGHT_SPLIT   =11
} Lib3dsLayoutStyle;

/*!
 * Viewport layout settings
 * \ingroup viewport
 */
typedef struct _Lib3dsLayout {
    Lib3dsWord style;
    Lib3dsIntw active;
    Lib3dsIntw swap;
    Lib3dsIntw swap_prior;
    Lib3dsIntw swap_view;
    Lib3dsWord position[2];
    Lib3dsWord size[2];
    Lib3dsDword views;
    Lib3dsView *viewL;
} Lib3dsLayout;

/*!
 * Default view settings
 * \ingroup viewport
 */
typedef struct _Lib3dsDefaultView {
    Lib3dsWord type;
    Lib3dsVector position;
    Lib3dsFloat width;
    Lib3dsFloat horiz_angle;
    Lib3dsFloat vert_angle;
    Lib3dsFloat roll_angle;
    char camera[64];
} Lib3dsDefaultView;

/*!
 * Viewport and default view settings
 * \ingroup viewport
 */
struct _Lib3dsViewport {
    Lib3dsLayout layout;
    Lib3dsDefaultView default_view;
};

extern Lib3dsBool lib3ds_viewport_read(Lib3dsViewport *viewport, Lib3dsIo *io);
extern void lib3ds_viewport_set_views(Lib3dsViewport *viewport, Lib3dsDword views);
extern Lib3dsBool lib3ds_viewport_write(Lib3dsViewport *viewport, Lib3dsIo *io);

#ifdef __cplusplus
};
#endif
#endif




