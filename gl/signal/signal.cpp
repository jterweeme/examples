/* 
 *  Demonstration of picking and rendering luminous objects.  Drag the
 *  middle mouse button to spin the object.  Move the mouse over the
 *  bulbs to light them.
 * 
 *  author: Nate Robins
 *  email: ndr@pobox.com
 *  www: http://www.pobox.com/~ndr 
 */


/* includes */
#include <math.h>
#include <stdio.h>
#include <GL/glut.h>
#include "trackball.h"


/* defines */
#define SELECT_BUFFER 32


/* globals */
GLuint signal_list = 0;			/* display list for traffic signal */

GLuint select_buffer[SELECT_BUFFER];	/* selection buffer */

GLint mouse_state = -1;
GLint mouse_button = -1;

GLuint picked = 0;			/* current light that is picked */

GLUquadricObj* quadric;


/* functions */

void
lists()
{
  /* build a new quadric */
  quadric = gluNewQuadric();
  gluQuadricDrawStyle(quadric, GLU_FILL);
  gluQuadricNormals(quadric, GLU_SMOOTH);
  gluQuadricOrientation(quadric, GLU_OUTSIDE);

  /* signal */
  if (signal_list)
    glDeleteLists(signal_list, 1);

  /* generate a display list */
  signal_list = glGenLists(1);
  glNewList(signal_list, GL_COMPILE);
  glBegin(GL_QUADS);
  glColor3f(0.4, 0.4, 0.4);
  glNormal3f(0.0, 0.0, 1.0);
  glVertex3f(-1.0,  3.0,  1.0);
  glVertex3f(-1.0, -3.0,  1.0);
  glVertex3f( 1.0, -3.0,  1.0);
  glVertex3f( 1.0,  3.0,  1.0);
  glNormal3f(1.0, 0.0, 0.0);
  glVertex3f( 1.0,  3.0,  1.0);
  glVertex3f( 1.0, -3.0,  1.0);
  glVertex3f( 1.0, -3.0, -1.0);
  glVertex3f( 1.0,  3.0, -1.0);
  glNormal3f(0.0, 0.0, -1.0);
  glVertex3f( 1.0,  3.0, -1.0);
  glVertex3f( 1.0, -3.0, -1.0);
  glVertex3f(-1.0, -3.0, -1.0);
  glVertex3f(-1.0,  3.0, -1.0);
  glNormal3f(-1.0, 0.0, 0.0);
  glVertex3f(-1.0,  3.0, -1.0);
  glVertex3f(-1.0, -3.0, -1.0);
  glVertex3f(-1.0, -3.0,  1.0);
  glVertex3f(-1.0,  3.0,  1.0);
  glNormal3f( 0.0, 1.0, 0.0);
  glVertex3f( 1.0,  3.0, -1.0);
  glVertex3f(-1.0,  3.0, -1.0);
  glVertex3f(-1.0,  3.0,  1.0);
  glVertex3f( 1.0,  3.0,  1.0);
  glNormal3f( 0.0, -1.0, 0.0);
  glVertex3f( 1.0, -3.0,  1.0);
  glVertex3f(-1.0, -3.0,  1.0);
  glVertex3f(-1.0, -3.0, -1.0);
  glVertex3f( 1.0, -3.0, -1.0);
  glEnd();
  glDisable(GL_CULL_FACE);
  glTranslatef(0.0, 1.75, 1.0);
  gluCylinder(quadric, 0.75, 0.75, 0.75, 32, 32);
  glTranslatef(0.0, -1.75, 0.0);
  gluCylinder(quadric, 0.75, 0.75, 0.75, 32, 32);
  glTranslatef(0.0, -1.75, 0.0);
  gluCylinder(quadric, 0.75, 0.75, 0.75, 32, 32);
  glTranslatef(0.0, 1.75, -1.0);
  glEnable(GL_CULL_FACE);
  glEndList();

  gluQuadricOrientation(quadric, GLU_INSIDE);
}

void
init(void)
{
  tbInit(GLUT_MIDDLE_BUTTON);

  /* create initial display lists */
  lists();

  /* lighting */
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_COLOR_MATERIAL);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  glEnable(GL_CULL_FACE);

  glClearColor(0.3, 0.3, 1.0, 0.0);

  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);

  /* set the selection buffer size */
  glSelectBuffer(SELECT_BUFFER, select_buffer);
}

