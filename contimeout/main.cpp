#include <windows.h>
#include <iostream>
using namespace std;

BOOL ThereIsCharEvents(HANDLE hStdIn)
{
    INPUT_RECORD tInput[255];
    DWORD dwEvents;

    if (PeekConsoleInput(hStdIn, tInput, 256, &dwEvents) == 0)
        return TRUE;

    if (dwEvents == 0)
        return TRUE;

    for (DWORD i = 0; i < dwEvents; i++)
    {
        if (tInput[i].EventType != KEY_EVENT)
            continue;

        if (tInput[i].Event.KeyEvent.bKeyDown == FALSE)
            continue;

        if (tInput[i].Event.KeyEvent.uChar.AsciiChar)
            return TRUE;
    }

    ReadConsoleInput(hStdIn, tInput, dwEvents, &dwEvents);
    return FALSE;
}

int main()
{
    DWORD oldMode;
    HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hStdIn, &oldMode);
    DWORD fdwMode = ENABLE_ECHO_INPUT;
    SetConsoleMode(hStdIn, fdwMode);
    //FlushConsoleInputBuffer(hStdIn);

    while (true)
    {
        int c = 0;
        //FlushConsoleInputBuffer(hStdIn);

        DWORD ret = WaitForSingleObject(hStdIn, 2000);
        //std::cout << "WaitForSingleObject: " << ret << "\r\n";
        std::cout.flush();

        if (ret == WAIT_OBJECT_0)
        {
            if (ThereIsCharEvents(hStdIn) == FALSE)
                continue;

            char buf;
            DWORD read = 0;

            if (ReadFile(hStdIn, &buf, 1, &read, NULL) == FALSE)
                break;

            if (read == 0)
                continue;

            if (c == 13 && buf == 10)
            {
                c = 0;
                continue;
            }

            c = buf;

            if (buf == 13)
                buf = 10;

            std::cout << c << "\r\n";
            std::cout.flush();
        }
        else if (ret == WAIT_TIMEOUT)
        {
            std::cout << "Timeout!\r\n";
            std::cout.flush();
        }


    }

    // restore console mode when exit
    //SetConsoleMode(hStdIn, oldMode);
    return 0;
}

