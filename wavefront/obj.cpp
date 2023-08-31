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

//TODO: werkt niet met DOS line endings bestanden!

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
};

class VertexPoint : std::array<float, 3>
{
public:
    VertexPoint() { }
    VertexPoint(float x, float y, float z) { at(0) = x, at(1) = y, at(2) = z; }
    void x(float x1) { at(0) = x1; }
    void y(float y1) { at(1) = y1; }
    void z(float z1) { at(2) = z1; }
    float x() const { return at(0); }
    float y() const { return at(1); }
    float z() const { return at(2); }
    void draw() const;
};

class NormalsPoint : std::array<float, 3>
{
public:
    NormalsPoint() { }
    NormalsPoint(float x, float y, float z) { at(0) = x, at(1) = y, at(2) = z; }
    void x(float x1) { at(0) = x1; }
    void y(float y1) { at(1) = y1; }
    void z(float z1) { at(2) = z1; }
    float x() const { return at(0); }
    float y() const { return at(1); }
    float z() const { return at(2); }
    void draw() const;
};


class Bitmap
{
public:
    GLuint id;
    std::string name;
    void bind();
};

void Bitmap::bind()
{
    glBindTexture(GL_TEXTURE_2D, id);
}

class Bitmaps
{
private:
    std::vector<Bitmap> _bitmaps;
    GLuint _bitmap(std::istream &is);
public:
    GLuint findBitmapId(std::string name);
    Bitmap get(std::string name);
    bool has(std::string name);
    Bitmap add(std::string name);
};

bool Bitmaps::has(std::string name)
{
    for (std::vector<Bitmap>::const_iterator it = _bitmaps.cbegin(); it != _bitmaps.cend(); ++it)
    {
        if (it->name == name)
            return true;
    }

    return false;
}

void NormalsPoint::draw() const
{
    //std::cout << x() << " " << y() << " " << z() << "\r\n";
    glNormal3f(x(), y(), z());
}

void VertexPoint::draw() const
{
    glVertex3f(x(), y(), z());
}

class TexCoord : public std::array<float, 2>
{
public:
    TexCoord(float u, float v) { at(0) = u, at(1) = v; }
    TexCoord() { }
    void u(float u1) { at(0) = u1; }
    void v(float v1) { at(1) = v1; }
    float u() const { return at(0); }
    float v() const { return at(1); }
    void draw() const;
};

void TexCoord::draw() const
{
    glTexCoord2f(u(), v());
}

class Face
{
public:
    TexCoord tca;
    TexCoord tcb;
    TexCoord tcc;
    NormalsPoint na;
    NormalsPoint nb;
    NormalsPoint nc;
    VertexPoint vpa;
    VertexPoint vpb;
    VertexPoint vpc;
    std::string material;
    void draw() const;
    void calculateNormal();
};

void Face::calculateNormal()
{
    VertexPoint va, vb, vr;
    float val;

    va.x(vpa.x() - vpb.x());
    va.y(vpa.y() - vpb.y());
    va.z(vpa.z() - vpb.z());

    vb.x(vpa.x() - vpc.x());
    vb.y(vpa.y() - vpc.y());
    vb.z(vpa.z() - vpc.z());

    vr.x(va.y() * vb.z() - vb.y() * va.z());
    vr.y(vb.x() * va.z() - va.x() * vb.y());
    vr.z(va.x() * vb.y() - vb.x() * va.y());

    val = sqrt(vr.x() * vr.x() + vr.y() * vr.y() + vr.z() * vr.z());
}

class Material
{
private:
    std::string _name;
    float _ns;
    float _ka[3];
    float _kd[3];
    float _ks[3];
    float _ni;
    float _d;
    int _illum;
    GLuint _texture = 0;
public:
    void name(std::string &name);
    std::string name() const;
    void texture(GLuint t) { _texture = t; }
    GLuint texture() { return _texture; }
};

std::string Material::name() const
{
    return _name;
}

void Material::name(std::string &name)
{
    _name = name;
}

class Materials
{
    Bitmaps _bitmaps;
public:
    std::vector<Material> _materials;
    void readFile(std::istream &is);
    GLuint getTexture(std::string material);
};

