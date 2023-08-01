#include <iostream>
#include <cstdint>

struct Triangle
{
    float n1, n2, n3;
    float x1, y1, z1;
    float x2, y2, z2;
    float x3, y3, z3;
};

int main()
{
    std::cin.ignore(80);
    uint32_t n_triangles;
    std::cin.read(reinterpret_cast<char *>(&n_triangles), sizeof(n_triangles));
    Triangle buf[n_triangles];
    
    for (uint32_t i = 0; i < n_triangles; ++i)
    {
        std::cin.read(reinterpret_cast<char *>(&buf[i].n1), sizeof(float));
        std::cin.read(reinterpret_cast<char *>(&buf[i].n2), sizeof(float));
        std::cin.read(reinterpret_cast<char *>(&buf[i].n3), sizeof(float));
        std::cin.read(reinterpret_cast<char *>(&buf[i].x1), sizeof(float));
        std::cin.read(reinterpret_cast<char *>(&buf[i].y1), sizeof(float));
        std::cin.read(reinterpret_cast<char *>(&buf[i].z1), sizeof(float));
        std::cin.read(reinterpret_cast<char *>(&buf[i].x2), sizeof(float));
        std::cin.read(reinterpret_cast<char *>(&buf[i].y2), sizeof(float));
        std::cin.read(reinterpret_cast<char *>(&buf[i].z2), sizeof(float));
        std::cin.read(reinterpret_cast<char *>(&buf[i].x3), sizeof(float));
        std::cin.read(reinterpret_cast<char *>(&buf[i].y3), sizeof(float));
        std::cin.read(reinterpret_cast<char *>(&buf[i].z3), sizeof(float));
    }

    return 0;
}


