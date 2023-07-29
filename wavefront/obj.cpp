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

struct XYZ
{
    float x;
    float y;
    float z;

    XYZ(float x1, float y1, float z1) : x(x1), y(y1), z(z1) { }
};

class Model_OBJ
{
private:
    std::vector<float> _normals;
    std::vector<float> _triangles;
public: 
    void load(std::istream &is);
    void load(const char *filename);
	void draw();
};
 
void Model_OBJ::load(const char* filename)
{
    std::ifstream objFile(filename);

    if (!objFile.is_open())
        throw "Cannot open file";

    load(objFile);
    objFile.close();
}

static std::vector<std::string> split(std::string line, std::string delim)
{
    std::vector<std::string> tokens;

    std::string::size_type pos = 0;
    std::string::size_type prev = 0;

    while ((pos = line.find(delim, prev)) != std::string::npos)
    {
        tokens.push_back(line.substr(prev, pos - prev));
        prev = pos + delim.size();
    }

    // To get the last substring (or only, if delimiter is not found)
    tokens.push_back(line.substr(prev));
    return tokens;
}

void Model_OBJ::load(std::istream &objFile)
{
    std::string line;
    std::vector<float> vertexBuf;
    std::vector<float> normalsBuf;
    std::vector<float> texturesBuf;
 
    while (!objFile.eof())
    {		
        getline(objFile, line);
        std::vector<std::string> tokens = split(line, " ");
 
        if (tokens[0].compare("v") == 0)
        {
            vertexBuf.push_back(std::stof(tokens[1]));
            vertexBuf.push_back(std::stof(tokens[2]));
            vertexBuf.push_back(std::stof(tokens[3]));
        }

        if (tokens[0].compare("vn") == 0)
        {
            normalsBuf.push_back(std::stof(tokens[1]));
            normalsBuf.push_back(std::stof(tokens[2]));
            normalsBuf.push_back(std::stof(tokens[3]));
        }

        // The first character is an 'f': on this line is a point stored
        if (tokens[0].compare("f") == 0)
        {
            int vertexNumber[3];
            int normalsNumber[3];
            int texturesNumber[3];
            float coord[3][3];
            float va[3], vb[3], vr[3], val;
            std::vector<std::string> vs[3];
            vs[0] = split(tokens[1], "/");
            vs[1] = split(tokens[2], "/");
            vs[2] = split(tokens[3], "/");

            if (vs[0].size() > 0)
            {
                vertexNumber[0] = std::stoi(vs[0][0]) - 1;
                vertexNumber[1] = std::stoi(vs[1][0]) - 1;
                vertexNumber[2] = std::stoi(vs[2][0]) - 1;

                for (int i = 0; i < 3; ++i)
                {
                    _triangles.push_back(vertexBuf[3 * vertexNumber[i] + 0 ]);
                    coord[i][0] = _triangles.back();
                    _triangles.push_back(vertexBuf[3 * vertexNumber[i] + 1 ]);
                    coord[i][1] = _triangles.back();
                    _triangles.push_back(vertexBuf[3 * vertexNumber[i] + 2 ]);
                    coord[i][2] = _triangles.back();
                }
            }

            if (vs[0].size() > 1)
            {
                normalsNumber[0] = std::stoi(vs[0][1]) - 1;
                normalsNumber[1] = std::stoi(vs[1][1]) - 1;
                normalsNumber[2] = std::stoi(vs[2][1]) - 1;

                for (int i = 0; i < 3; ++i)
                {
                    _normals.push_back(normalsBuf[3 * normalsNumber[i] + 0]);
                    _normals.push_back(normalsBuf[3 * normalsNumber[i] + 1]);
                    _normals.push_back(normalsBuf[3 * normalsNumber[i] + 2]);
                }
            }
#if 0
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
#endif
		}	
	}
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

    obj.load(argv[1]);
    glutMainLoop();   
}

int main(int argc, char **argv) 
{
    CMain cmain;
    cmain.run(argc, argv);
	return 0;
}


