/*
 * ---------------- www.spacesimulator.net --------------
 *   ---- Space simulators and 3d engine tutorials ----
 *
 * Author: Damiano Vitulli
 *
 * This program is released under the BSD licence
 * By using this program you agree to licence terms on spacesimulator.net copyright page
 *
 *
 * Tutorial 4: 3d engine - 3ds models loader
 * 
 * 
 *
 * To compile this project you must include the following libraries:
 * opengl32.lib,glu32.lib,glut.lib
 * You need also the header file glut.h in your compiler directory.
 *  
 */

/*
 * 3DS Spaceship created by: Rene Reiter <renereiter@hotmail.com>
 */

/*
Linux port by Panteleakis Ioannis
mail: pioann@csd.auth.gr

just run: make and you are done.
of course you may need to change the makefile
*/



#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "tutorial4.h"
#include "texture.h"
#include "3dsloader.h"

int num_texture=-1;

int LoadBitmap(char *filename)
{
    FILE * file;
    char temp;
    long i;

    BITMAPINFOHEADER infoheader;

    num_texture++; // The counter of the current texture is increased

    if( (file = fopen(filename, "rb"))==NULL) return (-1); // Open the file for reading
    
    fseek(file, 18, SEEK_CUR);  /* start reading width & height */
    fread(&infoheader.biWidth, sizeof(int), 1, file); 
    
    fread(&infoheader.biHeight, sizeof(int), 1, file);
    
    fread(&infoheader.biPlanes, sizeof(short int), 1, file);
    if (infoheader.biPlanes != 1) {
        printf("Planes from %s is not 1: %u\n", filename, infoheader.biPlanes);
        return 0;
    }

    // read the bpp
    fread(&infoheader.biBitCount, sizeof(unsigned short int), 1, file);
    if (infoheader.biBitCount != 24) {
      printf("Bpp from %s is not 24: %d\n", filename, infoheader.biBitCount);
      return 0;
    }

    fseek(file, 24, SEEK_CUR);

    // read the data.
    infoheader.data = (char *) malloc(infoheader.biWidth * infoheader.biHeight * 3);
    if (infoheader.data == NULL) {
        printf("Error allocating memory for color-corrected image data\n");
        return 0;
    }

    if ((i = fread(infoheader.data, infoheader.biWidth * infoheader.biHeight * 3, 1, file)) != 1) {
        printf("Error reading image data from %s.\n", filename);
        return 0;
    }

    // reverse all of the colors. (bgr -> rgb)
    for (i=0; i<(infoheader.biWidth * infoheader.biHeight * 3); i+=3) { 
        temp = infoheader.data[i];
        infoheader.data[i] = infoheader.data[i+2];
        infoheader.data[i+2] = temp;
    }


    fclose(file); // Closes the file stream


    glBindTexture(GL_TEXTURE_2D, num_texture); // Bind the ID texture specified by the 2nd parameter

    // The next commands sets the texture parameters
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // The magnification function ("linear" produces better results)
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST); //The minifying function

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE); // We don't combine the color with the original surface color, use only the texture map.

    // Finally we define the 2d texture
    glTexImage2D(GL_TEXTURE_2D, 0, 3, infoheader.biWidth, infoheader.biHeight,
                 0, GL_RGB, GL_UNSIGNED_BYTE, infoheader.data);

    // And create 2d mipmaps for the minifying function
    gluBuild2DMipmaps(GL_TEXTURE_2D, 3, infoheader.biWidth, infoheader.biHeight,
                      GL_RGB, GL_UNSIGNED_BYTE, infoheader.data);

    free(infoheader.data); // Free the memory we used to load the texture

    return num_texture; // Returns the current texture OpenGL ID
}

int screen_width=640;
int screen_height=480;

// Absolute rotation values (0-359 degrees) and rotation increments for each frame
double rotation_x=0, rotation_x_increment=0.1;
double rotation_y=0, rotation_y_increment=0.05;
double rotation_z=0, rotation_z_increment=0.03;
 
// Flag for rendering as lines or filled polygons
int filling=1; //0=OFF 1=ON

//Now the object is generic, the cube has annoyed us a little bit, or not?
obj_type object;

void resize (int width, int height)
{
    screen_width=width; // We obtain the new screen width values and store it
    screen_height=height; // Height value
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0,0,screen_width,screen_height); // Viewport transformation
    glMatrixMode(GL_PROJECTION); // Projection transformation
    glLoadIdentity(); // We initialize the projection matrix as identity
    gluPerspective(45.0f,(GLfloat)screen_width/(GLfloat)screen_height,10.0f,10000.0f);
    glutPostRedisplay (); 
}



/**********************************************************
 *
 * SUBROUTINE keyboard(unsigned char,int,int)
 *
 * Used to handle the keyboard input (ASCII Characters)
 * 
 *********************************************************/

