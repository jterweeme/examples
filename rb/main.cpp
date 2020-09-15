#include <windows.h>
#include <iostream>
using namespace std;

int main()
{
    //std::string inStr;
    DWORD fdwOldMode;
    HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hStdIn, &fdwOldMode);

    DWORD fdwMode = fdwOldMode;

    // disable mouse and window input
    fdwMode &= ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT | ENABLE_LINE_INPUT);

    SetConsoleMode(hStdIn, fdwMode);

    // flush to remove existing events
    FlushConsoleInputBuffer(hStdIn);

    while (true)
    {
        std::cout << "Debug bericht 1\r\n";
        std::cout.flush();

        int c = 0;
        FlushConsoleInputBuffer(hStdIn);
        if (WaitForSingleObject(hStdIn, 10000) == WAIT_OBJECT_0)
        {
            c = std::cin.get();
        }

        std::cout << c << "\r\n";
        std::cout.flush();
    }
    // restore console mode when exit
    SetConsoleMode(hStdIn, fdwOldMode);
    return 0;
}

