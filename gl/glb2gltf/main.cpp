#include <iostream>
#include <fstream>
#include <cstdint>

template <typename T> T readX(std::istream &is)
{
    T ret;
    is.read(reinterpret_cast<char *>(&ret), sizeof(ret));
    return ret;
}

int main(int argc, char **argv)
{
    std::ifstream ifs(argv[1]);
    uint32_t magic = readX<uint32_t>(ifs);
    uint32_t version = readX<uint32_t>(ifs);
    std::cout << "version: " << version << "\r\n";
    uint32_t filelength = readX<uint32_t>(ifs);
    std::cout << "filelength: " << filelength << "\r\n";
    uint32_t chunklength = readX<uint32_t>(ifs);
    std::cout << "chunk length: " << chunklength << "\r\n";
    uint32_t json = readX<uint32_t>(ifs);
    char chunk[chunklength];
    ifs.read(chunk, chunklength);
    std::ofstream ofs(std::string(argv[1]) + ".gltf");
    ofs.write(chunk, chunklength);
    ofs.close();
    chunklength = readX<uint32_t>(ifs);
    uint32_t bin = readX<uint32_t>(ifs);
    char chunk2[chunklength];
    ifs.read(chunk2, chunklength);
    ofs.open(std::string(argv[1]) + ".bin");
    ofs.write(chunk2, chunklength);
    ofs.close();
    ifs.close();
    return 0;
}


