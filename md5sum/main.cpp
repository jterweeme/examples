#include "hasher.h"
#include <fstream>

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

#ifdef WINCE
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nCmdShow)
{
    std::wcout << lpCmdLine << "\n";
    std::wcout.flush();
    size_t len = wcslen(lpCmdLine);

    if (len < 1)
        return 0;

    std::ifstream ifs;
    ifs.open(lpCmdLine, std::ifstream::in | std::ifstream::binary);

    if (!ifs.good())
    {
        std::cerr << "Cannot open file!\n";
        std::cerr.flush();
    }

    Hash hash = Hasher::stream(ifs);
    hash.dump(std::cout);
    std::cout << "\n";
    std::cout.flush();
    ifs.close();
    return 0;
}
#else
int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
#ifdef WIN32
    _setmode(_fileno(stdin), _O_BINARY);
#endif
    Hash hash = Hasher::stream(std::cin);
    hash.dump(std::cout);
    std::cout << "\n";
    std::cout.flush();
    return 0;
}
#endif
