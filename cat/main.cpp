#include "toolbox.h"
#include "options.h"
#include <fstream>

#if 0
#ifdef WINCE
#include <windows.h>
#endif
#endif

static void cat(Options &options)
{
    if (options.stdinput())
    {
#if 1
        std::string buf;

        while (std::getline(std::cin, buf))
            std::cout << buf << std::endl;
#else
        for (int c; (c = std::cin.get());)
            std::cout.put(c);
#endif
        return;
    }

    for (Options::si it = options._fnBegin(); it != options._fnEnd(); ++it)
    {
        std::ifstream ifs;
#ifndef CPP11
        ifs.open(it->c_str(), std::ifstream::in);
#else
        ifs.open(*it, std::ifstream::in);
#endif
        for (int c; (c = ifs.get()) != EOF;)
            std::cout << (char)c;

        ifs.close();
    }
}

#ifdef WINCE
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nCmdShow)
{
    Options options;
    options.parse(lpCmdLine);
    cat(options);
    return 0;
}
#else
int main(int argc, char **argv)
{
    Options options;
    options.parse(argc, argv);
    cat(options);
    return 0;
}
#endif

