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
 * $Id: player.c,v 1.2 2001/07/20 16:40:16 sunshine Exp $
 */
#include "file.h"                      
#include "camera.h"
#include "mesh.h"
#include "node.h"
#include "material.h"
#include "matrix.h"
#include "vector.h"
#include "light.h"
#include <string.h>
#include "config.h"
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>
#include <iostream>

/*!
\example player.c

Previews a <i>3DS</i> file using OpenGL.

\code
Syntax: player filename
\endcode

\warning To compile this program you must have OpenGL and glut installed.

\author J.E. Hoffmann <je-h@gmx.net>
*/

class App
{
public:
    const char* filename=0;
    const char* camera=0;
    Lib3dsFile *file = 0;
};

App app;


//static Lib3dsFile *file=0;
static float current_frame=0.0;
static int gl_width;
static int gl_height;
static int camera_menu_id=0;
static int halt=0;


/*!
 *
 */
static void
camera_menu(int value) 
{
  Lib3dsCamera *c;
  int i;

  for (c = app.file->cameras,i=0; i<value; ++i)
  {
    if (!c)
      return;
    
    c=c->next;
  }
  app.camera=c->name;
}


/*!
 *
 */
static void
init(void) 
{
  glClearColor(0.5, 0.5, 0.5, 1.0);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glDisable(GL_LIGHT1);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  //glDisable(GL_NORMALIZE);
  //glPolygonOffset(1.0, 2);


  app.file = Lib3dsFile::lib3ds_file_load(app.filename);

  if (!app.file)
  {
    std::cerr << "***ERROR*** Loading 3DS file failed.\r\n";
    exit(1);
  }

  if (!app.file->cameras)
  {  
    std::cerr << "***ERROR*** No Camera found.";
    Lib3dsFile::lib3ds_file_free(app.file);
    app.file=0;
    exit(1);
  }
  if (!app.camera) {
    app.camera = app.file->cameras->name;
  }
  camera_menu_id=glutCreateMenu(camera_menu);
  {
    Lib3dsCamera *c = app.file->cameras;
    int i = 0;

    while (c)
    {
      glutAddMenuEntry(c->name, i);
      c = c->next;
      ++i;
    }
  }
  glutAttachMenu(0);

  Lib3dsFile::lib3ds_file_eval(app.file,0);
}

static void
render_node(Lib3dsNode *node)
{
  ASSERT(file);

  for (Lib3dsNode *p = node->childs; p != nullptr; p=p->next)
      render_node(p);
  
  if (node->type==LIB3DS_OBJECT_NODE)
  {
    if (strcmp(node->name,"$$$DUMMY")==0)
      return;

    if (!node->user.d) {
      Lib3dsMesh *mesh=Lib3dsFile::lib3ds_file_mesh_by_name(app.file, node->name);
      ASSERT(mesh);
      if (!mesh) {
        return;
      }

      node->user.d=glGenLists(1);
      glNewList(node->user.d, GL_COMPILE);

      {
        Lib3dsVector *normalL = (Lib3dsVector *)malloc(3*sizeof(Lib3dsVector)*mesh->faces);

        {
          Lib3dsMatrix M;
          lib3ds_matrix_copy(M, mesh->matrix);
          lib3ds_matrix_inv(M);
          glMultMatrixf(&M[0][0]);
        }
        lib3ds_mesh_calculate_normals(mesh, normalL);

        for (unsigned p=0; p<mesh->faces; ++p)
        {
          Lib3dsFace *f=&mesh->faceL[p];
          Lib3dsMaterial *mat=0;

          if (f->material[0])
            mat = Lib3dsFile::lib3ds_file_material_by_name(app.file, f->material);

          if (mat)
          {
            static GLfloat a[4]={0,0,0,1};
            glMaterialfv(GL_FRONT, GL_AMBIENT, a);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, mat->diffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, mat->specular);
            float s = pow(2, 10.0*mat->shininess);

            if (s>128.0)
              s=128.0;
            
            glMaterialf(GL_FRONT, GL_SHININESS, s);
          }
          else
          {
            Lib3dsRgba a={0.2, 0.2, 0.2, 1.0};
            Lib3dsRgba d={0.8, 0.8, 0.8, 1.0};
            Lib3dsRgba s={0.0, 0.0, 0.0, 1.0};
            glMaterialfv(GL_FRONT, GL_AMBIENT, a);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, d);
            glMaterialfv(GL_FRONT, GL_SPECULAR, s);
          }
          
          glBegin(GL_TRIANGLES);
          glNormal3fv(f->normal);

          for (int i=0; i<3; ++i)
          {
                glNormal3fv(normalL[3*p+i]);
                glVertex3fv(mesh->pointL[f->points[i]].pos);
          }
          glEnd();
        }

        free(normalL);
      }

      glEndList();
    }

    if (node->user.d) {
      Lib3dsObjectData *d;

      glPushMatrix();
      d=&node->data.object;
      glMultMatrixf(&node->matrix[0][0]);
      glTranslatef(-d->pivot[0], -d->pivot[1], -d->pivot[2]);
      glCallList(node->user.d);
      /*glutSolidSphere(50.0, 20,20);*/
      glPopMatrix();
    }
  }
}

