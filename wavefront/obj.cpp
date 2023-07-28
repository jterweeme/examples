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

class Model_OBJ
{
private:
    std::vector<float> _normals;
    std::vector<float> _triangles;
public: 
    int load(const char *filename);
	void draw();
};
 
int Model_OBJ::load(const char* filename)
{
    std::string line;
    std::ifstream objFile(filename);

    if (!objFile.is_open())
    {
        std::cerr << "Cannot open file\r\n";
        std::cerr.flush();
        return -1;
    }

    std::vector<float> vertexBuf;
 
    while (!objFile.eof())
    {		
        getline (objFile,line);
 
        // The first character is a v: on this line is a vertex stored.
        if (line.c_str()[0] == 'v')	
        {
            line[0] = ' ';		// Set first character to 0. This will allow us to use sscanf
            float buf[3];

            // Read floats from the line: v X Y Z
            sscanf(line.c_str(),"%f %f %f ", &buf[0], &buf[1], &buf[2]);
            vertexBuf.insert(vertexBuf.end(), std::begin(buf), std::end(buf));
        }

        // The first character is an 'f': on this line is a point stored
        if (line.c_str()[0] == 'f')		
        {
		    line[0] = ' ';		// Set first character to 0. This will allow us to use sscanf
 
			int vertexNumber[3] = { 0, 0, 0 };
            sscanf(line.c_str(),"%i%i%i",		// Read integers from the line:  f 1 2 3
					&vertexNumber[0],   // First point of our triangle. This is an 
					&vertexNumber[1],   // pointer to our vertexBuffer list
					&vertexNumber[2] ); // each point represents an X,Y,Z.
 
			vertexNumber[0] -= 1;   // OBJ file starts counting from 1
			vertexNumber[1] -= 1;   // OBJ file starts counting from 1
			vertexNumber[2] -= 1;   // OBJ file starts counting from 1
 
 
			/********************************************************************
             * Create triangles (f 1 2 3) from points: (v X Y Z) (v X Y Z) (v X Y Z). 
             * The vertexBuffer contains all verteces
             * The triangles will be created using the verteces we read previously
             */

            float coord[3][3];
            float va[3], vb[3], vr[3], val;

            for (int i = 0; i < 3; ++i)
            {
                _triangles.push_back(vertexBuf[3 * vertexNumber[i] + 0 ]);
                coord[i][0] = _triangles.back();
                _triangles.push_back(vertexBuf[3 * vertexNumber[i] + 1 ]);
                coord[i][1] = _triangles.back();
                _triangles.push_back(vertexBuf[3 * vertexNumber[i] + 2 ]);
                coord[i][2] = _triangles.back();
            }

            va[0] = coord[0][0] - coord[1][0];
            va[1] = coord[0][1] - coord[1][1];
            va[2] = coord[0][2] - coord[1][2];
 
            vb[0] = coord[0][0] - coord[2][0];
            vb[1] = coord[0][1] - coord[2][1];
            vb[2] = coord[0][2] - coord[2][2];
 
            /* cross product */
            vr[0] = va[1] * vb[2] - vb[1] * va[2];
            vr[1] = vb[0] * va[2] - va[0] * vb[2];
            vr[2] = va[0] * vb[1] - vb[0] * va[1];
 
            /* normalization factor */
            val = sqrt( vr[0]*vr[0] + vr[1]*vr[1] + vr[2]*vr[2] );
 
            float norm[3];
        	norm[0] = vr[0]/val;
        	norm[1] = vr[1]/val;
        	norm[2] = vr[2]/val;
 
			for (int i = 0; i < 3; i++)
                _normals.insert(_normals.end(), std::begin(norm), std::end(norm));
		}	
	}

	objFile.close();
	return 0;
}
 
void Model_OBJ::draw()
{
    glEnableClientState(GL_VERTEX_ARRAY);						// Enable vertex arrays
    glEnableClientState(GL_NORMAL_ARRAY);						// Enable normal arrays
    glVertexPointer(3,GL_FLOAT,	0, _triangles.data());		    // Vertex Pointer to triangle array
    glNormalPointer(GL_FLOAT, 0, _normals.data());
    glDrawArrays(GL_TRIANGLES, 0, _triangles.size() / 3);       // Draw the triangles
    glDisableClientState(GL_VERTEX_ARRAY);						// Disable vertex arrays
    glDisableClientState(GL_NORMAL_ARRAY);						// Disable normal arrays
}
 
/***************************************************************************
 * Program code
 ***************************************************************************/
 


class CMain
{
private:
    static CMain *_pthis;
    Model_OBJ obj;
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
    glRotatef(deg++, 0, 1, 0);
    
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

    obj.load("cessna.obj");
    glutMainLoop();   
}

int main(int argc, char **argv) 
{
    CMain cmain;
    cmain.run(argc, argv);
	return 0;
}


