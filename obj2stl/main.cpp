#include <vector>
#include <iostream>

template <typename T> struct vec3
{
    T x, y, z;
};

using float3 = vec3<float>;

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

std::vector<float3> vertices;
std::vector<float3> normals;

void process(std::istream &is, std::ostream &os)
{
    os << "solid Onzin\r\n";
    std::string line;

    while (!is.eof())
    {
        getline(is, line);
        std::vector<std::string> tokens = split(line, " ");

        if (tokens[0].compare("v") == 0)
        {
            float3 foo;
            foo.x = std::stof(tokens[1]);
            foo.y = std::stof(tokens[2]);
            foo.z = std::stof(tokens[3]);
            vertices.push_back(foo);
        }

        if (tokens[0].compare("vn") == 0)
        {
            float3 foo;
            foo.x = std::stof(tokens[1]);
            foo.y = std::stof(tokens[2]);
            foo.z = std::stof(tokens[3]);
            normals.push_back(foo);
        }

        if (tokens[0].compare("f") == 0)
        {
            float vertexNumber[3];
            vec3<int> normalsNumber;
            int texturesNumber[3];
            std::vector<std::string> vs[3];
            vs[0] = split(tokens[1], "/");
            vs[1] = split(tokens[2], "/");
            vs[2] = split(tokens[3], "/");

            if (vs[0].size() > 2)
            {
                normalsNumber.x = std::stoi(vs[0][2]) - 1;
                normalsNumber.y = std::stoi(vs[1][2]) - 1;
                normalsNumber.z = std::stoi(vs[2][2]) - 1;
                float3 normal = normals[normalsNumber.x];

                os << "   facet normal " << normal.x << " " << normal.y
                   << " " << normal.z << "\r\n";

                os << "      outer loop\r\n";

                vertexNumber[0] = std::stoi(vs[0][0]) - 1;
                vertexNumber[1] = std::stoi(vs[1][0]) - 1;
                vertexNumber[2] = std::stoi(vs[2][0]) - 1;

                for (int i = 0; i < 3; ++i)
                {
                    float3 vertex = vertices[vertexNumber[i]];
                
                    os << "         vertex " << vertex.x << " " << vertex.y << " "
                       << vertex.z << "\r\n";
                }

                os << "      endloop\r\n";
                os << "   endfacet\r\n";
            }
        }
    }
    os << "endsolid\r\n\r\n";
}

int main()
{
    process(std::cin, std::cout);
    return 0;
}


