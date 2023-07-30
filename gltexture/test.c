/* compile with: gcc -lGL -lglut -Wall -o texture texture.c */
 
#include <GL/gl.h>
#include <GL/glut.h>
 
 /* This program does not feature some physical simulation screaming
    for continuous updates, disable that waste of resources */
#define STUFF_IS_MOVING 0
 
#if STUFF_IS_MOVING
    #include <unistd.h>
#endif
 
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdio.h>

/* using the routine above - replace this declaration with the snippet above */
GLuint raw_texture_load(const char *filename, int width, int height);
 
static GLuint texture;
 
GLuint raw_texture_load(const char *filename, int width, int height)
 {
     GLuint texture;
     unsigned char *data;
     FILE *file;
     
     // open texture data
     file = fopen(filename, "rb");
     if (file == NULL) return 0;
     
     // allocate buffer
     data = (unsigned char*) malloc(width * height * 4);
     
     // read texture data
     fread(data, width * height * 3, 1, file);
     fclose(file);
     
     // allocate a texture name
     glGenTextures(1, &texture);
     
     // select our current texture
     glBindTexture(GL_TEXTURE_2D, texture);
     
     // select modulate to mix texture with color for shading
     glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
     
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_DECAL);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_DECAL);
     
     // when texture area is small, bilinear filter the closest mipmap
     glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
     // when texture area is large, bilinear filter the first mipmap
     glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     
     // texture should tile
     glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
     glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
     
     // build our texture mipmaps
     gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
     
     // free buffer
     free(data);
     
     return texture;
 }

 void render()
 {
     glMatrixMode(GL_PROJECTION);
     glLoadIdentity();
     /* fov, aspect, near, far */
     gluPerspective(60, 1, 1, 10);
     gluLookAt(0, 0, -2, /* eye */
               2, 0, 2, /* center */
               0, 1, 0); /* up */
     glMatrixMode(GL_MODELVIEW);
     glLoadIdentity();
     
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
     glPushAttrib(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
         glEnable(GL_TEXTURE_2D);
         
         /* create a square on the XY
            note that OpenGL origin is at the lower left
            but texture origin is at upper left
            => it has to be mirrored
            (gasman knows why i have to mirror X as well) */
         glBegin(GL_QUADS);
         glNormal3f(0.0, 0.0, 1.0);
         glTexCoord2d(1, 1); glVertex3f(0.0, 0.0, 0.0);
         glTexCoord2d(1, 0); glVertex3f(0.0, 1.0, 0.0);
         glTexCoord2d(0, 0); glVertex3f(1.0, 1.0, 0.0);
         glTexCoord2d(0, 1); glVertex3f(1.0, 0.0, 0.0);
         glEnd();
         
         glDisable(GL_TEXTURE_2D);
     glPopAttrib();
     
     glFlush();
     
     glutSwapBuffers();
 }
 
 void init()
 {
     glClearColor(0.0, 0.0, 0.0, 0.0);
     glShadeModel(GL_SMOOTH);
     
     glEnable(GL_LIGHTING);
     glEnable(GL_LIGHT0);
     glEnable(GL_DEPTH_TEST);
     
     glLightfv(GL_LIGHT0, GL_POSITION, (GLfloat[]){2.0, 2.0, 2.0, 0.0});
     glLightfv(GL_LIGHT0, GL_AMBIENT, (GLfloat[]){1.0, 1.0, 1.0, 0.0});
     
     texture = raw_texture_load("texture.raw", 200, 256);
 }
 
 #if STUFF_IS_MOVING
 void idle()
 {
     render();
     usleep((1 / 50.0) * 1000000);
 }
 #endif
 
 void resize(int w, int h)
 {
     glViewport(0, 0, (GLsizei) w, (GLsizei) h);
 }
 
 void key(unsigned char key, int x, int y)
 {
     if (key == 'q') exit(0);
 }
 
 int main(int argc, char *argv[])
 {
     glutInit(&argc, argv);
     glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
     glutInitWindowSize(640, 480);
     glutCreateWindow("Texture demo - [q]uit");
     
     init();
     glutDisplayFunc(render);
     glutReshapeFunc(resize);
     #if STUFF_IS_MOVING
         glutIdleFunc(idle);
     #endif
     glutKeyboardFunc(key);
     
     glutMainLoop();
     
     return 0;
 }


