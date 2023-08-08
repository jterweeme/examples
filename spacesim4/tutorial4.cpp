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
#include <sys/stat.h>
#include <GL/glut.h>

extern int num_texture;


/**********************************************************
 *
 * FUNCTION LoadBitmap(char *)
 *
 * This function loads a bitmap file and return the OpenGL reference ID to use that texture
 *
 *********************************************************/

extern int LoadBitmap(char *filename);

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

#define MAX_VERTICES 8000 // Max number of vertices (for each object)
#define MAX_POLYGONS 8000 // Max number of polygons (for each object)

// Our vertex type
typedef struct{
    float x,y,z;
}vertex_type;

// The polygon (triangle), 3 numbers that aim 3 vertices
typedef struct{
    int a,b,c;
}polygon_type;

// The mapcoord type, 2 texture coordinates for each vertex
typedef struct{
    float u,v;
}mapcoord_type;

// The object type
typedef struct {
    char name[20];

    int vertices_qty;
    int polygons_qty;

    vertex_type vertex[MAX_VERTICES];
    polygon_type polygon[MAX_POLYGONS];
    mapcoord_type mapcoord[MAX_VERTICES];
    int id_texture;
} obj_type, *obj_type_ptr;

int num_texture=-1;

long filelength(int f)
{
    struct stat buf;

    fstat(f, &buf);

    return(buf.st_size);
}

