#include "input.h"
#include "xrecv.h"
#include <iostream>
#include <fstream>

#ifdef WIN32
#include <windows.h>
#endif

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Enter target filename";
        std::cerr.flush();
        return -1;
    }
#if 0
    std::ofstream ofs;
    ofs.open(argv[1], std::ofstream::out | std::ofstream::app);
    XReceiver xReceiver(&std::cin, &std::cout);
    xReceiver.receive(ofs);
    ofs.close();
#endif
    return 0;
}