GLuint Materials::getTexture(std::string material)
{
    for (std::vector<Material>::iterator it = _materials.begin(); it != _materials.end(); ++it)
    {
        if (it->name() == material)
        {
            return it->texture();
        }
    }

    return 0;
}

class Model_OBJ
{
private:
    //std::vector<float> _normals;
    Materials _materials;
    std::vector<Face> _faces;
public: 
    void load(std::istream &is);
    void load(const char *filename);
	void draw();
};

void Face::draw() const
{
    tca.draw();
    na.draw();
    vpa.draw();
    tcb.draw();
    nb.draw();
    vpb.draw();
    tcc.draw();
    nc.draw();
    vpc.draw();
}

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



template <typename T> T readX(std::istream &is)
{
    T ret;
    is.read(reinterpret_cast<char *>(&ret), sizeof(ret));
    return ret;
}

GLuint Bitmaps::_bitmap(std::istream &is)
{
    is.ignore(18);
    int width = readX<int>(is);
    int height = readX<int>(is);
    is.ignore(112);
    char *data = new char[width * height * 3];
    is.read(data, width * height * 3);

    for (int i = 0; i < width * height * 3; i += 3)
        std::swap(data[i], data[i + 2]);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
    delete[] data;
    return texture;
}

Bitmap Bitmaps::add(std::string name)
{
    std::cerr << name << "\r\n";
    std::ifstream ifs(name);
    GLuint id = _bitmap(ifs);
    ifs.close();
    Bitmap bitmap;
    bitmap.id = id;
    bitmap.name = name;
    _bitmaps.push_back(bitmap);
    return bitmap;
}

Bitmap Bitmaps::get(std::string name)
{
    for (std::vector<Bitmap>::const_iterator it = _bitmaps.cbegin(); it != _bitmaps.cend(); ++it)
    {
        if (it->name == name)
            return *it;
    }

    return Bitmap();
}

GLuint Bitmaps::findBitmapId(std::string name)
{
    for (std::vector<Bitmap>::const_iterator it = _bitmaps.cbegin(); it != _bitmaps.cend(); ++it)
    {
        if (it->name == name)
            return it->id;
    }

    return 0;
}

//read mtl file
void Materials::readFile(std::istream &is)
{
    while (!is.eof())
    {
        std::string line;
        getline(is, line);
        std::vector<std::string> tokens = split(line, " ");

        if (tokens[0].compare("newmtl") == 0)
        {
            _materials.push_back(Material());
            _materials.back().name(tokens[1]);
        }

        if (tokens[0].compare("map_Kd") == 0)
        {
            if (_bitmaps.has(tokens[1]) == false)
                _bitmaps.add(tokens[1]);

            _materials.back().texture(_bitmaps.get(tokens[1]).id);
        }
    }
}

