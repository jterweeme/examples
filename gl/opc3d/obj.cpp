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
#include <thread>
#include <opc/ua/client/client.h>
#include <opc/ua/node.h>
#include <opc/ua/subscription.h>
#include <opc/common/logger.h>

 
#define KEY_ESCAPE 27

using namespace OpcUa;

class SubClient : public SubscriptionHandler
{
  void DataChange(uint32_t handle, const Node & node, const Variant & val, AttributeId attr) override
  {
    std::cout << "Received DataChange event, value of Node " << node << " is now: "  << val.ToString() << std::endl;
  }
};


struct glutWindow
{
    int width;
    int height;
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

using Coords = XYZ<float>;

template <typename T> T readX(std::istream &is)
{
    T ret;
    is.read(reinterpret_cast<char *>(&ret), sizeof(ret));
    return ret;
}

void bitmap(std::istream &is)
{
    std::cerr << "Bitmap\r\n";
    std::cerr.flush();
    is.ignore(18);
    int width = readX<int>(is);
    int height = readX<int>(is);
    std::cerr << width << "x" << height << "\r\n";
    std::cerr.flush();
    is.ignore(112);
    char *data = new char[width * height * 3];
    is.read(data, width * height * 3);
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
    delete[] data;
}

void material(std::istream &is)
{
    while (!is.eof())
    {
        std::string line;
        getline(is, line);
        std::vector<std::string> tokens = split(line, " ");

        if (tokens[0].compare("map_Kd") == 0)
        {
            std::ifstream ifs(tokens[1]);
            bitmap(ifs);
            ifs.close();
        }
    }
}

void Model_OBJ::load(std::istream &objFile)
{
    std::vector<Coords> vertexBuf;
    std::vector<float> normalsBuf;
    std::vector<float> texturesBuf;
 
    while (!objFile.eof())
    {
        using std::stof;
        std::string line;
        getline(objFile, line);
        std::vector<std::string> tokens = split(line, " ");

        if (tokens[0].compare("mtllib") == 0)
        {
            std::ifstream ifs(tokens[1]);
            material(ifs);
            ifs.close();
        }

        if (tokens[0].compare("usemtl") == 0)
        {
        }
 
        if (tokens[0].compare("v") == 0)
        {
            float x = stof(tokens[1]);
            float y = stof(tokens[2]);
            float z = stof(tokens[3]);
            vertexBuf.push_back(XYZ(x, y, z));
        }

        if (tokens[0].compare("vn") == 0)
        {
            normalsBuf.push_back(stof(tokens[1]));
            normalsBuf.push_back(stof(tokens[2]));
            normalsBuf.push_back(stof(tokens[3]));
        }

        if (tokens[0].compare("vt") == 0)
        {
            texturesBuf.push_back(stof(tokens[1]));
            texturesBuf.push_back(stof(tokens[2]));
        }

        // The first character is an 'f': on this line is a point stored
        if (tokens[0].compare("f") == 0)
        {
            int normalsNumber[3];

            float coord[3][3];
            float va[3], vb[3], vr[3], val;
            std::vector<std::string> vs[3];
            vs[0] = split(tokens[1], "/");
            vs[1] = split(tokens[2], "/");
            vs[2] = split(tokens[3], "/");

            if (vs[0].size() > 0)
            {
                int vertexNumber[3];
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
                int texturesNumber[3];
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

void foo()
{
      auto logger = spdlog::stderr_color_mt("client");
  //logger->set_level(spdlog::level::debug);
  try
    {
      //std::string endpoint = "opc.tcp://192.168.56.101:48030";
      //std::string endpoint = "opc.tcp://user:password@192.168.56.101:48030";
      std::string endpoint = "opc.tcp://127.0.0.1:4840/freeopcua/server/";
      //std::string endpoint = "opc.tcp://localhost:53530/OPCUA/SimulationServer/";
      //std::string endpoint = "opc.tcp://localhost:48010";

      logger->info("Connecting to: {}", endpoint);
      OpcUa::UaClient client(logger);
      client.Connect(endpoint);

      //get Root node on server
      OpcUa::Node root = client.GetRootNode();
      logger->info("Root node is: {}", root);

      //get and browse Objects node
      logger->info("Child of objects node are:");
      Node objects = client.GetObjectsNode();

      for (OpcUa::Node node : objects.GetChildren())
        { logger->info("    {}", node); }

      //get a node from standard namespace using objectId
      logger->info("NamespaceArray is:");
      OpcUa::Node nsnode = client.GetNode(ObjectId::Server_NamespaceArray);
      OpcUa::Variant ns = nsnode.GetValue();

      for (std::string d : ns.As<std::vector<std::string>>())
        { logger->info("    {}", d); }

      OpcUa::Node myvar;
      OpcUa::Node myobject;
      OpcUa::Node mymethod;

      //Initialize Node myvar:

      //Get namespace index we are interested in

      // From freeOpcUa Server:
      uint32_t idx = client.GetNamespaceIndex("http://examples.freeopcua.github.io");
      ////Get Node using path (BrowsePathToNodeId call)
      //std::vector<std::string> varpath({ std::to_string(idx) + ":NewObject", "MyVariable" });
      //myvar = objects.GetChild(varpath);
      std::vector<std::string> methodpath({ std::to_string(idx) + ":NewObject" });
      myobject = objects.GetChild(methodpath);
      methodpath = { std::to_string(idx) + ":NewObject", "MyMethod" };
      mymethod = objects.GetChild(methodpath);
      std::vector<OpcUa::Variant> arguments;
      arguments.push_back(static_cast<uint8_t>(0));
      myobject.CallMethod(mymethod.GetId(), arguments);
      std::vector<std::string> varpath{ "Objects", "Server", "ServerStatus", "CurrentTime" };
      myvar = root.GetChild(varpath);

      logger->info("got node: {}", myvar);

      //Subscription
      SubClient sclt;
      Subscription::SharedPtr sub = client.CreateSubscription(100, sclt);
      uint32_t handle = sub->SubscribeDataChange(myvar);
      logger->info("Got sub handle: {}, sleeping 5 seconds", handle);
      std::this_thread::sleep_for(std::chrono::seconds(5));

      logger->info("Disconnecting");
      client.Disconnect();
      logger->flush();
    }

  catch (const std::exception & exc)
    {
      logger->error("Error: {}", exc.what());
    }

  catch (...)
    {
      logger->error("Unknown error.");
    }
}

void CMain::run(int argc, char **argv)
{
    std::thread th1(foo);
    //foo();
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

    glShadeModel( GL_SMOOTH );
    glEnable( GL_DEPTH_TEST );

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


