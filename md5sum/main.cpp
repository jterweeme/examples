#include "hasher.h"
#include <fstream>

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

    std::ifstream ifs;
    ifs.open("baku.png", std::ifstream::in | std::ifstream::binary);
#if 0
    size_t n = 0;

    while (ifs.good())
    {
        ifs.get();
        n++;
    }

    std::cout << ifs.fail() << "\r\n";
    std::cout << n << "\r\n";
    std::cout.flush();
#endif
#if 1
    Hash hash = Hasher::stream(ifs);
    hash.dump(std::cout);
    std::cout << "\n";
    std::cout.flush();
#endif

    return 0;
}
#endif