void Model_OBJ::load(std::istream &objFile)
{
    std::vector<VertexPoint> vertexBuf;
    std::vector<float> normalsBuf;
    std::vector<TexCoord> texCoordBuf;
    std::string currentMaterial;
 
    while (!objFile.eof())
    {
        using std::stof;
        using std::stoi;
        std::string line;
        getline(objFile, line);
        std::vector<std::string> tokens = split(line, " ");

        if (tokens[0].compare("mtllib") == 0)
        {
            std::ifstream ifs(tokens[1]);
            _materials.readFile(ifs);
            ifs.close();
        }

        if (tokens[0].compare("usemtl") == 0)
            currentMaterial = tokens[1];
 
        if (tokens[0].compare("v") == 0)
            vertexBuf.push_back(VertexPoint(stof(tokens[1]), stof(tokens[2]), stof(tokens[3])));

        if (tokens[0].compare("vn") == 0)
        {
            normalsBuf.push_back(stof(tokens[1]));
            normalsBuf.push_back(stof(tokens[2]));
            normalsBuf.push_back(stof(tokens[3]));
        }

        if (tokens[0].compare("vt") == 0)
            texCoordBuf.push_back(TexCoord(stof(tokens[1]), stof(tokens[2])));

        if (tokens[0].compare("f") == 0)
        {
            Face face;
            face.material = currentMaterial;

            std::vector<std::string> vs[3];
            vs[0] = split(tokens[1], "/");
            vs[1] = split(tokens[2], "/");
            vs[2] = split(tokens[3], "/");
            std::cerr << vs[0][0] << " " << vs[0][1] << " " << vs[0][2] << "\r\n";
            std::cerr.flush();

            if (vs[0][0].size() > 0)
            {
                int vertexNumber[3];
                vertexNumber[0] = stoi(vs[0][0]) - 1;
                vertexNumber[1] = stoi(vs[1][0]) - 1;
                vertexNumber[2] = stoi(vs[2][0]) - 1;
                face.vpa = vertexBuf[vertexNumber[0]];
                face.vpb = vertexBuf[vertexNumber[1]];
                face.vpc = vertexBuf[vertexNumber[2]];
            }

            if (vs[0][1].size() > 0)
            {
                int texturesNumber[3];
                texturesNumber[0] = stoi(vs[0][1]) - 1;
                texturesNumber[1] = stoi(vs[1][1]) - 1;
                texturesNumber[2] = stoi(vs[2][1]) - 1;
                face.tca = texCoordBuf[texturesNumber[0]];
                face.tcb = texCoordBuf[texturesNumber[1]];
                face.tcc = texCoordBuf[texturesNumber[2]];
            }

            //obj bestand heeft normals
            if (vs[0][2].size() > 0)
            {
                int normalsNumber[3];
                normalsNumber[0] = stoi(vs[0][2]) - 1;
                normalsNumber[1] = stoi(vs[1][2]) - 1;
                normalsNumber[2] = stoi(vs[2][2]) - 1;
                NormalsPoint vp;
                vp.x(normalsBuf[3 * normalsNumber[0] + 0]);
                vp.y(normalsBuf[3 * normalsNumber[0] + 1]);
                vp.z(normalsBuf[3 * normalsNumber[0] + 2]);
                face.na = vp;
                vp.x(normalsBuf[3 * normalsNumber[1] + 0]);
                vp.y(normalsBuf[3 * normalsNumber[1] + 1]);
                vp.z(normalsBuf[3 * normalsNumber[1] + 2]);
                face.nb = vp;
                vp.x(normalsBuf[3 * normalsNumber[2] + 0]);
                vp.y(normalsBuf[3 * normalsNumber[2] + 1]);
                vp.z(normalsBuf[3 * normalsNumber[2] + 2]);
                face.nc = vp;
            }
            else
            {
                float coord[3][3];
                float va[3], vb[3], vr[3], val;
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
#if 0
			    for (int i = 0; i < 3; i++)
                    _normals.insert(_normals.end(), std::begin(norm), std::end(norm));
#endif
            }

            _faces.push_back(face);
		}	
	}
}
 
void Model_OBJ::draw()
{
    std::string currentMaterial = "";
    glBegin(GL_TRIANGLES);

    for (std::vector<Face>::iterator it = _faces.begin(); it != _faces.end(); ++it)
    {
        if (it->material != currentMaterial)
        {
            currentMaterial = it->material;
            GLuint texture = _materials.getTexture(currentMaterial);

            if (texture > 0)
            {
                glEnd();
                glDisable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, texture);
                glEnable(GL_TEXTURE_2D);
                glBegin(GL_TRIANGLES);
            }
        }
        it->draw();
    }
    
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glFlush();
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
float scale = 1;
float panY = -10;

void CMain::display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    gluLookAt(0,1,40, 0,0,0, 0,1,0);

    glScalef(scale, scale, scale);

    //pan naar beneden
    glTranslatef(0, panY, 0);

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
    switch (key)
    {
    case 'a':
        scale /= 2;
        break;
    case 'z':
        scale *= 2;
        break;
    case 'w':
        panY += 5;
        break;
    case 's':
        panY -= 5;
        break;
    case KEY_ESCAPE:        
        exit(0);
        break;    
    default:
        break;
    }
}

void idle()
{
    glRotatef(deg++, 0, 1, 0);
    
    if (deg > 360)
        deg = 0;
}

void CMain::run(int argc, char **argv)
{
    _pthis = this;
    win.width = 640;
    win.height = 480;
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
	gluPerspective(45, aspect, 1, 500);
    glMatrixMode(GL_MODELVIEW);

    glShadeModel(GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

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


