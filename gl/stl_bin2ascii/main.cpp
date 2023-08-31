#include <iostream>
#include <cstdint>

static float readF(std::istream &is)
{
    float tmp;
    is.read(reinterpret_cast<char *>(&tmp), sizeof(tmp));
    return tmp;
}

int main()
{
    std::cin.ignore(80);
    uint32_t n_triangles;
    std::cin.read(reinterpret_cast<char *>(&n_triangles), sizeof(n_triangles));
    std::cout << "solid " << n_triangles << "\r\n";

    for (uint32_t i = 0; i < n_triangles; ++i)
    {
        float tmp;
        std::cout << "   facet normal ";
        std::cout << readF(std::cin);
        std::cout << " ";
        std::cout << readF(std::cin);
        std::cout << " ";
        std::cout << readF(std::cin);
        std::cout << "\r\n";
        std::cout << "      outer loop\r\n";
        std::cout << "         vertex ";
        std::cout << readF(std::cin);
        std::cout << " ";
        std::cout << readF(std::cin);
        std::cout << " ";
        std::cout << readF(std::cin);
        std::cout << "\r\n";
        std::cout << "         vertex ";
        std::cout << readF(std::cin);
        std::cout << " ";
        std::cout << readF(std::cin);
        std::cout << " ";
        std::cout << readF(std::cin);
        std::cout << "\r\n";       
        std::cout << "         vertex ";
        std::cout << readF(std::cin);
        std::cout << " ";
        std::cout << readF(std::cin);
        std::cout << " ";
        std::cout << readF(std::cin);
        std::cout << "\r\n";
        std::cout << "      endloop\r\n";
        std::cout << "   endfacet\r\n";
        std::cin.ignore(2);
    }

    std::cout << "endsolid\r\n";
    std::cout.flush();

    return 0;
}