void keyboard (unsigned char key, int x, int y)
{
    switch (key)
    {
    case ' ':
        rotation_x_increment=0;
        rotation_y_increment=0;
        rotation_z_increment=0;
        break;
    case 'r': case 'R':
        if (filling==0)
        {
                // Polygon rasterization mode (polygon filled)
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); 
                filling=1;
        }   
        else 
        {
                // Polygon rasterization mode (polygon outlined)
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); 
                filling=0;
        }
        break;
    }
}

void keyboard_s(int key, int x, int y)
{
    switch (key)
    {
        case GLUT_KEY_UP:
            rotation_x_increment = rotation_x_increment +0.005;
        break;
        case GLUT_KEY_DOWN:
            rotation_x_increment = rotation_x_increment -0.005;
        break;
        case GLUT_KEY_LEFT:
            rotation_y_increment = rotation_y_increment +0.005;
        break;
        case GLUT_KEY_RIGHT:
            rotation_y_increment = rotation_y_increment -0.005;
        break;
    }
}

void display(void)
{
    int l_index;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW); // Modeling transformation
    glLoadIdentity(); // Initialize the model matrix as identity
    
    glTranslatef(0.0,0.0,-300); // We move the object forward (the model matrix is multiplied by the translation matrix)
 
    rotation_x = rotation_x + rotation_x_increment;
    rotation_y = rotation_y + rotation_y_increment;
    rotation_z = rotation_z + rotation_z_increment;

    if (rotation_x > 359) rotation_x = 0;
    if (rotation_y > 359) rotation_y = 0;
    if (rotation_z > 359) rotation_z = 0;

    glRotatef(rotation_x,1.0,0.0,0.0);
    glRotatef(rotation_y,0.0,1.0,0.0);
    glRotatef(rotation_z,0.0,0.0,1.0);
    
    glBindTexture(GL_TEXTURE_2D, object.id_texture); // We set the active texture 

    glBegin(GL_TRIANGLES); // glBegin and glEnd delimit the vertices that define a primitive (in our case triangles)
    for (l_index=0;l_index<object.polygons_qty;l_index++)
    {
        glTexCoord2f( object.mapcoord[ object.polygon[l_index].a ].u,
                      object.mapcoord[ object.polygon[l_index].a ].v);

        glVertex3f( object.vertex[ object.polygon[l_index].a ].x,
                    object.vertex[ object.polygon[l_index].a ].y,
                    object.vertex[ object.polygon[l_index].a ].z);

        glTexCoord2f( object.mapcoord[ object.polygon[l_index].b ].u,
                      object.mapcoord[ object.polygon[l_index].b ].v);
        glVertex3f( object.vertex[ object.polygon[l_index].b ].x,
                    object.vertex[ object.polygon[l_index].b ].y,
                    object.vertex[ object.polygon[l_index].b ].z);
        
        glTexCoord2f( object.mapcoord[ object.polygon[l_index].c ].u,
                      object.mapcoord[ object.polygon[l_index].c ].v);
        glVertex3f( object.vertex[ object.polygon[l_index].c ].x,
                    object.vertex[ object.polygon[l_index].c ].y,
                    object.vertex[ object.polygon[l_index].c ].z);
    }
    glEnd();

    glFlush();
    glutSwapBuffers(); // In double buffered mode we invert the positions of the visible buffer and the writing buffer
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);    
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(screen_width,screen_height);
    glutInitWindowPosition(0,0);
    glutCreateWindow("www.spacesimulator.net - 3d engine tutorials: Tutorial 4");    
    glutDisplayFunc(display);
    glutIdleFunc(display);
    glutReshapeFunc (resize);
    glutKeyboardFunc (keyboard);
    glutSpecialFunc (keyboard_s);
    glClearColor(0.0, 0.0, 0.0, 0.0); // This clear the background color to black
    glShadeModel(GL_SMOOTH); // Type of shading for the polygons
    glViewport(0,0,screen_width,screen_height);  
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity(); // We initialize the projection matrix as identity
    gluPerspective(45.0f,(GLfloat)screen_width/(GLfloat)screen_height,10.0f,10000.0f); 
    glEnable(GL_DEPTH_TEST); // We enable the depth test (also called z buffer)
    glPolygonMode (GL_FRONT_AND_BACK, GL_FILL); // Polygon rasterization mode (polygon filled)
    glEnable(GL_TEXTURE_2D); // This Enable the Texture mapping
    Load3DS (&object,"spaceship.3ds");
    object.id_texture=LoadBitmap("spaceshiptexture.bmp"); 
    
    // If the last function returns -1 it means the file was not found so we exit from the program
    if (object.id_texture==-1)
    {
    	printf("Image file: spaceshiptexture.bmp not found\n");
        exit (0);
    }
    glutMainLoop();

    return(0);    
}