static void display(void)
{
  Lib3dsNode *c,*t;
  Lib3dsMatrix M;
  
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (!app.file) {
    return;
  }
  c=Lib3dsFile::lib3ds_file_node_by_name(app.file, app.camera, LIB3DS_CAMERA_NODE);
  t=Lib3dsFile::lib3ds_file_node_by_name(app.file, app.camera, LIB3DS_TARGET_NODE);
  if (!c || !t) {
    return;
  }

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective( c->data.camera.fov, 1.0*gl_width/gl_height, 100.0, 20000.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glRotatef(-90, 1.0,0,0);

  {
    GLfloat a[] = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat c[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat p[] = {0.0f, 0.0f, 0.0f, 1.0f};
    Lib3dsLight *l;

    int li=GL_LIGHT0;
    for (l = app.file->lights; l; l=l->next) {
      glEnable(li);

      glLightfv(li, GL_AMBIENT, a);
      glLightfv(li, GL_DIFFUSE, c);
      glLightfv(li, GL_SPECULAR, c);

      p[0] = l->position[0];
      p[1] = l->position[1];
      p[2] = l->position[2];
      glLightfv(li, GL_POSITION, p);

      if (!l->spot_light) {
        continue;
      }

      p[0] = l->spot[0] - l->position[0];
      p[1] = l->spot[1] - l->position[1];
      p[2] = l->spot[2] - l->position[2];      
      glLightfv(li, GL_SPOT_DIRECTION, p);
      ++li;
    }
  }

  lib3ds_matrix_camera(M, c->data.camera.pos, t->data.target.pos, c->data.camera.roll);
  glMultMatrixf(&M[0][0]);

  for (Lib3dsNode *p = app.file->nodes; p!=0; p=p->next)
      render_node(p);

  if (!halt) {
    current_frame+=1.0;
    if (current_frame > app.file->frames) {
      current_frame=0;
    }
    Lib3dsFile::lib3ds_file_eval(app.file, current_frame);
    glutSwapBuffers();
    glutPostRedisplay();
  }
}


/*!
 *
 */
static void
reshape (int w, int h)
{
  gl_width=w;
  gl_height=h;
  glViewport(0,0,w,h);
}


/*!
 *
 */
static void
keyboard(unsigned char key, int x, int y)
{
   switch (key) {
      case 27:
         exit(0);
         break;
      case 'h':
         halt=!halt;
         glutPostRedisplay();
         break;
   }
}


/*!
 *
 */
int
main(int argc, char** argv)
{
  glutInit(&argc, argv);
  if (argc!=2) {
    puts("***ERROR*** No 3DS file specified");
    exit(1);
  }
  app.filename=argv[1];

  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutInitWindowSize(500, 500); 
  glutInitWindowPosition(100, 100);
  glutCreateWindow(argv[0]);
  init();
  glutDisplayFunc(display); 
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutMainLoop();
  return(0);
}
