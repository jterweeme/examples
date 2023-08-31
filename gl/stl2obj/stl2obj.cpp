#include <iostream>
#include <cstdint>
#include <array>
#include <vector>

template <typename T> T readX(std::istream &is)
{
    T ret;
    is.read(reinterpret_cast<char *>(&ret), sizeof(ret));
    return ret;
}

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

struct History
{
    int n;
    VertexPoint vp;
};

struct Triangle
{
    float n1, n2, n3;
    VertexPoint vp[3];
};

class Log
{
    std::vector<History> history;
};

Log xlog;

int main()
{
    using std::cin;
    using std::cout;
    std::cin.ignore(80);
    uint32_t n_triangles = readX<uint32_t>(std::cin);
    std::cerr << "n_triangles: " << n_triangles << "\r\n";
    std::cerr.flush();
    int k = 1;
    int l = 1;

    
    for (uint32_t i = 0; i < n_triangles; ++i)
    {
        Triangle buf;
        std::cerr << i << "\r\n";
        std::cerr.flush();
        buf.n1 = readX<float>(cin);
        buf.n2 = readX<float>(cin);
        buf.n3 = readX<float>(cin);
        cout << "vn " << buf.n1 << " " << buf.n2 << " " << buf.n3 << "\r\n";

        for (uint32_t j = 0; j < 3; ++j)
        {
            float x = readX<float>(cin);
            float y = readX<float>(cin);
            float z = readX<float>(cin);
            VertexPoint vp(x, y, z);
            //history.push_back(vp);
            buf.vp[j] = vp;

            cout << "v " << buf.vp[j].x() << " " << buf.vp[j].y()
                 << " " << buf.vp[j].z() << "\r\n";
        }

        cin.ignore(2);

        cout << "f " << k++ << "//" << l << " ";
        cout << k++ << "//" << l << " ";
        cout << k++ << "//" << l++ << "\r\n";
    }

    return 0;
}


