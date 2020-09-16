#include "input.h"
#include <iostream>

int main()
{
    InputStreamWin is(STD_INPUT_HANDLE);
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

