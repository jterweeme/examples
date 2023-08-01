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

template <typename T> class XYZ
{
public:
    T _xyz[3];
    XYZ(T x, T y, T z) { _xyz[0] = x, _xyz[1] = y, _xyz[2] = z; }
    T x() { return _xyz[0]; }
    T y() { return _xyz[1]; }
    T z() { return _xyz[2]; }
};

class Model_OBJ
{
private:
    std::vector<float> _normals;
    std::vector<float> _triangles;
    std::vector<float> _texCoords;
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

//werkt niet met meerdere spaties!
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

GLuint raw_texture_load(const char *filename, int width, int height)
{
    GLuint texture;
    uint8_t *data = new uint8_t[width * height * 3];
    std::ifstream ifs(filename);
    ifs.read((char *)(data), width * height * 3);
    ifs.close();
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_DECAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_DECAL);
#if 0
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB_INTEGER, width, height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, data);
#endif
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
    //gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, width, height, GL_RGB, GL_BITMAP, data);
    delete[] data;
    return texture;
}

using Coords = XYZ<float>;

void Model_OBJ::load(std::istream &objFile)
{
    std::string line;
    std::vector<Coords> vertexBuf;
    std::vector<float> normalsBuf;
    std::vector<float> texturesBuf;
 
    while (!objFile.eof())
    {		
        getline(objFile, line);
        std::vector<std::string> tokens = split(line, " ");
 
        if (tokens[0].compare("v") == 0)
        {
            float x = std::stof(tokens[1]);
            float y = std::stof(tokens[2]);
            float z = std::stof(tokens[3]);
            vertexBuf.push_back(XYZ(x, y, z));
            std::cout << "#" << vertexBuf.size() << " " << x << " " << y << " " << z << "\r\n";
            std::cout.flush();
        }

        if (tokens[0].compare("vn") == 0)
        {
            normalsBuf.push_back(std::stof(tokens[1]));
            normalsBuf.push_back(std::stof(tokens[2]));
            normalsBuf.push_back(std::stof(tokens[3]));
        }

        if (tokens[0].compare("vt") == 0)
        {
            texturesBuf.push_back(std::stof(tokens[1]));
            texturesBuf.push_back(std::stof(tokens[2]));
        }

        // The first character is an 'f': on this line is a point stored
        if (tokens[0].compare("f") == 0)
        {
            int vertexNumber[3];    //xyz
            int normalsNumber[3];   //xyz
            int texturesNumber[3];  //xyz
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
                    float *p = vertexBuf[vertexNumber[i]]._xyz;
                    _triangles.insert(_triangles.end(), p, p + 3);
                    std::copy(p, p + 3, coord[i]);
                }
            }

            if (vs[0].size() > 1)
            {
                texturesNumber[0] = std::stoi(vs[0][1]) - 1;
                texturesNumber[1] = std::stoi(vs[1][1]) - 1;
                texturesNumber[2] = std::stoi(vs[2][1]) - 1;

                for (int i = 0; i < 3; ++i)
                {
                    _texCoords.push_back(texturesBuf[2 * texturesNumber[i] + 0]);
                    _texCoords.push_back(texturesBuf[2 * texturesNumber[i] + 1]);
                }
            }

            //obj bestand heeft normals
            if (vs[0].size() > 2)
            {
                normalsNumber[0] = std::stoi(vs[0][2]) - 1;
                normalsNumber[1] = std::stoi(vs[1][2]) - 1;
                normalsNumber[2] = std::stoi(vs[2][2]) - 1;

                for (int i = 0; i < 3; ++i)
                {
                    _normals.push_back(normalsBuf[3 * normalsNumber[i] + 0]);
                    _normals.push_back(normalsBuf[3 * normalsNumber[i] + 1]);
                    _normals.push_back(normalsBuf[3 * normalsNumber[i] + 2]);
                }
            }
            else
            {
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
	}
}
 
void Model_OBJ::draw()
{
    glEnableClientState(GL_VERTEX_ARRAY);						// Enable vertex arrays
    glEnableClientState(GL_NORMAL_ARRAY);						// Enable normal arrays
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnable(GL_TEXTURE_2D);
    glVertexPointer(3, GL_FLOAT, 0, _triangles.data());         // Vertex Pointer to triangle array
    glNormalPointer(GL_FLOAT, 0, _normals.data());
    glTexCoordPointer(2, GL_FLOAT, 0, _texCoords.data());
    glDrawArrays(GL_TRIANGLES, 0, _triangles.size() / 3);       // Draw the trianglesa
    glDisable(GL_TEXTURE_2D);
    glDisableClientState(GL_VERTEX_ARRAY);						// Disable vertex arrays
    glDisableClientState(GL_NORMAL_ARRAY);						// Disable normal arrays
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
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
    gluLookAt(0,1,5, 0,0,0, 0,1,0);
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

    raw_texture_load("etcube.raw", 512, 1560);
    obj.load(argv[1]);
    std::cout << "Finished loading model\r\n";
    std::cout.flush();
    glutMainLoop();   
}

int main(int argc, char **argv) 
{
    CMain cmain;
    cmain.run(argc, argv);
	return 0;
}


