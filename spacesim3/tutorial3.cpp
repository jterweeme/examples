/*
 * ---------------- www.spacesimulator.net --------------
 *   ---- Space simulators and 3d engine tutorials ----
 *
 * Author: Damiano Vitulli
 *
 * This program is released under the BSD licence
 * By using this program you agree to licence terms on spacesimulator.net copyright page
 *
 * Tutorial 3: 3d engine - Texture mapping with OpenGL!
 * 
 *
 * To compile this project you must include the following libraries:
 * opengl32.lib,glu32.lib,glut.lib
 * You need also the header file glut.h in your compiler directory.
 *  
 */

/*
Linux port by Panteleakis Ioannis
mail: pioann@csd.auth.gr

just run: make and you are done.
of course you may need to change the makefile
*/
 
 
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <GL/glu.h>
#include <GL/glut.h>

#define MAX_VERTICES 2000 // Max number of vertices (for each object)
#define MAX_POLYGONS 2000 // Max number of polygons (for each object)


int num_texture = -1;

typedef struct                       /**** BMP file info structure ****/
    {
    unsigned int   biSize;           /* Size of info header */
    int            biWidth;          /* Width of image */
    int            biHeight;         /* Height of image */
    unsigned short biPlanes;         /* Number of color planes */
    unsigned short biBitCount;       /* Number of bits per pixel */
    unsigned int   biCompression;    /* Type of compression to use */
    unsigned int   biSizeImage;      /* Size of image data */
    int            biXPelsPerMeter;  /* X pixels per meter */
    int            biYPelsPerMeter;  /* Y pixels per meter */
    unsigned int   biClrUsed;        /* Number of colors used */
    unsigned int   biClrImportant;   /* Number of important colors */
    char *data;
    } BITMAPINFOHEADER;

int LoadBitmap(const char *filename)
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
    fread(&infoheader.biBitCount, sizeof(unsigned short int), 1, file);
    fseek(file, 24, SEEK_CUR);

    infoheader.data = (char *) malloc(infoheader.biWidth * infoheader.biHeight * 3);

    if ((i = fread(infoheader.data, infoheader.biWidth * infoheader.biHeight * 3, 1, file)) != 1) {
        printf("Error reading image data from %s.\n", filename);
        return 0;
    }

    // reverse all of the colors. (bgr -> rgb)
#if 1
    for (i=0; i<(infoheader.biWidth * infoheader.biHeight * 3); i+=3)
    {
        temp = infoheader.data[i];
        infoheader.data[i] = infoheader.data[i+2];
        infoheader.data[i+2] = temp;
    }
#endif

    fclose(file);
    glBindTexture(GL_TEXTURE_2D, num_texture);

    gluBuild2DMipmaps(GL_TEXTURE_2D, 3, infoheader.biWidth, infoheader.biHeight,
                      GL_RGB, GL_UNSIGNED_BYTE, infoheader.data);

    free(infoheader.data);
    return num_texture;
}

/*** Our vertex type ***/
typedef struct{
    float x,y,z;
}vertex_type;

/*** The polygon (triangle), 3 number that aim 3 vertex ***/
typedef struct{
    int a,b,c;
}polygon_type;

/*** The mapcoord type, 2 texture coordinates for each vertex ***/
typedef struct{
    float u,v;
}mapcoord_type;

/*** The object type ***/
typedef struct { 
    vertex_type vertex[MAX_VERTICES]; 
    polygon_type polygon[MAX_POLYGONS];
    mapcoord_type mapcoord[MAX_VERTICES];
    int id_texture;
} obj_type;



/**********************************************************
 *
 * VARIABLES DECLARATION
 *
 *********************************************************/

/*** The width and height of your window, change it as you like  ***/
int screen_width=640;
int screen_height=480;

/*** Absolute rotation values (0-359 degrees) and rotation increments for each frame ***/
double rotation_x=0, rotation_x_increment=0.1;
double rotation_y=0, rotation_y_increment=0.05;
double rotation_z=0, rotation_z_increment=0.03;
 
/*** Flag for rendering as lines or filled polygons ***/
int filling=1; //0=OFF 1=ON

/*** And, finally our first object! ***/
obj_type cube = 
{
    {
        -10, -10, 10,   // vertex v0
        10,  -10, 10,   // vertex v1
        10,  -10, -10,  // vertex v2
        -10, -10, -10,  // vertex v3
        -10, 10,  10,   // vertex v4
        10,  10,  10,   // vertex v5
        10,  10,  -10,  // vertex v6 
        -10, 10,  -10   // vertex v7
    }, 
    {
        0, 1, 4,  // polygon v0,v1,v4
        1, 5, 4,  // polygon v1,v5,v4
        1, 2, 5,  // polygon v1,v2,v5
        2, 6, 5,  // polygon v2,v6,v5
        2, 3, 6,  // polygon v2,v3,v6
        3, 7, 6,  // polygon v3,v7,v6
        3, 0, 7,  // polygon v3,v0,v7
        0, 4, 7,  // polygon v0,v4,v7
        4, 5, 7,  // polygon v4,v5,v7
        5, 6, 7,  // polygon v5,v6,v7
        3, 2, 0,  // polygon v3,v2,v0
        2, 1, 0   // polygon v2,v1,v0
    },
    {
        0.0, 0.0,  // mapping coordinates for vertex v0
        1.0, 0.0,  // mapping coordinates for vertex v1
        1.0, 0.0,  // mapping coordinates for vertex v2
        0.0, 0.0,  // mapping coordinates for vertex v3
        0.0, 1.0,  // mapping coordinates for vertex v4
        1.0, 1.0,  // mapping coordinates for vertex v5
        1.0, 1.0,  // mapping coordinates for vertex v6 
        0.0, 1.0   // mapping coordinates for vertex v7
    },
    0, 
};

