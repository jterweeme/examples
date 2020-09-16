#include "input.h"

InputStreamWin::InputStreamWin(DWORD fd) : _fd(fd)
{

}

void InputStreamWin::init()
{
    _handle = GetStdHandle(_fd);
    GetConsoleMode(_handle, &_oldMode);
    SetConsoleMode(_handle, ENABLE_ECHO_INPUT);
}

BOOL InputStreamWin::thereIsCharEvents() const
{
    INPUT_RECORD tInput[255];
    DWORD dwEvents;

    if (PeekConsoleInput(_handle, tInput, 256, &dwEvents) == 0)
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

    ReadConsoleInput(_handle, tInput, dwEvents, &dwEvents);
    return FALSE;
}

int InputStreamWin::getc(int timeout)
{
    int c = 0;

    while (true)
    {
        DWORD ret = WaitForSingleObject(_handle, timeout * 1000);

        if (ret == WAIT_OBJECT_0)
        {
            if (thereIsCharEvents() == FALSE)
                continue;

            char buf;
            DWORD read = 0;

            if (ReadFile(_handle, &buf, 1, &read, NULL) == FALSE)
                return -1;

            if (read == 0)
                continue;

            c = buf;

            if (buf == 13)
                buf = 10;

            return c;
        }

        return -1;
    }

    return -1;
}
