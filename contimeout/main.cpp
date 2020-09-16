#include "input.h"
#include <iostream>

int main()
{
#ifdef WIN32
    InputStreamWin is(STD_INPUT_HANDLE);
#else
    InputStreamUnix is(0);
#endif
    is.init();

    while (true)
    {
        int c = is.getc(2);
        std::cout << c << "\r\n";
        std::cout.flush();
    }

    // restore console mode when exit
    //SetConsoleMode(hStdIn, oldMode);
    return 0;
}

