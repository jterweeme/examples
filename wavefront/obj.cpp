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
    long TotalConnectedTriangles;
    std::array<float, 3> _calcNormal(float *coord1, float *coord2, float *coord3);
public: 
	Model_OBJ();			
    int Load(const char *filename);	// Loads the model
	void draw();					// Draws the model on the screen
};
 
 
#define POINTS_PER_VERTEX 3
#define TOTAL_FLOATS_IN_TRIANGLE 9
 
Model_OBJ::Model_OBJ()
{
    this->TotalConnectedTriangles = 0; 
}

std::array<float, 3> Model_OBJ::_calcNormal(float *coord1, float *coord2, float *coord3)
{
    /* calculate Vector1 and Vector2 */
    float va[3], vb[3], vr[3], val;
    va[0] = coord1[0] - coord2[0];
    va[1] = coord1[1] - coord2[1];
    va[2] = coord1[2] - coord2[2];
 
    vb[0] = coord1[0] - coord3[0];
    vb[1] = coord1[1] - coord3[1];
    vb[2] = coord1[2] - coord3[2];
 
    /* cross product */
    vr[0] = va[1] * vb[2] - vb[1] * va[2];
    vr[1] = vb[0] * va[2] - va[0] * vb[2];
    vr[2] = va[0] * vb[1] - vb[0] * va[1];
 
    /* normalization factor */
    val = sqrt( vr[0]*vr[0] + vr[1]*vr[1] + vr[2]*vr[2] );
 
    std::array<float, 3> norm;
	norm[0] = vr[0]/val;
	norm[1] = vr[1]/val;
	norm[2] = vr[2]/val;
 
	return norm;
}

int Model_OBJ::Load(const char* filename)
{
    std::string line;
    std::ifstream objFile(filename);

    if (!objFile.is_open())
    {
        std::cerr << "Cannot open file\r\n";
        std::cerr.flush();
        return 0;
    }

    int triangle_index = 0;					// Set triangle index to zero
    int normal_index = 0;					// Set normal index to zero
    long TotalConnectedPoints = 0;
    std::vector<float> vertexBuf;
 
    while (! objFile.eof() )						// Start reading file data
    {		
        getline (objFile,line);							// Get line from file
 
        // The first character is a v: on this line is a vertex stored.
        if (line.c_str()[0] == 'v')	
        {
            line[0] = ' ';		// Set first character to 0. This will allow us to use sscanf
            float buf[3];

            // Read floats from the line: v X Y Z
            sscanf(line.c_str(),"%f %f %f ", &buf[0], &buf[1], &buf[2]);	

            vertexBuf.push_back(buf[0]);
            vertexBuf.push_back(buf[1]);
            vertexBuf.push_back(buf[2]);
 
            TotalConnectedPoints += POINTS_PER_VERTEX;	// Add 3 to the total connected points
        }

        // The first character is an 'f': on this line is a point stored
        if (line.c_str()[0] == 'f')		
        {
		    line[0] = ' ';		// Set first character to 0. This will allow us to use sscanf
 
			int vertexNumber[4] = { 0, 0, 0 };
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
 
			int tCounter = 0;

			for (int i = 0; i < POINTS_PER_VERTEX; i++)					
			{
                _triangles.push_back(vertexBuf[3 * vertexNumber[i] + 0 ]);
                _triangles.push_back(vertexBuf[3 * vertexNumber[i] + 1 ]);
                _triangles.push_back(vertexBuf[3 * vertexNumber[i] + 2 ]);
                tCounter += POINTS_PER_VERTEX;
			}
 
			/*********************************************************************
				 * Calculate all normals, used for lighting
				 */
            float coord1[3];
            coord1[0] = _triangles[triangle_index + 0];
            coord1[1] = _triangles[triangle_index + 1];
            coord1[2] = _triangles[triangle_index + 2];

            float coord2[3];
            coord2[0] = _triangles[triangle_index + 3];
            coord2[1] = _triangles[triangle_index + 4];
            coord2[2] = _triangles[triangle_index + 5];

            float coord3[3];
            coord3[0] = _triangles[triangle_index + 6];
            coord3[1] = _triangles[triangle_index + 7];
            coord3[2] = _triangles[triangle_index + 8];

            std::array<float, 3> norm = _calcNormal(coord1, coord2, coord3);
 
			for (int i = 0; i < POINTS_PER_VERTEX; i++)
			{
                _normals.push_back(norm[0]);
                _normals.push_back(norm[1]);
                _normals.push_back(norm[2]);
			}

			triangle_index += TOTAL_FLOATS_IN_TRIANGLE;
			normal_index += TOTAL_FLOATS_IN_TRIANGLE;
			TotalConnectedTriangles += TOTAL_FLOATS_IN_TRIANGLE;			
		}	
	}
	objFile.close();
    _triangles.resize(_triangles.size() + 999999);
	return 0;
}
 
void Model_OBJ::draw()
{
    glEnableClientState(GL_VERTEX_ARRAY);						// Enable vertex arrays
    glEnableClientState(GL_NORMAL_ARRAY);						// Enable normal arrays
    glVertexPointer(3,GL_FLOAT,	0, _triangles.data());		    // Vertex Pointer to triangle array
    glNormalPointer(GL_FLOAT, 0, _normals.data());
    glDrawArrays(GL_TRIANGLES, 0, TotalConnectedTriangles);		// Draw the triangles
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

void CMain::display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	gluLookAt( 0,1,40, 0,0,0, 0,1,0);
	glPushMatrix();
	glRotatef(45,0,1,0);
	glRotatef(90,0,1,0);
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

    obj.Load("cessna.obj");
    glutMainLoop();   
}

int main(int argc, char **argv) 
{
    CMain cmain;
    cmain.run(argc, argv);
	return 0;
}