char Load3DS(obj_type_ptr p_object, const char *p_filename)
{
    int i; //Index variable

    FILE *l_file; //File pointer

    unsigned short l_chunk_id; //Chunk identifier
    unsigned int l_chunk_lenght; //Chunk lenght

    unsigned char l_char; //Char variable
    unsigned short l_qty; //Number of elements in each chunk

    unsigned short l_face_flags; //Flag that stores some face information

    if ((l_file=fopen (p_filename, "rb"))== NULL) return 0; //Open the file

    while (ftell (l_file) < filelength (fileno (l_file))) //Loop to scan the whole file
    //while(!EOF)
    {
        //getch(); //Insert this command for debug (to wait for keypress for each chuck reading)

        fread (&l_chunk_id, 2, 1, l_file); //Read the chunk header
        printf("ChunkID: %x\n",l_chunk_id);
        fread (&l_chunk_lenght, 4, 1, l_file); //Read the lenght of the chunk
        printf("ChunkLenght: %x\n",l_chunk_lenght);

        switch (l_chunk_id)
        {
            //----------------- MAIN3DS -----------------
            // Description: Main chunk, contains all the other chunks
            // Chunk ID: 4d4d 
            // Chunk Lenght: 0 + sub chunks
            //-------------------------------------------
            case 0x4d4d:
            break;

            //----------------- EDIT3DS -----------------
            // Description: 3D Editor chunk, objects layout info 
            // Chunk ID: 3d3d (hex)
            // Chunk Lenght: 0 + sub chunks
            //-------------------------------------------
            case 0x3d3d:
            break;

            //--------------- EDIT_OBJECT ---------------
            // Description: Object block, info for each object
            // Chunk ID: 4000 (hex)
            // Chunk Lenght: len(object name) + sub chunks
            //-------------------------------------------
            case 0x4000:
                i=0;
                do
                {
                    fread (&l_char, 1, 1, l_file);
                    p_object->name[i]=l_char;
                    i++;
                }while(l_char != '\0' && i<20);
            break;

            //--------------- OBJ_TRIMESH ---------------
            // Description: Triangular mesh, contains chunks for 3d mesh info
            // Chunk ID: 4100 (hex)
            // Chunk Lenght: 0 + sub chunks
            //-------------------------------------------
            case 0x4100:
            break;

            //--------------- TRI_VERTEXL ---------------
            // Description: Vertices list
            // Chunk ID: 4110 (hex)
            // Chunk Lenght: 1 x unsigned short (number of vertices) 
            //             + 3 x float (vertex coordinates) x (number of vertices)
            //             + sub chunks
            //-------------------------------------------
            case 0x4110:
                fread (&l_qty, sizeof (unsigned short), 1, l_file);
                p_object->vertices_qty = l_qty;
                printf("Number of vertices: %d\n",l_qty);
                for (i=0; i<l_qty; i++)
                {
                    fread (&p_object->vertex[i].x, sizeof(float), 1, l_file);
                    printf("Vertices list x: %f\n",p_object->vertex[i].x);
                    fread (&p_object->vertex[i].y, sizeof(float), 1, l_file);
                    printf("Vertices list y: %f\n",p_object->vertex[i].y);
                    fread (&p_object->vertex[i].z, sizeof(float), 1, l_file);
                    printf("Vertices list z: %f\n",p_object->vertex[i].z);
                }
                break;

            //--------------- TRI_FACEL1 ----------------
            // Description: Polygons (faces) list
            // Chunk ID: 4120 (hex)
            // Chunk Lenght: 1 x unsigned short (number of polygons) 
            //             + 3 x unsigned short (polygon points) x (number of polygons)
            //             + sub chunks
            //-------------------------------------------
            case 0x4120:
                fread (&l_qty, sizeof (unsigned short), 1, l_file);
                p_object->polygons_qty = l_qty;
                printf("Number of polygons: %d\n",l_qty);
                for (i=0; i<l_qty; i++)
                {
                    fread (&p_object->polygon[i].a, sizeof (unsigned short), 1, l_file);
                    printf("Polygon point a: %d\n",p_object->polygon[i].a);
                    fread (&p_object->polygon[i].b, sizeof (unsigned short), 1, l_file);
                    printf("Polygon point b: %d\n",p_object->polygon[i].b);
                    fread (&p_object->polygon[i].c, sizeof (unsigned short), 1, l_file);
                    printf("Polygon point c: %d\n",p_object->polygon[i].c);
                    fread (&l_face_flags, sizeof (unsigned short), 1, l_file);
                    printf("Face flags: %x\n",l_face_flags);
                }
                break;

            //------------- TRI_MAPPINGCOORS ------------
            // Description: Vertices list
            // Chunk ID: 4140 (hex)
            // Chunk Lenght: 1 x unsigned short (number of mapping points) 
            //             + 2 x float (mapping coordinates) x (number of mapping points)
            //             + sub chunks
            //-------------------------------------------
            case 0x4140:
                fread (&l_qty, sizeof (unsigned short), 1, l_file);
                for (i=0; i<l_qty; i++)
                {
                    fread (&p_object->mapcoord[i].u, sizeof (float), 1, l_file);
                    printf("Mapping list u: %f\n",p_object->mapcoord[i].u);
                    fread (&p_object->mapcoord[i].v, sizeof (float), 1, l_file);
                    printf("Mapping list v: %f\n",p_object->mapcoord[i].v);
                }
                break;

            //----------- Skip unknow chunks ------------
            //We need to skip all the chunks that currently we don't use
            //We use the chunk lenght information to set the file pointer
            //to the same level next chunk
            //-------------------------------------------
            default:
                 fseek(l_file, l_chunk_lenght-6, SEEK_CUR);
        }
    }
    fclose (l_file); // Closes the file stream
    return (1); // Returns ok
}

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
    glutReshapeFunc(resize);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(keyboard_s);
    glClearColor(0.0, 0.0, 0.0, 0.0); // This clear the background color to black
    glShadeModel(GL_SMOOTH); // Type of shading for the polygons
    glViewport(0,0,screen_width,screen_height);  
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity(); // We initialize the projection matrix as identity
    gluPerspective(45.0f,(GLfloat)screen_width/(GLfloat)screen_height,10.0f,10000.0f); 
    glEnable(GL_DEPTH_TEST); // We enable the depth test (also called z buffer)
    glPolygonMode (GL_FRONT_AND_BACK, GL_FILL); // Polygon rasterization mode (polygon filled)
    glEnable(GL_TEXTURE_2D); // This Enable the Texture mapping
    Load3DS(&object, "spaceship.3ds");
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
