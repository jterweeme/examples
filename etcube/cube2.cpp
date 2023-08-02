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

int deg = 0;

struct glutWindow
{
    int width;
    int height;
    float field_of_view_angle;
    float z_near;
    float z_far;
};

class Vertex
{
private:
    float _x, _y, _z;
public:
    Vertex() { }
    Vertex(float x1, float y1, float z1) : _x(x1), _y(y1), _z(z1) { }
    float x() { return _x; }
    float y() { return _y; }
    float z() { return _z; }
};

class TexCoord
{
public:
    float u, v;
    TexCoord() { }
    TexCoord(float u1, float v1) : u(u1), v(v1) { }
};

class VP
{
public:
    Vertex *v;
    TexCoord *t;
};

using Triangle = std::array<VP, 3>;

class Model_OBJ
{
private:
    std::vector<Vertex> _vertices;
    std::vector<TexCoord> _texCoords;
    std::vector<Triangle> _triangles2;
    void _printVertex(Vertex *v);
    void _texCoord(TexCoord *t);
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
    glBindTexture(GL_TEXTURE_2D, texture);
    gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
    delete[] data;
    return texture;
}

void Model_OBJ::_printVertex(Vertex *v)
{
    glVertex3f(v->x(), v->y(), v->z());
}

void Model_OBJ::_texCoord(TexCoord *t)
{
    glTexCoord2f(t->u, t->v);
}

void Model_OBJ::load(std::istream &objFile)
{
    while (!objFile.eof())
    {
        std::string line;
        using std::stof;
        using std::stoi;
        getline(objFile, line);
        std::vector<std::string> tokens = split(line, " ");
 
        if (tokens[0].compare("v") == 0)
            _vertices.push_back(Vertex(stof(tokens[1]), stof(tokens[2]), stof(tokens[3])));

        if (tokens[0].compare("vt") == 0)
            _texCoords.push_back(TexCoord(stof(tokens[1]), stof(tokens[2])));

        // The first character is an 'f': on this line is a point stored
        if (tokens[0].compare("f") == 0)
        {
            Triangle f;

            std::vector<std::string> vs[3];
            vs[0] = split(tokens[1], "/");
            vs[1] = split(tokens[2], "/");
            vs[2] = split(tokens[3], "/");

            if (vs[0].size() > 0)
            {
                int vertexNumber;
                vertexNumber = stoi(vs[0][0]) - 1;
                f[0].v = &_vertices[vertexNumber];
                vertexNumber = stoi(vs[1][0]) - 1;
                f[1].v = &_vertices[vertexNumber];
                vertexNumber = stoi(vs[2][0]) - 1;
                f[2].v = &_vertices[vertexNumber];
            }

            if (vs[0].size() > 1)
            {
                int texturesNumber;
                texturesNumber = std::stoi(vs[0][1]) - 1;
                f[0].t = &_texCoords[texturesNumber];
                texturesNumber = std::stoi(vs[1][1]) - 1;
                f[1].t = &_texCoords[texturesNumber];
                texturesNumber = std::stoi(vs[2][1]) - 1;
                f[2].t = &_texCoords[texturesNumber];
            }
            _triangles2.push_back(f);
		}	
	}
}
 
void Model_OBJ::draw()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0,1,5, 0,0,0, 0,1,0);
    glPushMatrix();
    glRotatef(deg++, 0, 1, 0);
    
    if (deg > 360)
        deg = 0;
#if 1
    glBegin(GL_TRIANGLES);

    for (int i = 0; i < _triangles2.size(); ++i)
    {
        _texCoord(_triangles2[i][0].t);
        _printVertex(_triangles2[i][0].v);
        _texCoord(_triangles2[i][1].t);
        _printVertex(_triangles2[i][1].v);
        _texCoord(_triangles2[i][2].t);
        _printVertex(_triangles2[i][2].v);
    }

    glEnd();
#endif
    glFlush();
    glutSwapBuffers();
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



void CMain::display()
{
    _pthis->obj.draw();
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
    glutIdleFunc(display);									// register Idle Function
    glutKeyboardFunc(keyboard);								// register Keyboard Handler

    glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, win.width, win.height);
	GLfloat aspect = (GLfloat) win.width / win.height;
    glLoadIdentity();
	gluPerspective(win.field_of_view_angle, aspect, win.z_near, win.z_far);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

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