void resize (int width, int height)
{
    std::cerr << "Resize\r\n";
    std::cerr.flush();
    screen_width=width; // We obtain the new screen width values and store it
    screen_height=height; // Height value

    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // We clear both the color and the depth buffer so to draw the next frame
    glViewport(0,0,screen_width,screen_height); // Viewport transformation

    glMatrixMode(GL_PROJECTION); // Projection transformation
    glLoadIdentity(); // We initialize the projection matrix as identity
    gluPerspective(45.0f,(GLfloat)screen_width/(GLfloat)screen_height,1.0f,1000.0f);

    glutPostRedisplay (); // This command redraw the scene (it calls the same routine of glutDisplayFunc)
}

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
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Polygon rasterization mode (polygon filled)
                filling=1;
            }   
            else 
            {
                glPolygonMode (GL_FRONT_AND_BACK, GL_LINE); // Polygon rasterization mode (polygon outlined)
                filling=0;
            }
        break;
    case 27:
        exit(0);
        break;
    }
}

void keyboard_s (int key, int x, int y)
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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    glMatrixMode(GL_MODELVIEW); // Modeling transformation
    glLoadIdentity(); // Initialize the model matrix as identity
    
    glTranslatef(0.0,0.0,-50); // We move the object 50 points forward (the model matrix is multiplied by the translation matrix)
 
    rotation_x = rotation_x + rotation_x_increment;
    rotation_y = rotation_y + rotation_y_increment;
    rotation_z = rotation_z + rotation_z_increment;

    if (rotation_x > 359) rotation_x = 0;
    if (rotation_y > 359) rotation_y = 0;
    if (rotation_z > 359) rotation_z = 0;

    glRotatef(rotation_x,1.0,0.0,0.0); // Rotations of the object (the model matrix is multiplied by the rotation matrices)
    glRotatef(rotation_y,0.0,1.0,0.0);
    glRotatef(rotation_z,0.0,0.0,1.0);
    
    //glBindTexture(GL_TEXTURE_2D, cube.id_texture); // We set the active texture

    // GlBegin and glEnd delimit the vertices that define a primitive (in our case triangles)
    glBegin(GL_TRIANGLES);

    for (int l_index=0;l_index<12;l_index++)
    {
        /*** FIRST VERTEX ***/
        // Texture coordinates of the first vertex
        glTexCoord2f( cube.mapcoord[ cube.polygon[l_index].a ].u,
                      cube.mapcoord[ cube.polygon[l_index].a ].v);
        // Coordinates of the first vertex
        glVertex3f( cube.vertex[ cube.polygon[l_index].a ].x,
                    cube.vertex[ cube.polygon[l_index].a ].y,
                    cube.vertex[ cube.polygon[l_index].a ].z); //Vertex definition

        /*** SECOND VERTEX ***/
        // Texture coordinates of the second vertex
        glTexCoord2f( cube.mapcoord[ cube.polygon[l_index].b ].u,
                      cube.mapcoord[ cube.polygon[l_index].b ].v);
        // Coordinates of the second vertex
        glVertex3f( cube.vertex[ cube.polygon[l_index].b ].x,
                    cube.vertex[ cube.polygon[l_index].b ].y,
                    cube.vertex[ cube.polygon[l_index].b ].z);
        
        /*** THIRD VERTEX ***/
        // Texture coordinates of the third vertex
        glTexCoord2f( cube.mapcoord[ cube.polygon[l_index].c ].u,
                      cube.mapcoord[ cube.polygon[l_index].c ].v);
        // Coordinates of the Third vertex
        glVertex3f( cube.vertex[ cube.polygon[l_index].c ].x,
                    cube.vertex[ cube.polygon[l_index].c ].y,
                    cube.vertex[ cube.polygon[l_index].c ].z);
    }
    glEnd();

    glFlush(); // This force the execution of OpenGL commands
    glutSwapBuffers(); // In double buffered mode we invert the positions of the visible buffer and the writing buffer
}



/**********************************************************
 *
 * The main routine
 * 
 *********************************************************/

int main(int argc, char **argv)
{
    // We use the GLUT utility to initialize the window, to handle the input and to interact with the windows system
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(screen_width,screen_height);
    glutInitWindowPosition(0,0);
    glutCreateWindow("www.spacesimulator.net - 3d engine tutorials: Tutorial 3");    
    glutDisplayFunc(display);
    glutIdleFunc(display);
    glutReshapeFunc (resize);
    glutKeyboardFunc (keyboard);
    glutSpecialFunc (keyboard_s);

   	
    // Viewport transformation
    glViewport(0,0,screen_width,screen_height);  

    // Projection transformation
    glLoadIdentity(); // We initialize the projection matrix as identity
    glEnable(GL_DEPTH_TEST); // We enable the depth test (also called z buffer)
    glEnable(GL_TEXTURE_2D); // This Enable the Texture mapping
    cube.id_texture=LoadBitmap("texture1.bmp");

    glutMainLoop();

    return(0);    
}
