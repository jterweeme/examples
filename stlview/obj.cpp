/*
 *
 * Demonstrates how to load and display an Wavefront OBJ file. 
 * Using triangles and normals as static object. No texture mapping.
 * https://tutorialsplay.com/opengl/
 *
 * OBJ files must be triangulated!!!
 * Non triangulated objects wont work!
 * You can use Blender to triangulate
 *
 * g++ obj.cpp -lglut -lGL -lGLU
 *
 */
 
#include <iostream>
#include <fstream>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <array>
 
#define KEY_ESCAPE 27
 
struct glutWindow
{
    int width;
    int height;
    float field_of_view_angle;
    float z_near;
    float z_far;
};

class Model
{
private:
    std::vector<float> _normals;
    std::vector<float> _triangles;
public: 
    void load(std::istream &is);
	void draw();
};

template <typename T> T readX(std::istream &is)
{
    T ret;
    is.read(reinterpret_cast<char *>(&ret), sizeof(ret));
    return ret;
}

void Model::load(std::istream &is)
{
    is.ignore(80);
    uint32_t n_triangles = readX<uint32_t>(is);
    
    for (uint32_t i = 0; i < n_triangles; ++i)
    {
        float normals[3];
        
        for (uint32_t j = 0; j < 3; ++j)
            normals[j] = read<float>(is);

        for (uint32_t j = 0; j < 3; ++j)
            _normals.insert(_normals.end(), std::begin(normals), std::end(normals));           

        for (uint32_t j = 0; j < 9; ++j)
            _triangles.push_back(read<float>(is));

        is.ignore(2);
    }
}
 
void Model::draw()
{
    glEnableClientState(GL_VERTEX_ARRAY);						// Enable vertex arrays
    glEnableClientState(GL_NORMAL_ARRAY);						// Enable normal arrays
    glVertexPointer(3,GL_FLOAT,	0, _triangles.data());		    // Vertex Pointer to triangle array
    glNormalPointer(GL_FLOAT, 0, _normals.data());
    glDrawArrays(GL_TRIANGLES, 0, _triangles.size() / 3);       // Draw the triangles
    glDisableClientState(GL_VERTEX_ARRAY);						// Disable vertex arrays
    glDisableClientState(GL_NORMAL_ARRAY);						// Disable normal arrays
}

class CMain
{
private:
    static CMain *_pthis;
    Model obj;
    glutWindow win;
    static void display();
    static void keyboard(unsigned char key, int x, int y);
public:
    void run(int argc, char **argv);
};

CMain *CMain::_pthis;

int deg = 0;

void CMain::display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	gluLookAt( 0,1,40, 0,0,0, 0,1,0);
	glPushMatrix();
    glRotatef(deg++, 1, 1, 1);
    
    if (deg > 360)
        deg = 0;

    //glRotatef(45,0,1,0);
    //glRotatef(90,0,1,0);
    _pthis->obj.draw();
	glPopMatrix();
	glutSwapBuffers();
}

void CMain::keyboard(unsigned char key, int x, int y)
{
    switch ( key )
    {
    case KEY_ESCAPE:        
      exit ( 0 );   
      break;      
    default:      
      break;
    }
}

void CMain::run(int argc, char **argv)
{
    _pthis = this;
    win.width = 640;
    win.height = 480;
    win.field_of_view_angle = 45;
    win.z_near = 1.0f;
	win.z_far = 500.0f;
    glutInit(&argc, argv);                                      // GLUT initialization
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );  // Display Mode
    glutInitWindowSize(win.width, win.height);					// set window size
    glutCreateWindow("OpenGL/GLUT OBJ Loader.");
    glutDisplayFunc(display);									// register Display Function
    glutIdleFunc( display );									// register Idle Function
    glutKeyboardFunc(keyboard);								// register Keyboard Handler

    glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, win.width, win.height);
	GLfloat aspect = (GLfloat) win.width / win.height;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluPerspective(win.field_of_view_angle, aspect, win.z_near, win.z_far);
    glMatrixMode(GL_MODELVIEW);
    glShadeModel( GL_SMOOTH );
    glClearColor( 0.0f, 0.1f, 0.0f, 0.5f );
    glClearDepth( 1.0f );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );
    glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
 
    GLfloat amb_light[] = { 0.1, 0.1, 0.1, 1.0 };
    GLfloat diffuse[] = { 0.6, 0.6, 0.6, 1 };
    GLfloat specular[] = { 0.7, 0.7, 0.3, 1 };
    glLightModelfv( GL_LIGHT_MODEL_AMBIENT, amb_light );
    glLightfv( GL_LIGHT0, GL_DIFFUSE, diffuse );
    glLightfv( GL_LIGHT0, GL_SPECULAR, specular );
    glEnable( GL_LIGHT0 );
    glEnable( GL_COLOR_MATERIAL );
    glShadeModel( GL_SMOOTH );
    glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE );
    glDepthFunc( GL_LEQUAL );
    glEnable( GL_DEPTH_TEST );
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    std::ifstream ifs("utah_teapot_solid.stl");
    obj.load(ifs);
    ifs.close();
    
    glutMainLoop();   
}

int main(int argc, char **argv) 
{
    CMain cmain;
    cmain.run(argc, argv);
	return 0;
}


