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

static GLuint texture;
 
GLuint raw_texture_load(const char *filename, int width, int height)
 {
     GLuint texture;
     unsigned char *data;
     FILE *file;
     file = fopen(filename, "rb");
     if (file == NULL) return 0;
     data = (unsigned char*) malloc(width * height * 3);
     fread(data, width * height * 3, 1, file);
     fclose(file);
     glGenTextures(1, &texture);
     glBindTexture(GL_TEXTURE_2D, texture);
     gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
     free(data);
     return texture;
 }

 void render()
 {
     glMatrixMode(GL_PROJECTION);
     glLoadIdentity();
     gluPerspective(60, 1, 1, 10);
     gluLookAt(0, 0, -2, /* eye */
               2, 0, 2, /* center */
               0, 1, 0); /* up */
     glMatrixMode(GL_MODELVIEW);
     glLoadIdentity();
     
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
         
    glBegin(GL_QUADS);
    glTexCoord2d(1, 1);
    glVertex3f(0.0, 0.0, 0.0);
    glTexCoord2d(1, 0);
    glVertex3f(0.0, 1.0, 0.0);
    glTexCoord2d(0, 0);
    glVertex3f(1.0, 1.0, 0.0);
    glTexCoord2d(0, 1);
    glVertex3f(1.0, 0.0, 0.0);
    glEnd();
         
    glDisable(GL_TEXTURE_2D);
     
    glFlush();
     
    glutSwapBuffers();
}
 
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
     glClearColor(0.0, 0.0, 0.0, 0.0);
     glShadeModel(GL_SMOOTH);
     texture = raw_texture_load("texture.raw", 200, 256);
     glutDisplayFunc(render);
     glutReshapeFunc(resize);
     glutKeyboardFunc(key);
     
     glutMainLoop();
     
     return 0;
}


