#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <GL/glut.h>
#include <iostream>

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
struct obj_type
{
    char name[20];

    int vertices_qty;
    int polygons_qty;

    vertex_type vertex[MAX_VERTICES];
    polygon_type polygon[MAX_POLYGONS];
    mapcoord_type mapcoord[MAX_VERTICES];
    int id_texture;
};

long filelength(int f)
{
    struct stat buf;

    fstat(f, &buf);

    return(buf.st_size);
}

static uint16_t readW(FILE *fp)
{
    uint16_t ret;
    fread(&ret, sizeof (uint16_t), 1, fp);
    return ret;
}

static uint32_t readDW(FILE *fp)
{
    uint32_t ret;
    fread(&ret, sizeof(uint32_t), 1, fp);
    return ret;
}

static float readFloat(FILE *fp)
{
    float ret;
    fread(&ret, sizeof(float), 1, fp);
    return ret;
}

static char nibble(uint8_t n)
{   
    return n <= 9 ? '0' + char(n) : 'A' + char (n - 10);
}

static std::string hex8(uint8_t b)
{
    std::string ret;
    ret += nibble(b >> 4 & 0xf);
    ret += nibble(b >> 0 & 0xf);
    return ret;
}

static std::string hex16(uint16_t w)
{
    std::string ret;
    ret += hex8(w >> 8 & 0xff);
    ret += hex8(w >> 0 & 0xff);
    return ret;
}

static std::string hex32(uint32_t dw)
{
    std::string ret;
    ret += hex16(dw >> 16 & 0xffff);
    ret += hex16(dw >>  0 & 0xffff);
    return ret;
}

void Load3DS(obj_type *p_object, const char *p_filename)
{
    FILE *l_file; //File pointer

    if ((l_file=fopen (p_filename, "rb"))== NULL)
        throw "cannot open file";

    while (ftell (l_file) < filelength (fileno (l_file))) //Loop to scan the whole file
    {
        uint16_t l_chunk_id = readW(l_file);
        uint32_t l_chunk_lenght = readDW(l_file);

        if (l_chunk_id == 0x4d4d)
        {
        }
        else if (l_chunk_id == 0x3d3d)
        {
        }
        else if (l_chunk_id == 0x4000)
        {
            uint8_t l_char;
            int i = 0;
            
            do
            {
                fread(&l_char, 1, 1, l_file);
                p_object->name[i] = l_char;
                ++i;
            }
            while (l_char != '\0' && i < 20);
        }
        else if (l_chunk_id == 0x4100)
        {
        }
        else if (l_chunk_id == 0x4110)
        {
            uint16_t l_qty = readW(l_file);
            p_object->vertices_qty = l_qty;

            for (int i=0; i<l_qty; i++)
            {
                p_object->vertex[i].x = readFloat(l_file);
                p_object->vertex[i].y = readFloat(l_file);
                p_object->vertex[i].z = readFloat(l_file);
            }
        }
        else if (l_chunk_id == 0x4120)
        {
            uint16_t l_qty = readW(l_file);
            p_object->polygons_qty = l_qty;

            for (int i=0; i<l_qty; ++i)
            {
                p_object->polygon[i].a = readW(l_file);
                p_object->polygon[i].b = readW(l_file);
                p_object->polygon[i].c = readW(l_file);
                uint16_t l_face_flags = readW(l_file);
            }
        }
        else if (l_chunk_id == 0x4140)
        {
            std::cerr << hex16(l_chunk_id) << "\r\n";

            uint16_t l_qty = readW(l_file);

            for (int i = 0; i < l_qty; ++i)
            {
                p_object->mapcoord[i].u = readFloat(l_file);
                p_object->mapcoord[i].v = readFloat(l_file);
            }
        }
        else
        {
            std::cerr << hex16(l_chunk_id) << "\r\n";
            fseek(l_file, l_chunk_lenght-6, SEEK_CUR);       
        }
    }
    fclose (l_file); // Closes the file stream
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

    glBegin(GL_TRIANGLES);
    for (int l_index=0;l_index<object.polygons_qty;l_index++)
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
    glutMainLoop();

    return(0);    
}
