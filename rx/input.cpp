#include "input.h"
#include "logger.h"
#include <unistd.h>
#include <iostream>

#ifdef WIN32
InputStreamWin::InputStreamWin(DWORD fd, Logger *log)
    : _fd(fd), _log(log)
{

}

void InputStreamWin::init()
{
    _handle = GetStdHandle(_fd);
    GetConsoleMode(_handle, &_oldMode);
    SetConsoleMode(_handle, 0);
}

BOOL InputStreamWin::thereIsCharEvents() const
{
    constexpr DWORD MAX_RECORDS = 2048;
    INPUT_RECORD tInput[MAX_RECORDS];
    DWORD dwEvents = 0;
    BOOL ret = PeekConsoleInputA(_handle, tInput, MAX_RECORDS, &dwEvents);

    if (ret == 0)
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
            BOOL tisce = thereIsCharEvents();

            if (tisce == FALSE)
                continue;

            char buf;
            DWORD read = 0;
            BOOL rf = ReadFile(_handle, &buf, 1, &read, NULL);

            if (rf == FALSE)
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

int InputStreamWin::get(char *buf, size_t n, int timeout)
{
    _log->logf("getting %u bytes, timeout %ds...", n, timeout);
    for (size_t i = 0; i < n; ++i)
    {
        int c = getc(timeout);

        if (c == -1)
            return i;

        buf[i] = c;
    }
    return n;
}
#else
InputStreamUnix::InputStreamUnix(int fd, Logger *log)
    : _fd(fd), _log(log)
{

}

void InputStreamUnix::init()
{
    _alarm.init();
}

int InputStreamUnix::get(char *buf, size_t n, int timeout)
{
    _alarm.set(timeout);
    ssize_t ret = ::read(_fd, buf, n);
    _alarm.set(0);
    return ret;
}

int InputStreamUnix::getc(int timeout)
{
    _alarm.set(timeout);
    char c;
    ssize_t ret = ::read(_fd, &c, 1);
    _alarm.set(0);
    return ret > 0 ? c : -1;
}
#endif