void
reshape(int width, int height)
{
  GLfloat light_position[4] = { 1.0, 1.0, 1.0, 0.0 };

  tbReshape(width, height);

  glViewport(0, 0, width, height);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0, (GLfloat)height / (GLfloat)width, 1.0, 128.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glLightfv(GL_LIGHT0, GL_POSITION, light_position);

  glTranslatef(0.0, 0.0, -8.0);
  glRotatef(15.0, 0.0, 1.0, 0.0);
}

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glPushMatrix();

  tbMatrix();

  glDisable(GL_LIGHT1);

  /* draw the lights */
  glTranslatef(0.0, 1.75, 1.0);
  if (picked == 1) {
    glDisable(GL_LIGHTING);
    glColor3f(0.3, 0.0, 0.0);
    gluCylinder(quadric, 0.74, 0.74, 0.75, 32, 32);
    glColor3f(1.0, 0.0, 0.0);
    glutSolidSphere(0.75, 32, 32);
    glEnable(GL_LIGHTING);
  } else {
    glColor3f(0.4, 0.4, 0.4);
    gluCylinder(quadric, 0.74, 0.74, 0.75, 32, 32);
    glColor3f(0.3, 0.0, 0.0);
    glutSolidSphere(0.75, 32, 32);
  }
  glTranslatef(0.0, -1.75, 0.0);
  if (picked == 2) {
    glDisable(GL_LIGHTING);
    glColor3f(0.3, 0.3, 0.0);
    gluCylinder(quadric, 0.74, 0.74, 0.75, 32, 32);
    glColor3f(1.0, 1.0, 0.0);
    glutSolidSphere(0.75, 32, 32);
    glEnable(GL_LIGHTING);
  } else {
    glColor3f(0.4, 0.4, 0.4);
    gluCylinder(quadric, 0.74, 0.74, 0.75, 32, 32);
    glColor3f(0.3, 0.3, 0.0);
    glutSolidSphere(0.75, 32, 32);
  }
  glTranslatef(0.0, -1.75, 0.0);
  if (picked == 3) {
    glDisable(GL_LIGHTING);
    glColor3f(0.0, 0.3, 0.0);
    gluCylinder(quadric, 0.74, 0.74, 0.75, 32, 32);
    glColor3f(0.0, 1.0, 0.0);
    glutSolidSphere(0.75, 32, 32);
    glEnable(GL_LIGHTING);
  } else {
    glColor3f(0.4, 0.4, 0.4);
    gluCylinder(quadric, 0.74, 0.74, 0.75, 32, 32);
    glColor3f(0.0, 0.3, 0.0);
    glutSolidSphere(0.75, 32, 32);
  }
  glTranslatef(0.0, 1.75, -1.0);
  
  glCallList(signal_list);

  glPopMatrix();

  glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y)
{
  switch (key) {
  case 'h':
    printf("help\n\n");
    printf("f            -  Filled\n");
    printf("w            -  Wireframe\n");
    printf("backspace    -  Reset\n");
    printf("escape or q  -  Quit\n\n");
    break;

  case 'f':
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    break;

  case 'w':
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    break;

  case '\b':
    init();
    break;

  case 'q':
  case 27:
    exit(0);
    break;
  }

  glutPostRedisplay();
}

GLuint
pick(int x, int y)
{
  GLuint    i, hits, num_names, picked;
  GLuint*   p;
  GLboolean save;
  GLuint    depth = (GLuint)-1;
  GLint     viewport[4];

  /* get the current viewport parameters */
  glGetIntegerv(GL_VIEWPORT, viewport);

  /* set the render mode to selection */
  glRenderMode(GL_SELECT);
  glInitNames();
  glPushName(0);

  /* setup a picking matrix and render into selection buffer */
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();

  glLoadIdentity();
  gluPickMatrix(x, viewport[3] - y, 5.0, 5.0, viewport);
  gluPerspective(60.0, (GLfloat)viewport[3]/(GLfloat)viewport[2], 1.0, 128.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0, 0.0, -8.0);
  glRotatef(15.0, 0.0, 1.0, 0.0);

  glPushMatrix();
  tbMatrix();
  glTranslatef(0.0,  1.75, 1.0);
  glLoadName(1);
  glutSolidSphere(0.75, 8, 8);
  glTranslatef(0.0, -1.75, 0.0);
  glLoadName(2);
  glutSolidSphere(0.75, 8, 8);
  glTranslatef(0.0, -1.75, 0.0);
  glLoadName(3);
  glutSolidSphere(0.75, 8, 8);
  glPopMatrix();

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);

  hits = glRenderMode(GL_RENDER);

  p = select_buffer;
  picked = 0;
  for (i = 0; i < hits; i++) {
    save = GL_FALSE;
    num_names = *p;			/* number of names in this hit */
    p++;

    if (*p <= depth) {			/* check the 1st depth value */
      depth = *p;
      save = GL_TRUE;
    }
    p++;
    if (*p <= depth) {			/* check the 2nd depth value */
      depth = *p;
      save = GL_TRUE;
    }
    p++;

    if (save)
      picked = *p;

    p += num_names;			/* skip over the rest of the names */
  }

  return picked;
}

void
mouse(int button, int state, int x, int y)
{
  mouse_state = state;
  mouse_button = button;

  tbMouse(button, state, x, y);
}

void
motion(int x, int y)
{
  tbMotion(x, y);
  glutPostRedisplay();
}

void
passive(int x, int y) 
{
  picked = pick(x, y);
  glutPostRedisplay();
}

int
main(int argc, char** argv)
{
  glutInit(&argc, argv);

  glutInitWindowSize(512, 512);
  glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
  glutCreateWindow("Pick");
  
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutPassiveMotionFunc(passive);
  
  init();
  
  glutMainLoop();
  return 0;
}
